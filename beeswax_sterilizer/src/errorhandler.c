/*
 *   Copyright (C) 2017  Gyorgy Stercz
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/** \file errorhandler.c
  * \brief Errorhandler thread source.
  */

#include <stdlib.h>
#include <string.h>

#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <appconf.h>
#include <gpiosetup.h>
#include <errorhandler.h>
#include <sterilizer.h>
#include <lcdcontrol.h>

#if ERRORHANDLER_STACK_SIZE < 128
    #error Minimum task stack size is 128!
#endif

#if ERRORHANDLER_SLEEP_TIME_US < 1
    #error task sleep time must be at least 1
#endif

#if ERRORHANDLER_SLEEP_TIME_US < 100
    #warning task sleep time seems to be to few!
#endif

static THD_WORKING_AREA(waThreaderrorhandler, ERRORHANDLER_STACK_SIZE);
static MUTEX_DECL(errmtx);

/** \brief Error list memory pool.
  *
  * \{
  */
static struct error_item err_items[ERROR_LIST_MAX_SIZE];
static MEMORYPOOL_DECL(errpool, sizeof(struct error_item), NULL);
/**\} */

/** \brief Error names.
  */
static char *errortypes[] = {"Sensor0 error",
                             "Sensor1 error",
                             "Sensor2 error",
                             "Critically temperature rise",
                             "Critically high temperature",
                             "Fuzzy logic error",
                             "CH0 fuse error",
                             "CH1 fuse error",
                             "CH2 fuse error"};

/** \brief Structure for thread data.
  */
static struct{
    msg_t mb_buff[ERR_HANDL_MAILBOX_SIZE];
    msg_t curr_massage;
    STAILQ_HEAD(errorlisthead, error_item) head;
    struct errorlisthead *headp;
    uint8_t erroritems;
    uint8_t freeitems;
    uint8_t poolunderflow;
    virtual_timer_t vt;
}errhandl;



static MAILBOX_DECL(error_mb, errhandl.mb_buff, ERR_HANDL_MAILBOX_SIZE);

static struct GPIO_Pin int_ch[CHANNEL_NUM] = {INT_CH0, INT_CH1, INT_CH2};

/** \brief CHO fuse error callback.
  */
static void ch0FuseError(void *arg){
    (void)arg;
    if (palReadPad(int_ch[0].port, int_ch[0].pin))
        chMBPostI(&error_mb, CH0_FUSE_ERROR);
    extChannelEnable(&EXTD1, int_ch[0].pin);
}

/** \brief CHO relay debounce timer.
  */
static void ch0RelayDebounce(EXTDriver *extp, expchannel_t channel){
    (void)extp;
    (void)channel;
    extChannelDisableI(&EXTD1, int_ch[0].pin);
    chVTResetI(&errhandl.vt);
    chVTSetI(&errhandl.vt, MS2ST(10), ch0FuseError, NULL);
}

/** \brief CH1 fuse error callback.
  */
static void ch1FuseError(void *arg){
    (void)arg;
    if (palReadPad(int_ch[1].port, int_ch[1].pin))
        chMBPostI(&error_mb, CH1_FUSE_ERROR);
    extChannelEnable(&EXTD1, int_ch[1].pin);
}

/** \brief CH1 relay debounce timer.
  */
static void ch1RelayDebounce(EXTDriver *extp, expchannel_t channel){
    (void)extp;
    (void)channel;
    extChannelDisableI(&EXTD1, int_ch[1].pin);
    chVTResetI(&errhandl.vt);
    chVTSetI(&errhandl.vt, MS2ST(10), ch1FuseError, NULL);
}

/** \brief CH2 fuse error callback.
  */
static void ch2FuseError(void *arg){
    (void)arg;
    if (palReadPad(int_ch[2].port, int_ch[2].pin))
        chMBPostI(&error_mb, CH2_FUSE_ERROR);
    extChannelEnable(&EXTD1, int_ch[2].pin);
}

/** \brief CH2 relay debounce timer.
  */
static void ch2RelayDebounce(EXTDriver *extp, expchannel_t channel){
    (void)extp;
    (void)channel;
    extChannelDisableI(&EXTD1, int_ch[2].pin);
    chVTResetI(&errhandl.vt);
    chVTSetI(&errhandl.vt, MS2ST(10), ch2FuseError, NULL);
}

/** \brief EXT Driver configuration.
  */
static const EXTConfig extcfg ={
    {
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART| EXT_MODE_GPIOI, ch0RelayDebounce},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART| EXT_MODE_GPIOB, ch2RelayDebounce},
        {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART| EXT_MODE_GPIOA, ch1RelayDebounce},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL}
    }
};

/** \brief Errorhandler thread function.
  *         - Receive massage for error mailbox and create error list.
  */
__attribute__((noreturn))
static THD_FUNCTION(Threaderrorhandler, arg) {
    (void) arg;
    chRegSetThreadName("errorhandler");
    struct error_item *item;
    uint8_t i;
    for(i=0; i<CHANNEL_NUM; i++){
        if (palReadPad(int_ch[i].port, int_ch[i].pin))
            chMBPost(&error_mb, CH0_FUSE_ERROR+i, TIME_INFINITE);
    }
    while(TRUE) {
        chMBFetch(&error_mb, &errhandl.curr_massage, TIME_INFINITE);
        if (errhandl.curr_massage)
            item = chPoolAlloc(&errpool);
        if(item){
            switch(errhandl.curr_massage){
                case SENSOR0_ERR_MSG:
                case SENSOR1_ERR_MSG:
                case SENSOR2_ERR_MSG:
                case CRIT_DTEMP_ERR_MSG:
                case CRIT_TEMP_ERR_MSG:
                case FUZZY_LOGIC_ERR_MSG:
                case CH0_FUSE_ERROR:
                case CH1_FUSE_ERROR:
                case CH2_FUSE_ERROR:        chMtxLock(&errmtx);
                                            item->error_str = errortypes[errhandl.curr_massage-1];
                                            if (STAILQ_EMPTY(&errhandl.head))
                                                STAILQ_INSERT_HEAD(&errhandl.head, item, entries);
                                            else
                                                STAILQ_INSERT_TAIL(&errhandl.head, item, entries);
                                            errhandl.erroritems++;
                                            errhandl.freeitems--;
                                            chMtxUnlock(&errmtx);
                                            errhandl.curr_massage = 0;
                                            sendDisableMailToRegluator(FUZZY_REG_DISABLE_MSG);
                                            sendMailtoSterilizer(STOPERROR_STERILIZER);
                                            displayErrorListItem(item->error_str);
                                            break;
            }
        }
        else{
            errhandl.poolunderflow++;
        }
        item = NULL;
        chThdSleepMicroseconds(ERRORHANDLER_SLEEP_TIME_US);
    }
    chThdExit(1);
}

/** \brief Sends mailbox massage to errorhandler thread.
  *
  * \param msg  massage code.
  */
void sendErrMail(msg_t msg){
    chMBPost(&error_mb, msg, TIME_INFINITE);
}

/** \brief Errorlist user interface
  *
  */
void cmd_errorlist(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    chprintf(chp, "Error list size: %d\r\n", ERROR_LIST_MAX_SIZE);
    chprintf(chp, "Error list free items: %d\r\n", errhandl.freeitems);
    chprintf(chp, "Error  list item number: %d\r\n", errhandl.erroritems);
    chprintf(chp, "Error list pool underflow: %d\r\n", errhandl.poolunderflow);
}


/** \brief Initializes errorhandler
  *         - Error list init.
  *         - EXT driver start.
  *         - Creates errorhandler thread.
  */
void errorhandlerInit(void){
    bzero(&errhandl, sizeof(errhandl));
    STAILQ_INIT(&errhandl.head);
    errhandl.freeitems = ERROR_LIST_MAX_SIZE;
    chPoolLoadArray(&errpool, err_items, ERROR_LIST_MAX_SIZE);
    extStart(&EXTD1, &extcfg);
    chThdCreateStatic(waThreaderrorhandler, sizeof(waThreaderrorhandler), NORMALPRIO+30, Threaderrorhandler, NULL);
}
