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
/** \file sterilizer.c
  * \brief Sterilizer thread source.
  */

#include <stdlib.h>
#include <string.h>

#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <sterilizer.h>
#include <tempreader.h>
#include <errorhandler.h>
#include <cardhandler.h>
#include <regulator.h>
#include <lcdcontrol.h>
#include <printer.h>

#if STERILIZER_STACK_SIZE < 128
    #error Minimum task stack size is 128!
#endif

#if STERILIZER_SLEEP_TIME_US < 1
    #error task sleep time must be at least 1
#endif

#if STERILIZER_SLEEP_TIME_US < 100
    #warning task sleep time seems to be to few!
#endif

static THD_WORKING_AREA(waThreadsterilizer, STERILIZER_STACK_SIZE);
static MUTEX_DECL(smtx);

/*===========================================================================*/
/* Sterilizer thread data                                                    */
/*===========================================================================*/

/** \brief Structure for thread data.
  */
static struct{
    msg_t mb_buff[STERILIZER_MAILBOX_SIZE];
    msg_t curr_massage;
    sterilizer_state_t state;
    temperature_t curr_temp;
    thread_reference_t thread;
    const stm32_dma_stream_t *dma;
    uint32_t dmamode;
    uint8_t num_of_swing;
}sterilizer;

static MAILBOX_DECL(steril_mb, sterilizer.mb_buff, STERILIZER_MAILBOX_SIZE);

/*===========================================================================*/
/* Result list                                                               */
/*===========================================================================*/

/** \brief Result list item
  */
struct result_item{
    STAILQ_ENTRY(result_item) entries;
    char str[FILE_BUFFER_ITEM_SIZE];
};

/** \brief Memory pool of result list
  *
  * \{
  */
static struct result_item resultitems[RESULT_LIST_SIZE];
static MEMORYPOOL_DECL(resultlist, sizeof(struct result_item), NULL);
/**\} */

/** \brief Result list privet area.
  */
static struct{
    STAILQ_HEAD(resulthead, result_item) head;
    struct resulthead *headp;
    uint8_t itemnum;
    uint8_t freeitem;
    uint8_t underflow;
    systime_t savetime;
    RTCDateTime starttime;
    uint32_t endtime;
    bool finalresult;
    uint8_t file_error;
}result;

/** \brief Initializes result list.
  */
static void resultListInit(void){
    bzero(&result, sizeof(result));
    STAILQ_INIT(&result.head);
    chPoolLoadArray(&resultlist, resultitems, RESULT_LIST_SIZE);
    result.freeitem = RESULT_LIST_SIZE;
}

/** \brief Creates item string from temperature object and put it to result list.
  *
  * \param data     Pointer to temperature_t object, NULL save.
  */
void putResultToList(temperature_t *data){
    if (!data)
        return;
    char *status;
    chMtxLock(&smtx);
    struct result_item *item = chPoolAlloc(&resultlist);
    result.freeitem--;
    chMtxUnlock(&smtx);
    if (item){
        if (data->is_sterile)
            status = "Sterile\n";
        else
            status = "Failure\n";
        uint32_t sec = data->timestamp / 1000;
        chsnprintf(item->str, sizeof(item->str), "%02d\t%02d:%02d:%02d\t%3.1f C\t%3.1f C\t%3.1f C\t", result.itemnum, sec/3600,  (sec%3600/60), ((sec%3600)%60),
                                                data->temp[0]*SENSOR_TEMP_QUANTUM, data->temp[1]*SENSOR_TEMP_QUANTUM, data->temp[2]*SENSOR_TEMP_QUANTUM);
        strcat(item->str, status);
        STAILQ_INSERT_TAIL(&result.head, item, entries);
        result.itemnum++;
    }
    else{
        chMtxLock(&smtx);
        result.underflow++;
        chMtxUnlock(&smtx);
    }
    displayResultListItem(item->str);
}

/** \brief Deleted all result list item.
  */
void freeResultList(void){
    if (STAILQ_EMPTY(&result.head))
        return;
    chMtxLock(&smtx);
    struct result_item *head = STAILQ_FIRST(&result.head);
    while(head){
        STAILQ_REMOVE_HEAD(&result.head, entries);
        result.itemnum--;
        chPoolFree(&resultlist, head);
        result.freeitem++;
        head = STAILQ_FIRST(&result.head);
    }
    chMtxUnlock(&smtx);
    destroyDisplayedResultList();
}


/*===========================================================================*/
/* Tempreader thread local functions.                                        */
/*===========================================================================*/

/** \brief DMA transfer end callback function, wakes up sterilizer thread.
  */
static void wake_up_sterilizer(thread_reference_t *thread, uint32_t flags){
    if (flags & STM32_DMA_ISR_TCIF){
        chSysLockFromISR();
        chThdResumeI(thread, MSG_OK);
        chSysUnlockFromISR();
    }
}

/** \brief Copied data into buffer item memory area.
  *
  * \param src  Pointer to source, NULL save.
  * \param dest Pointer to destination, NULL save.
  * \param size Size of copied data in byte, can not 0.
  * \note Size must be less than or equal to the buffer size.
  * \return MSG_RESET if pointer to src or dest is NULL, or size is 0.
  */
static inline msg_t dmaFillBuffer(void *src, void *dest, size_t size){
    if (!(src&&dest&&size))
        return MSG_RESET;
    dmaStartMemCopy(sterilizer.dma, sterilizer.dmamode, src, dest, size);
    chSysLock();
    return chThdSuspendS(&sterilizer.thread);
    chSysUnlock();
}

/** \brief Saves string into SD card over file buffer with DMA controller.
  *
  * \param str  Pointer to string, NULL save.
  * \param size Lenght of string, can not greater than FILE_BUFFER_ITEM_SIZE.
  */
static void saveString(char *str, size_t size){
    if (!str)
        return;
    if (size > FILE_BUFFER_ITEM_SIZE)
        return;
    struct inner_buffer_item *item = getEmptyResultFileBuffer();
    while(!item){
        chThdSleepMicroseconds(STERILIZER_SLEEP_TIME_US);
        item = getEmptyResultFileBuffer();
    }
    struct fbuff_item *buffer = (struct fbuff_item*)item->data;
    buffer->element_num = size;
    dmaFillBuffer(str, buffer->fbuff, buffer->element_num);
    postFullResultFileBuffer(item);

}

/** \brief Prints string over printer buffer with DMA controller.
  *
  * \param str  Pointer to string, NULL save.
  * \param size Lenght of string, can not greater than FILE_BUFFER_ITEM_SIZE.
  */
static void printString(char *str, size_t size){
    if (!str)
        return;
    if (size > PRINTER_BUFFER_ITEM_SIZE)
        return;
    struct inner_buffer_item *item = getEmptyPrinterBuffer();
    while(!item){
        chThdSleepMicroseconds(STERILIZER_SLEEP_TIME_US);
        item = getEmptyPrinterBuffer();
    }
    struct pbuff_item *buffer = (struct pbuff_item*)item->data;
    buffer->element_num = size;
    dmaFillBuffer(str, buffer->pbuff, buffer->element_num);
    postFullPrinterBuffer(item);

}

/** \brief Start routine of sterilizing.
  *         - Clear result list.
  *         - Get start Date and time.
  *         - Start fuzzy regulator.
  *         - Sterilizer state transaction.
  */
static void startRoutine(void){
    if (sterilizer.state == STERILIZER_STOP){
        if (!STAILQ_EMPTY(&result.head))
            freeResultList();
        getDate(&result.starttime);
        displayResultStart(&result.starttime);
        result.finalresult = FALSE;
        sendMailToRegulator(FUZZY_REG_START_MSG);
        result.savetime = chVTGetSystemTime();
        sterilizer.state = STERILIZER_ACTIVE;
        sterilizer.num_of_swing = 0;
        displaySterilizerState(&sterilizer.state);
    }
}

/** \brief Start routine of sterilizing.
  *         - Stop Fuzzy regulator.
  *         - Get sterilization end time.
  *         - Sterilizer state transaction.
  */
static void stopRoutine(void){
    if (sterilizer.state == STERILIZER_ACTIVE){
        sendMailToRegulator(FUZZY_REG_STOP_MSG);
        getTime(&result.endtime);
        displayResultEnd(&result.endtime, result.finalresult);
        sterilizer.state = STERILIZER_SAVE;
        displaySterilizerState(&sterilizer.state);
    }
}
/** \brief Start routine of sterilizing.
  *         - Sterilizer state transaction.
  */
static void stopErrorRoutine(void){
    sterilizer.state = STERILIZER_ERROR;
    displaySterilizerState(&sterilizer.state);
}


/** \brief Sterilizer thread function
  *         - Checks temperature values.
  *         - Manages sterilization result list.
  *         - Saves and prints result list.
  */
__attribute__((noreturn))
static THD_FUNCTION(Threadsterilizer, arg) {
    (void) arg;
    chRegSetThreadName("sterilizer");
    uint32_t sec;
    struct result_item *listhead;
    char linebuff[50];
    displaySterilizerState(&sterilizer.state);
    systime_t curr_time;
    while(TRUE) {
        /* read mailbox massages */
        chMBFetch(&steril_mb, &sterilizer.curr_massage, TIME_IMMEDIATE);
        switch(sterilizer.curr_massage){
            case SENSOR_INIT_END:       if (sterilizer.state == STERILIZER_INIT){
                                            sterilizer.state = STERILIZER_STOP;
                                            displaySterilizerState(&sterilizer.state);
                                            }
                                        break;
            case START_STERILIZER:      startRoutine();
                                        break;
            case STOP_STERILZER:        stopRoutine();
                                        break;
            case STOPERROR_STERILIZER:  stopErrorRoutine();
                                        break;
            case PRINT_RESULT_LIST:     if (sterilizer.state == STERILIZER_STOP){
                                            sterilizer.state = STERILIZER_PRINT;
                                            displaySterilizerState(&sterilizer.state);
                                        }
            default:                    break;
        }
        sterilizer.curr_massage = 0;
        switch(sterilizer.state){
            /* Put temp into result list, if it save time */
            case STERILIZER_ACTIVE:     curr_time = chVTGetSystemTime();
                                        if ( curr_time >= result.savetime){
                                            getCurrentTemp(&sterilizer.curr_temp);
                                            if (sterilizer.curr_temp.is_sterile){
                                                putResultToList(&sterilizer.curr_temp);
                                                result.savetime = curr_time + S2ST(STERLIZER_SAVE_INTERVAL_S);
                                                break;
                                                }
                                            if (!STAILQ_EMPTY(&result.head)){
                                                if (sterilizer.num_of_swing <= NUM_OF_TEMP_SWING){
                                                    sterilizer.num_of_swing++;
                                                    freeResultList();
                                                    result.savetime = curr_time + S2ST(STERLIZER_SAVE_INTERVAL_S);
                                                    break;
                                                }
                                                putResultToList(&sterilizer.curr_temp);
                                                result.finalresult = FALSE;
                                                stopRoutine();
                                                break;
                                            }
                                            result.savetime = curr_time + S2ST(STERLIZER_SAVE_INTERVAL_S);
                                        }
                                        if (result.itemnum == RESULT_LIST_SIZE){
                                            result.finalresult = TRUE;
                                            stopRoutine();
                                        }
                                        break;
            case STERILIZER_SAVE:    if (!STAILQ_EMPTY(&result.head)){
                                            sec = result.starttime.millisecond / 1000;
                                            chsnprintf(linebuff, sizeof(linebuff), "/results/%d_%02d_%02d_%02d_%02d_%02d.txt",
                                                result.starttime.year+1980, result.starttime.month, result.starttime.day,
                                                sec/3600,  (sec%3600/60), (sec%3600)%60);

                                            result.file_error = openResultFile(linebuff);
                                            if (result.file_error){
                                                 sterilizer.state = STERILIZER_STOP;
                                                 displaySterilizerState(&sterilizer.state);
                                                 switchToresultPage();
                                                 break;
                                            }

                                            chsnprintf(linebuff, sizeof(linebuff), "Date: %d.%02d.%02d\nStart: %02d:%02d:%02d\n",
                                                        result.starttime.year+1980, result.starttime.month, result.starttime.day,
                                                        sec/3600,  (sec%3600/60), (sec%3600)%60);
                                            saveString(linebuff, strlen(linebuff));

                                            chsnprintf(linebuff, sizeof(linebuff), "Nr.\tTime\t\tCH0\tCH1\tCH2\tStatus\n");
                                            saveString(linebuff, strlen(linebuff));

                                            listhead = STAILQ_FIRST(&result.head);
                                            while(listhead){
                                                saveString(listhead->str, strlen(listhead->str));
                                                listhead = listhead->entries.stqe_next;
                                                }
                                            sec = result.endtime / 1000;
                                            if (result.finalresult)
                                                chsnprintf(linebuff, sizeof(linebuff), "End: %02d:%02d:%02d\nResult: SUCCESS\n",
                                                                            sec/3600,  (sec%3600/60), (sec%3600)%60);
                                            else
                                                chsnprintf(linebuff, sizeof(linebuff), "End: %02d:%02d:%02d\nResult: FAILURE\n",
                                                                            sec/3600,  (sec%3600/60), (sec%3600)%60);
                                            saveString(linebuff, strlen(linebuff));
                                            closeResultFile();
                                            }
                                            sterilizer.state = STERILIZER_STOP;
                                            displaySterilizerState(&sterilizer.state);
                                            switchToresultPage();
                                            break;
            case STERILIZER_PRINT:       if (!STAILQ_EMPTY(&result.head)){
                                            sec = result.starttime.millisecond / 1000;
                                            chsnprintf(linebuff, sizeof(linebuff), "\n\nDate: %d.%02d.%02d\nStart: %02d:%02d:%02d\n",
                                                                result.starttime.year+1980, result.starttime.month, result.starttime.day,
                                                                sec/3600,  (sec%3600/60), (sec%3600)%60);
                                            printString(linebuff, strlen(linebuff));

                                            chsnprintf(linebuff, sizeof(linebuff), "Nr.\tTime\t\tCH0\tCH1\tCH2\tStatus\n");
                                            printString(linebuff, strlen(linebuff));

                                            listhead = STAILQ_FIRST(&result.head);
                                            while(listhead){
                                                printString(listhead->str, strlen(listhead->str));
                                                listhead = listhead->entries.stqe_next;
                                            }
                                            sec = result.endtime / 1000;
                                            if (result.finalresult)
                                                chsnprintf(linebuff, sizeof(linebuff), "End: %02d:%02d:%02d\nResult: SUCCESS\n",
                                                                            sec/3600,  (sec%3600/60), (sec%3600)%60);
                                            else
                                                chsnprintf(linebuff, sizeof(linebuff), "End: %02d:%02d:%02d\nResult: FAILURE\n",
                                                                            sec/3600,  (sec%3600/60), (sec%3600)%60);
                                            printString(linebuff, strlen(linebuff));
                                            }
                                            sterilizer.state = STERILIZER_STOP;
                                            displaySterilizerState(&sterilizer.state);
            default:                        break;
        }
        chThdSleepMicroseconds(STERILIZER_SLEEP_TIME_US);

    }
    chThdExit(1);
}


/** \brief result list user interface
  *
  */
void cmd_resultlist(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    chMtxLock(&smtx);
    uint8_t freeitem = result.freeitem;
    uint8_t itemnum = result.itemnum;
    uint8_t underflow = result.underflow;
    chMtxUnlock(&smtx);
    chprintf(chp, "Result list size: %d\r\n" , RESULT_LIST_SIZE);
    chprintf(chp, "Free items: %d\r\n" , freeitem);
    chprintf(chp, "Item num: %d\r\n" , itemnum);
    chprintf(chp, "Result list underflow: %d\r\n" , underflow);
}

/** \brief Sends mailbox massage to sterilizer thread.
  *
  * \param msg Mailbox massage code.
  */
void sendMailtoSterilizer(msg_t msg){
    chMBPost(&steril_mb, msg, TIME_INFINITE);
}

/** \brief Initializes sterilizer
  *         - Result lis init.
  *         - DMA init.
  *         - Creates thread.
  */
void sterilizerInit(void){
    resultListInit();
    bzero(&sterilizer, sizeof(sterilizer));
    sterilizer.dma = STM32_DMA_STREAM(STERILIZER_DMA_STREAM);
    sterilizer.dmamode = STM32_DMA_CR_CHSEL(STERILIZER_DMA_CHANNEL) | STM32_DMA_CR_PL(STERILIZER_DMA_PRIORITY) | STM32_DMA_CR_TCIE;
    dmaStreamAllocate(sterilizer.dma, STERILIZER_DMA_IRQ_PRIORITY, (stm32_dmaisr_t)wake_up_sterilizer, &sterilizer.thread);
    chThdCreateStatic(waThreadsterilizer, sizeof(waThreadsterilizer), NORMALPRIO+20, Threadsterilizer, NULL);

}
