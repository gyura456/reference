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
/** \file regulator.c
  * \brief Regulator thread source.
  */

#include <stdlib.h>
#include <string.h>

#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <sys/queue.h>
#include <tempreader.h>
#include <lcdcontrol.h>
#include <errorhandler.h>
#include <cardhandler.h>
#include "regulator.h"

#if REGULATOR_STACK_SIZE < 128
    #error Minimum task stack size is 128!
#endif

#if REGULATOR_SLEEP_TIME_US < 1
    #error task sleep time must be at least 1
#endif

#if REGULATOR_SLEEP_TIME_US < 100
    #warning task sleep time seems to be to few!
#endif

static THD_WORKING_AREA(waThreadregulator, REGULATOR_STACK_SIZE);
static MUTEX_DECL(regmtx);
/*===========================================================================*/
/* Temperature FIFO.                                                         */
/*===========================================================================*/

/** \brief Temperature FIFO
  *
  */
static temperature_t tempitems[TEMP_FIFO_SIZE];
static MEMORYPOOL_DECL(tempbuffer, sizeof(temperature_t), NULL);
static inner_buffer_t tempFIFO;
/**\} */

/** \brief Get an empty temperature FIFO item.
  *
  * \return Pointer to FIFO item or NULL, if the FIFO is full.
  */
struct inner_buffer_item *getTempFIFOItem(void){
    return getEmptyInnerBufferItem(&tempFIFO);
}

/** \brief Put temperature into the FIFO.
  *
  * \param item     Pointer of the FIFO item, NULL save.
  */
void putTempToFIFO(struct inner_buffer_item *item){
    postFullInnerBufferItem(&tempFIFO, item);
}

/** \brief Says, that the temperature FIFO is full.
  *
  * \return TRUE, if the FIFO is full, FALSE if not full.
  */
int8_t isTempFIFOFull(void){
    return isInnerBufferFull(&tempFIFO);
}

/*===========================================================================*/
/* Fuzzy regulator data                                                      */
/*===========================================================================*/

/** \brief Structure for thread data.
  */
static struct{
    msg_t mb_buff[FUZZYREG_MAILBOX_SIZE];
    msg_t curr_msg;
    heat_channel_t heat_ch[CHANNEL_NUM];
    fuzzyreg_state_t state;
    uint8_t fuzzy_errors;
    pwmcnt_t dutycycle[CHANNEL_NUM];
    temperature_t curr_temp;
    RTCDateTime starttime;
    int16_t start_temp[CHANNEL_NUM];
    uint32_t idle_time[CHANNEL_NUM];
    uint32_t melting_time[CHANNEL_NUM];
    float tg_alpha[CHANNEL_NUM];
    systime_t checktime;
    uint8_t logfile_error;
    char logbuff[FILE_BUFFER_ITEM_SIZE];
    uint32_t lognum;
}fuzzyreg;


static MAILBOX_DECL(fuzzyreg_mb, fuzzyreg.mb_buff, FUZZYREG_MAILBOX_SIZE);
/*===========================================================================*/
/* PWM channels configuration.                                               */
/*===========================================================================*/

static void setDutyCycleCH0(PWMDriver *pwmp){
    (void)pwmp;
    pwmEnableChannelI(fuzzyreg.heat_ch[0].pwmp, fuzzyreg.heat_ch[0].chnum, fuzzyreg.dutycycle[0]);
}

static void setDutyCycleCH1(PWMDriver *pwmp){
    (void)pwmp;
    pwmEnableChannelI(fuzzyreg.heat_ch[1].pwmp, fuzzyreg.heat_ch[1].chnum, fuzzyreg.dutycycle[1]);
}

static void setDutyCycleCH2(PWMDriver *pwmp){
    (void)pwmp;
    pwmEnableChannelI(fuzzyreg.heat_ch[2].pwmp, fuzzyreg.heat_ch[2].chnum, fuzzyreg.dutycycle[2]);
}

static const PWMConfig channels_cfg[CHANNEL_NUM] = HEAT_CHANNELS_CFG;

/*===========================================================================*/
/* Fuzzy logic                                                               */
/*===========================================================================*/

/** \brief Fuzzyfication crisp input with triangle type
  *        membership function.
  * \note maxto field in the membership function structure
  *       don't care by this type.
  *     ^
  *     |
  *   1 -           maxfrom
  *     |             /\
  *     |            /  \
  *     |___________/    \______________
  *  ---------------|----|----------------------------------->
  *         rangefrom     rangeto
  * \param mf    pointer to membership function structure, NULL save.
  * \param input crisp input
  * \return fuzzy value of input, or 2 if pointer of membership function structure is NULL
  *         or the membership function structure has wrong values.
  */
static float fuzzyficTriangleTypeMf(void *mf, int16_t input){
    if (!mf)
        return 2.0;
    input_mf *mfp = (input_mf*)mf;
    if (input < mfp->rangefrom || input > mfp->rangeto)
        return 0.0;
    if (input >= mfp->rangefrom && input <= mfp->maxfrom)
        return (float)(input-mfp->rangefrom)/(float)(mfp->maxfrom-mfp->rangefrom);
    if (input > mfp->maxfrom && input <= mfp->rangeto)
        return (float)(mfp->rangeto-input)/(float)(mfp->rangeto-mfp->maxfrom);
    return 2.0;
}

/** \brief Fuzzyfication crisp input with half trapeze type
  *        membership function.
  * \note maxto field in the membership function structure
  *       don't care by this type.
  *     ^
  *     |
  *   1 -    ________ maxfrom        maxfrom ______
  *     |              \                    /
  *     |               \                  /
  *     |                \______     _____/
  *  -----------------|--|----------------|-|---------------->
  *          rangefrom   rangeto  rangefrom  rangeto
  * \param mf    pointer to membership function structure, NULL save.
  * \param input crisp input
  * \return fuzzy value of input, or 2 if pointer of membership function structure is NULL
  *         or the membership function structure has wrong values.
  */
static float fuzzyficHalfTrapezeTypeMf(void *mf, int16_t input){
    if (!mf)
        return 2.0;
    input_mf *mfp = (input_mf*)mf;
    if (mfp->rangefrom == mfp->maxfrom){
        if (input < mfp->rangefrom)
            return 1.0;
        if (input > mfp->rangeto)
            return 0.0;
        if (input >= mfp->rangefrom && input <= mfp->rangeto)
            return (float)(mfp->rangeto-input)/(float)(mfp->rangeto-mfp->rangefrom);
    }
    if (mfp->rangeto == mfp->maxfrom){
        if (input < mfp->rangefrom)
        return 0.0;
    if (input > mfp->rangeto)
        return 1.0;
    if (input >= mfp->rangefrom && input <= mfp->rangeto)
        return (float)(input-mfp->rangefrom)/(float)(mfp->rangeto-mfp->rangefrom);
    }
    return 2.0;
}
/** \brief Fuzzyfication crisp input with trapeze type
  *        membership function.
  *     ^
  *     |
  *   1 -      maxfrom _________ maxto
  *     |             /         \
  *     |            /           \
  *     |___________/             \______________
  *  ---------------|-------------|-------------------------->
  *         rangefrom             rangeto
  * \param mf    pointer to membership function structure, NULL save.
  * \param input crisp input
  * \return fuzzy value of input, or 2 if pointer of membership function structure is NULL
  *         or the membership function structure has wrong values.
  */
static float fuzzyficTrapezeTypeMf(void *mf, int16_t input){
    if (!mf)
        return 2.0;
    input_mf *mfp = (input_mf*)mf;
    if (input < mfp->rangefrom || input > mfp->rangeto)
        return 0.0;
    if (input >= mfp->rangefrom && input < mfp->maxfrom)
        return (float)(input-mfp->rangefrom)/(float)(mfp->maxfrom-mfp->rangefrom);
    if (input >= mfp->maxfrom && input<= mfp->maxto)
        return 1;
    if (input > mfp->maxto && input <= mfp->rangeto)
        return (float)(mfp->rangeto-input)/(float)(mfp->rangeto-mfp->maxto);
    return 2.0;
}

/** \brief Temperature input membership functions.
  */
static struct Temp_mship temp_mships = TEMP_INPUT_MFS;

/** \brief Delta temperature input membership functions.
  */
static struct DeltaTemp_mship dtemp_mships = DTEMP_INPUT_MFS;;

/** \brief PWM output membership functions.
  */
static struct PWM_mship pwm_mships = PWM_OUTPUT_MFS;

/** \brief Fuzzy rules array.
  */
fuzzy_rule rules[FUZZY_RULES_NUM] = FUZZY_RULES;

/** \brief Structure for fuzzy logic.
*/
static struct{
    float fuzzy_temp[FUZZY_RULES_NUM];
    float fuzzy_dtemp[FUZZY_RULES_NUM];
    float fuzzy_pwm[FUZZY_RULES_NUM];
    bool errors[FUZZY_INPUT_NUM][FUZZY_RULES_NUM];
}fuzzy_logic;

/** \brief Creates fuzzy values from crisp input values.
  *
  * \param temp     Temperature crisp input.
  * \param dtemp    Delta temperature crisp input.
  */
static void fuzzyfication_input(int16_t temp, int16_t dtemp){
   uint8_t i;
   for(i=0; i<FUZZY_RULES_NUM; i++){
        if (!rules[i].if_side1)
            fuzzy_logic.fuzzy_temp[i] = 2.0;
        else
            fuzzy_logic.fuzzy_temp[i] = rules[i].if_side1->fuzzyfic_func(rules[i].if_side1, temp);
   }
    for(i=0; i<FUZZY_RULES_NUM; i++){
        if (!rules[i].if_side2)
            fuzzy_logic.fuzzy_dtemp[i] = 3.0;
        else{
            fuzzy_logic.fuzzy_dtemp[i] = rules[i].if_side2->fuzzyfic_func(rules[i].if_side2, dtemp);
       }
   }
}

/** \brief Evaluates fuzzy rules.
  */
static void evaluation_rules(void){
    uint8_t i;
    for(i=0; i<FUZZY_RULES_NUM; i++){
        if (fuzzy_logic.fuzzy_temp[i] == 2.0){
            fuzzy_logic.errors[0][i] = 1;
            fuzzyreg.fuzzy_errors++;
            }
        if (fuzzy_logic.fuzzy_dtemp[i] == 2.0){
            fuzzy_logic.errors[1][i] = 1;
            fuzzyreg.fuzzy_errors++;
            }
        if (fuzzy_logic.fuzzy_dtemp[i] == 3.0){
            fuzzy_logic.fuzzy_pwm[i] = fuzzy_logic.fuzzy_temp[i];
            continue;
        }
        fuzzy_logic.fuzzy_pwm[i] = min(fuzzy_logic.fuzzy_temp[i], fuzzy_logic.fuzzy_dtemp[i]);
    }
}

/** \brief Creates crisp PWM output.
  *
  * \return PWM Duty cycle value.
  */
static uint32_t defuzzyfication(void){
    uint8_t i;
    float sum_maximums =0;
    float sum_wight =0;
    for (i=0; i<FUZZY_RULES_NUM; i++){
        sum_maximums += fuzzy_logic.fuzzy_pwm[i] * rules[i].then_side->maxpoint;
        sum_wight += fuzzy_logic.fuzzy_pwm[i];
    }
    uint32_t res = (sum_maximums / sum_wight);
    return res * PWM_STEP;
}

/*===========================================================================*/
/* Thread local functions                                                    */
/*===========================================================================*/

/** \brief Start routine of regulator.
  *         - Enable pwm channels.
  *         - Clear fuzzy errors and fuzzy logic private area.
  *         - Create log file name.
  *         - Regulator state transaction.
  *
  */
static void startRoutine(void){
    uint8_t i;
    if (fuzzyreg.state == FUZZYREG_STOP){
        for(i=0; i<CHANNEL_NUM; i++){
            pwmEnableChannel(fuzzyreg.heat_ch[i].pwmp, fuzzyreg.heat_ch[i].chnum, fuzzyreg.dutycycle[i]);
            pwmEnablePeriodicNotification(fuzzyreg.heat_ch[i].pwmp);
        }
        bzero(&fuzzy_logic, sizeof(fuzzy_logic));
        fuzzyreg.fuzzy_errors = 0;
        for(i=0; i<CHANNEL_NUM; i++)
            fuzzyreg.start_temp[i] = fuzzyreg.curr_temp.temp[i];
        getDate(&fuzzyreg.starttime);
        uint32_t sec = fuzzyreg.starttime.millisecond / 1000;
        chsnprintf(fuzzyreg.logbuff, sizeof(fuzzyreg.logbuff), "/logs/log%d_%02d_%02d_%02d_%02d_%02d.dat",
                                                fuzzyreg.starttime.year+1980, fuzzyreg.starttime.month, fuzzyreg.starttime.day,
                                                sec/3600,  (sec%3600/60), (sec%3600)%60);
        fuzzyreg.lognum = 0;
        fuzzyreg.state = FUZZYREG_ACTIVE;
        setFuzzyregState(&fuzzyreg.state);
    }
}

/** \brief Stop routine of regulator.
  *         - Disable pwm channels and clear duty cycle.
  *         - Regulator state transaction.
  */
static void stopRoutine(void){
    uint8_t i;
    if (fuzzyreg.state == FUZZYREG_ACTIVE){
        for(i=0; i<CHANNEL_NUM; i++){
            pwmDisableChannel(fuzzyreg.heat_ch[i].pwmp, fuzzyreg.heat_ch[i].chnum);
            pwmDisablePeriodicNotification(fuzzyreg.heat_ch[i].pwmp);
            fuzzyreg.dutycycle[i]=0;
            }
        fuzzyreg.state = FUZZYREG_STOP;
        setFuzzyregState(&fuzzyreg.state);
        displayHeatPower(fuzzyreg.dutycycle);
    }
}
/** \brief Stop routine of regulator.
  *         - Disable pwm channels and clear duty cycle.
  *         - Regulator state transaction.
  */
static void disableRoutine(void){
    uint8_t i;
    for(i=0; i<CHANNEL_NUM; i++){
        pwmDisableChannel(fuzzyreg.heat_ch[i].pwmp, fuzzyreg.heat_ch[i].chnum);
        pwmDisablePeriodicNotification(fuzzyreg.heat_ch[i].pwmp);
        fuzzyreg.dutycycle[i]=0;
    }
    fuzzyreg.state = FUZZYREG_DISABLE;
    setFuzzyregState(&fuzzyreg.state);
    displayHeatPower(fuzzyreg.dutycycle);
}
/** \brief Creates log registration and post it in the flog file buffer.
  *             -if log file buffer is full, sleep regulator sleep time and try again.
  */
static void saveLog(void){
    struct inner_buffer_item *item = getEmptyLogFileBuffer();
    /* Get new empty buffer item */
    while(!item){
        chThdSleepMicroseconds(REGULATOR_SLEEP_TIME_US);
        item = getEmptyLogFileBuffer();
    }
    /* Fill it */
    struct fbuff_item *buffer = (struct fbuff_item*)item->data;
    chsnprintf((char*)buffer->fbuff, FILE_BUFFER_ITEM_SIZE, "%d %3.3f %3.3f %3.3f %1.3f %1.3f %1.3f ", fuzzyreg.lognum++,
        fuzzyreg.curr_temp.temp[0]*SENSOR_TEMP_QUANTUM, fuzzyreg.curr_temp.temp[1]*SENSOR_TEMP_QUANTUM, fuzzyreg.curr_temp.temp[2]*SENSOR_TEMP_QUANTUM,
        fuzzyreg.curr_temp.dtemp[0]*SENSOR_TEMP_QUANTUM, fuzzyreg.curr_temp.dtemp[1]*SENSOR_TEMP_QUANTUM,fuzzyreg.curr_temp.dtemp[2]*SENSOR_TEMP_QUANTUM);
    buffer->element_num = strlen((char*)buffer->fbuff);
    /* Post it */
    postFullLogFileBuffer(item);
    item = getEmptyLogFileBuffer();
    /* Get new empty buffer item */
    while(!item){
        chThdSleepMicroseconds(REGULATOR_SLEEP_TIME_US);
        item = getEmptyLogFileBuffer();
    }
    buffer = (struct fbuff_item*)item->data;
    /* Fill it */
    chsnprintf((char*)buffer->fbuff, FILE_BUFFER_ITEM_SIZE, "%d %d %d\n",fuzzyreg.dutycycle[0], fuzzyreg.dutycycle[1], fuzzyreg.dutycycle[2]);
    buffer->element_num = strlen((char*)buffer->fbuff);
    /* Post it */
    postFullLogFileBuffer(item);
}


/** \brief Regulator thread function.
  *     - Reads temperature form tempFIFO.
  *     - Calculates pwm duty cycle with fuzzy logic.
  */
__attribute__((noreturn))
static THD_FUNCTION(Threadregulator, arg) {
    (void) arg;
    chRegSetThreadName("regulator");
    struct inner_buffer_item *item;
    temperature_t *curr_temp;
    int16_t res;
    uint8_t i;
    /* Disable pwm channels */
    for(i=0; i<CHANNEL_NUM; i++)
        pwmDisableChannel(fuzzyreg.heat_ch[i].pwmp, fuzzyreg.heat_ch[i].chnum);
    displayHeatPower(fuzzyreg.dutycycle);
    while(TRUE) {
        /* Read mailbox */
        chMBFetch(&fuzzyreg_mb, &fuzzyreg.curr_msg, TIME_IMMEDIATE);
        switch(fuzzyreg.curr_msg){
            case FUZZY_REG_START_MSG:   startRoutine();
                                        break;
            case FUZZY_REG_STOP_MSG:   stopRoutine();
                                        break;
            case FUZZY_REG_DISABLE_MSG: disableRoutine();
                                        break;
            default:break;
        }
        fuzzyreg.curr_msg = 0;
        /* Get new temperature form fifo */
        item = getFullInnerBufferItem(&tempFIFO);
        if (item){
            curr_temp = (temperature_t*)item->data;
            chMtxLock(&regmtx);
            for(i=0; i<CHANNEL_NUM; i++){
                fuzzyreg.curr_temp.temp[i] = curr_temp->temp[i];
                fuzzyreg.curr_temp.dtemp[i] = curr_temp->dtemp[i];
            }
            fuzzyreg.curr_temp.is_sterile = curr_temp->is_sterile;
            fuzzyreg.curr_temp.timestamp = curr_temp->timestamp;
            chMtxUnlock(&regmtx);
            /* Put back FIFO item */
            bzero(item, sizeof(temperature_t));
            releaseEmptyInnerBufferItem(&tempFIFO, item);
            switch(fuzzyreg.state){
                case FUZZYREG_ACTIVE:   for(i=0; i<CHANNEL_NUM; i++){
                                            if (fuzzyreg.curr_temp.temp[i] >= CRITICAL_TEMP)
                                                sendErrMail(CRIT_TEMP_ERR_MSG);
                                            /* Calculate PWM duty cycle with fuzzy logic*/
                                            fuzzyfication_input(fuzzyreg.curr_temp.temp[i], fuzzyreg.curr_temp.dtemp[i]);
                                            evaluation_rules();
                                            fuzzyreg.dutycycle[i] = defuzzyfication();
                                            /* Check tg alpha */
                                            if (fuzzyreg.curr_temp.temp[i] < MELTING_END_TEMP && !fuzzyreg.melting_time[i]){
                                                res = fuzzyreg.curr_temp.temp[i] - fuzzyreg.start_temp[i];
                                                if (res >= 64 && !fuzzyreg.idle_time[i]){
                                                        fuzzyreg.idle_time[i] = fuzzyreg.curr_temp.timestamp;
                                                        fuzzyreg.start_temp[i] = fuzzyreg.curr_temp.temp[i];
                                                    }
                                                if (fuzzyreg.idle_time[i]){
                                                    /*The idle time and current time is before midnight */
                                                    if (fuzzyreg.idle_time[i] < fuzzyreg.curr_temp.timestamp)
                                                        fuzzyreg.tg_alpha[i] = ((float)res*SENSOR_TEMP_QUANTUM) / (((float)fuzzyreg.curr_temp.timestamp-(float)fuzzyreg.idle_time[i])/1000);
                                                    /*The idle time is before midnight and current time is after midnight */
                                                    if (fuzzyreg.idle_time[i] > fuzzyreg.curr_temp.timestamp)
                                                        fuzzyreg.tg_alpha[i] = ((float)res*SENSOR_TEMP_QUANTUM)/ ((float)SEC_IN_A_DAY-(float)fuzzyreg.curr_temp.timestamp/1000 + (float)fuzzyreg.idle_time[i]/1000);
                                                    if (fuzzyreg.tg_alpha[i] >= CRITICAL_TG_ALPHA)
                                                        sendErrMail(CRIT_DTEMP_ERR_MSG);
                                                }

                                            }
                                            else{
                                                if (!fuzzyreg.melting_time[i]){
                                                        fuzzyreg.melting_time[i] = fuzzyreg.curr_temp.timestamp;
                                                        fuzzyreg.start_temp[i] = fuzzyreg.curr_temp.temp[i];
                                                    }
                                                else{
                                                    res = fuzzyreg.curr_temp.temp[i] - fuzzyreg.start_temp[i];
                                                    /*The idle time and current time is before midnight */
                                                    if (fuzzyreg.melting_time[i] < fuzzyreg.curr_temp.timestamp)
                                                        fuzzyreg.tg_alpha[i] = ((float)res*SENSOR_TEMP_QUANTUM) / (((float)fuzzyreg.curr_temp.timestamp-(float)fuzzyreg.melting_time[i])/1000);
                                                    /*The idle time is before midnight and current time is after midnight */
                                                    if (fuzzyreg.melting_time[i] > fuzzyreg.curr_temp.timestamp)
                                                        fuzzyreg.tg_alpha[i] = ((float)res*SENSOR_TEMP_QUANTUM)/ ((float)SEC_IN_A_DAY-(float)fuzzyreg.curr_temp.timestamp/1000 + (float)fuzzyreg.melting_time[i]/1000);
                                                    if (fuzzyreg.tg_alpha[i] >= CRITICAL_TG_ALPHA)
                                                        sendErrMail(CRIT_DTEMP_ERR_MSG);
                                                }
                                            }

                                        }
                                        displayHeatPower(fuzzyreg.dutycycle);
                                        fuzzyreg.logfile_error = openLogFile(fuzzyreg.logbuff);
                                        if (!fuzzyreg.logfile_error){
                                            saveLog();
                                            closeLogFile();
                                            }
                                        if (fuzzyreg.fuzzy_errors)
                                            sendErrMail(FUZZY_LOGIC_ERR_MSG);
                default:break;

            }
            /* Show current temp*/
            displayCurrentTemp(fuzzyreg.curr_temp.temp);
        }
        chThdSleepMicroseconds(REGULATOR_SLEEP_TIME_US);

    }
    chThdExit(1);
}


/** \brief Temperature FIFO user interface.
  */
void cmd_tempfifo(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    chprintf(chp, "Temp fifo size: %d fifo item\r\n", innerBufferSize(&tempFIFO));
    chprintf(chp, "Temp fifo free items: %d free item\r\n", innerBufferFreeItem(&tempFIFO));
    chprintf(chp, "Temp fifo full items: %d full item\r\n", innerBufferFullItem(&tempFIFO));
    chprintf(chp, "Temp fifo underflow: %d\r\n", innerBufferUnderflow(&tempFIFO));
    chprintf(chp, "Temp fifo overflow: %d\r\n", innerBufferOverflow(&tempFIFO));
    chprintf(chp, "Temp fifo postoverflow: %d\r\n", innerBufferPostOverflow(&tempFIFO));
    chprintf(chp, "Temp fifo malloc_error: %d\r\n", innerBufferMallocError(&tempFIFO));
    chprintf(chp, "Temp fifo pool_error: %d\r\n", innerBufferPoolError(&tempFIFO));
}

/** \brief Fuzzy logic user interface.
  */
void cmd_fuzzyerror(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    uint8_t i, j;
    chprintf(chp, "Fuzzy error num: %d\r\n", fuzzyreg.fuzzy_errors);
    chprintf(chp, "Fuzzy error code: \r\n");
    for (i=0; i<FUZZY_INPUT_NUM; i++){
        for (j=0; j<FUZZY_RULES_NUM; j++)
            chprintf(chp, "%d ", fuzzy_logic.errors[i][j]);
        chprintf(chp, "\r\n");
        }
}

/** \brief Sends mailbox massage to regulator thread
  *
  * \param msg  Massage code.
  */
void sendMailToRegulator(msg_t msg){
    chMBPost(&fuzzyreg_mb, msg, TIME_INFINITE);
}

/** \brief Sends disable high priority mailbox massage to regulator thread
  *
  * \param msg  Massage code.
  */
void sendDisableMailToRegluator(msg_t msg){
    chMBPostAhead(&fuzzyreg_mb, msg, TIME_INFINITE);
}

/** \brief Get current temperature.
  *
  * \param data     Pointer to temperature object, NULL save.
  */
void getCurrentTemp(temperature_t *data){
    if (!data)
        return;
    uint8_t i;
    chMtxLock(&regmtx);
    for (i=0; i<CHANNEL_NUM; i++){
        data->temp[i] = fuzzyreg.curr_temp.temp[i];
        data->dtemp[i] = fuzzyreg.curr_temp.dtemp[i];
    }
    data->is_sterile = fuzzyreg.curr_temp.is_sterile;
    data->timestamp = fuzzyreg.curr_temp.timestamp;
    chMtxUnlock(&regmtx);
}

/** \brief Initializes regulator
  *         - Temperature FIFO init.
  *         - PWM driver init.
  *         - Creates regulator thread.
  */
void regulatorInit(void){
    innerBufferInit(&tempFIFO, &tempbuffer, tempitems, TEMP_FIFO_SIZE);
    bzero(&fuzzyreg, sizeof(fuzzyreg));
    bzero(&fuzzy_logic, sizeof(fuzzy_logic));
    heat_channel_t channels[CHANNEL_NUM] = HEAT_CHANNELS;
    uint8_t i;
    for(i=0; i<CHANNEL_NUM; i++){
        fuzzyreg.heat_ch[i].pwmp = channels[i].pwmp;
        fuzzyreg.heat_ch[i].chnum = channels[i].chnum;
        pwmStart(fuzzyreg.heat_ch[i].pwmp, &channels_cfg[i]);
    }
    chThdCreateStatic(waThreadregulator, sizeof(waThreadregulator), NORMALPRIO+20, Threadregulator, NULL);
}
