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
/** \file cardhandler.c
  * \brief SD card and RTC handler source.
  */

#include <stdlib.h>
#include <string.h>

#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <limits.h>
#include <ff.h>
#include <appconf.h>
#include <cardhandler.h>
#include <lcdcontrol.h>

#if CARDHANDLER_STACK_SIZE < 128
    #error Minimum task stack size is 128!
#endif

#if CARDHANDLER_SLEEP_TIME_US < 1
    #error task sleep time must be at least 1
#endif

#if CARDHANDLER_SLEEP_TIME_US < 100
    #warning task sleep time seems to be to few!
#endif


static THD_WORKING_AREA(waThreadcardhandler, CARDHANDLER_STACK_SIZE);
static MUTEX_DECL(chrmtx);

/*===========================================================================*/
/* File buffers to write operations.                                         */
/*===========================================================================*/

/** \brief Result file buffer.
  * \{
  */
static struct fbuff_item resfilebuff[FILE_BUFFER_SIZE];
static MEMORYPOOL_DECL(resfilepool, sizeof(struct fbuff_item), NULL);
static inner_buffer_t resfilequeue;
/**\} */

/** \brief Get an empty buffer item from result file buffer.
  *
  * \return Pointer to buffer item or NULL, if the result file buffer is full.
  */
struct inner_buffer_item *getEmptyResultFileBuffer(void){
    return getEmptyInnerBufferItem(&resfilequeue);
}

/** \brief Post a filled result file buffer item.
  *
  * \param item     Pointer of the filled result file buffer item, NULL save.
  */
void postFullResultFileBuffer(struct inner_buffer_item *item){
    postFullInnerBufferItem(&resfilequeue, item);
}
/** \brief Says, that the result file buffer is full.
  *
  * \return TRUE, if the result file buffer is full, FALSE if not full.
  */
int8_t isResultFileBufferFull(void){
    return isInnerBufferFull(&resfilequeue);
}

/** \brief Log file buffer.
  *
  * \{
  */
static struct fbuff_item logfilebuff[FILE_BUFFER_SIZE];
static MEMORYPOOL_DECL(logfilepool, sizeof(struct fbuff_item), NULL);
static inner_buffer_t logfilequeue;
/**\} */

/** \brief Get an empty buffer item from log file buffer.
  *
  * \return Pointer to buffer item or NULL, if the log file buffer is full.
  */
struct inner_buffer_item *getEmptyLogFileBuffer(void){
    return getEmptyInnerBufferItem(&logfilequeue);
}

/** \brief Post a filled log file buffer item.
  *
  * \param item     Pointer of the filled log file buffer item, NULL save.
  */
void postFullLogFileBuffer(struct inner_buffer_item *item){
    postFullInnerBufferItem(&logfilequeue, item);
}
/** \brief Says, that the log file buffer is full.
  *
  * \return TRUE, if the log file buffer is full, FALSE if not full.
  */
int8_t isLogFileBufferFull(void){
    return isInnerBufferFull(&logfilequeue);
}

/*===========================================================================*/
/* Cardhandler data                                                          */
/*===========================================================================*/

/** \brief Structure for thread data.
  */
static struct{
    bool fs_ready;
    FATFS SDC_FS;
    uint64_t freespace;
    sdc_state_t state;
    RTCDateTime rtctime;
}cardhandler;

/** \brief Structure of result file.
  */
static struct{
    bool isopen;
    bool close;
    FIL file;
    FRESULT fr;
    UINT bw;
}resultfile;

/** \brief Structure of log file.
  */
static struct{
    bool isopen;
    bool close;
    FIL file;
    FRESULT fr;
    UINT bw;
}logfile;

/*===========================================================================*/
/* Card monitor                                                              */
/*===========================================================================*/
/** \brief   Card monitor timer.
  */
static virtual_timer_t tmr;

/** \brief   Debounce counter.
  */
static unsigned cnt;

/** \brief   Card event sources.
  */
static event_source_t inserted_event, removed_event;

/** \brief   Insertion monitor timer callback function.
  *
  * \param p         pointer to the @p BaseBlockDevice object
  */
static void tmrfunc(void *p) {
  BaseBlockDevice *bbdp = p;

  chSysLockFromISR();
  if (cnt > 0) {
    if (blkIsInserted(bbdp)) {
      if (--cnt == 0) {
        chEvtBroadcastI(&inserted_event);
      }
    }
    else
      cnt = SDC_POLLING_INTERVAL;
  }
  else {
    if (!blkIsInserted(bbdp)) {
      cnt = SDC_POLLING_INTERVAL;
      chEvtBroadcastI(&removed_event);
    }
  }
  chVTSetI(&tmr, MS2ST(SDC_POLLING_DELAY_MS), tmrfunc, bbdp);
  chSysUnlockFromISR();
}

/**\brief   Polling monitor start.
  *
  * \param p         pointer to an object implementing @p BaseBlockDevice
  */
static void tmr_init(void *p) {
  chEvtObjectInit(&inserted_event);
  chEvtObjectInit(&removed_event);
  chSysLock();
  cnt = SDC_POLLING_INTERVAL;
  chVTSetI(&tmr, MS2ST(SDC_POLLING_DELAY_MS), tmrfunc, p);
  chSysUnlock();
}

/** \brief Card insertion event.
  *
  */
static void InsertHandler(eventid_t id) {
  (void)id;
    FRESULT err;
    FATFS *fsp;
    uint32_t clusters;
  /*
   * On insertion SDC initialization and FS mount.
   */
    if (sdcConnect(&SDCD1))
        return;
    err = f_mount(&cardhandler.SDC_FS, "/", 1);
    if (err != FR_OK){
        sdcDisconnect(&SDCD1);
        cardhandler.state = SDC_ERROR;
        displaySdcState(&cardhandler.state);
        return;
    }
    err = f_getfree("/", &clusters, &fsp);
        if(err != FR_OK){
            sdcDisconnect(&SDCD1);
            cardhandler.state = SDC_ERROR;
            displaySdcState(&cardhandler.state);
            return;
        }
    cardhandler.freespace = clusters*(uint32_t)cardhandler.SDC_FS.csize*(uint32_t)MMCSD_BLOCK_SIZE;
    if (!cardhandler.freespace){
        cardhandler.state = SDC_FULL;
        displaySdcState(&cardhandler.state);
        return;
        }
    cardhandler.fs_ready = TRUE;
    cardhandler.state = SDC_READY;
    displaySdcState(&cardhandler.state);
}

/** \brief Card removal event.
  */
static void RemoveHandler(eventid_t id) {
    (void)id;
    sdcDisconnect(&SDCD1);
    cardhandler.fs_ready = FALSE;
    cardhandler.state = SDC_NOTINSERTED;
    displaySdcState(&cardhandler.state);
}

/*===========================================================================*/
/* Thread function.                                                          */
/*===========================================================================*/
/** \brief Cradhandler thread function.
  *         - Update date and tme form RTC.
  *         - Write data for file buffer into the result and log file.
  */
__attribute__((noreturn))
static THD_FUNCTION(Threadcardhandler, arg) {
    (void) arg;
    chRegSetThreadName("cardhandler");
    struct inner_buffer_item *item;
    struct fbuff_item *buffer;
    static const evhandler_t evhndl[] = {
        InsertHandler,
        RemoveHandler
    };
    event_listener_t el0, el1;
    chEvtRegister(&inserted_event, &el0, 0);
    chEvtRegister(&removed_event, &el1, 1);
    displaySdcState(&cardhandler.state);
    while(TRUE) {
        /* Read date and time from RTC*/
        rtcGetTime(&RTCD1, &cardhandler.rtctime);
        /* Wait for SDC event with timeout */
        chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, US2ST(CARDHANDLER_SLEEP_TIME_US)));
        if (cardhandler.state == SDC_READY || cardhandler.state == SDC_BUSY){
            /* Write an item form result file buffer into the result file */
            if (resultfile.isopen){
                if (cardhandler.state != SDC_BUSY){
                    cardhandler.state = SDC_BUSY;
                    displaySdcState(&cardhandler.state);
                }
                if(!isInnerBufferEmpty(&resfilequeue)){
                    item = getFullInnerBufferItem(&resfilequeue);
                    if (item){
                        buffer = (struct fbuff_item*)item->data;
                        resultfile.fr = f_write(&resultfile.file, buffer->fbuff, buffer->element_num, &resultfile.bw);
                        bzero(buffer->fbuff, FILE_BUFFER_ITEM_SIZE);
                        releaseEmptyInnerBufferItem(&resfilequeue, item);
                    }
                }
            }
            /* Close result file, if the buffer is empty */
            if (resultfile.close && isInnerBufferEmpty(&resfilequeue)){
                    resultfile.fr = f_close(&resultfile.file);
                    chMtxLock(&chrmtx);
                    resultfile.isopen = 0;
                    resultfile.close = 0;
                    chMtxUnlock(&chrmtx);
                    cardhandler.state = SDC_READY;
                    displaySdcState(&cardhandler.state);
            }
            /* Write an item form log file buffer into the log file */
            if (logfile.isopen){
                if (cardhandler.state != SDC_BUSY){
                    cardhandler.state = SDC_BUSY;
                    displaySdcState(&cardhandler.state);
                }
                if(!isInnerBufferEmpty(&logfilequeue)){
                    item = getFullInnerBufferItem(&logfilequeue);
                    if (item){
                        buffer = (struct fbuff_item*)item->data;
                        logfile.fr = f_write(&logfile.file, buffer->fbuff, buffer->element_num, &logfile.bw);
                        bzero(buffer->fbuff, FILE_BUFFER_ITEM_SIZE);
                        releaseEmptyInnerBufferItem(&logfilequeue, item);
                    }
                }
            }
            /* Close log file, if the buffer is empty */
            if (logfile.close && isInnerBufferEmpty(&logfilequeue)){
                logfile.fr = f_close(&logfile.file);
                chMtxLock(&chrmtx);
                logfile.isopen = 0;
                logfile.close = 0;
                chMtxUnlock(&chrmtx);
                cardhandler.state = SDC_READY;
                displaySdcState(&cardhandler.state);
            }
        }
    }
    chThdExit(1);
}

/*===========================================================================*/
/* Exported functions.                                                       */
/*===========================================================================*/
/** \brief Creates new result file, if exist,
  *        will be overwritten.
  * \param filename     Pointer to string of the filename with full path, NULL save.
  * \note  The /results is always the beginning of the string!
  * \return Result of file open operation (see Chan FAT FS ff.h file)
  *         or UCHAR_MAX, if the pointer of the filename string is NULL.
  */
uint8_t openResultFile(const char* filename){
    if (!filename)
        return UCHAR_MAX;
    f_mkdir("/results");
    chMtxLock(&chrmtx);
    resultfile.fr = f_open(&resultfile.file, filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (!resultfile.fr){
        resultfile.isopen = 1;
        resultfile.close = 0;
        }
    chMtxUnlock(&chrmtx);
    return (uint8_t)resultfile.fr;
}

/** \brief Closes result file.
  */
void closeResultFile(void){
    chMtxLock(&chrmtx);
    resultfile.close = 1;
    chMtxUnlock(&chrmtx);
}

/** \brief Creates new log file, if exist,
  *        the data will be appended to the end.
  * \param filename     Pointer to string of the filename with full path, NULL save.
  * \note  The /logs is always the beginning of the string!
  * \return Result of file open operation (see Chan FAT FS ff.h file)
  *         or UCHAR_MAX, if the pointer of the filename string is NULL.
  */
uint8_t openLogFile(const char* filename){
    if (!filename)
        return UCHAR_MAX;
    f_mkdir("/logs");
    chMtxLock(&chrmtx);
    logfile.fr = f_open(&logfile.file, filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (!logfile.fr){
        logfile.isopen = 1;
        logfile.close = 0;
        f_lseek(&logfile.file, f_size(&logfile.file));
    }
    chMtxUnlock(&chrmtx);
    return (uint8_t)logfile.fr;
}

/** \brief Closes log file.
  */
void closeLogFile(void){
    chMtxLock(&chrmtx);
    logfile.close = 1;
    chMtxUnlock(&chrmtx);
}

/** \brief Createsss Date and time string. Format: "yyyy.mm.dd hh:mm:ss".
  *
  * \param str  Pointer to char array of date string, NULL save.
  * \param size Size of char array of date string in size_t unit.
  *             Must be >=25.
  */
void getDateStr(char *str, size_t size){
    if (!str || size < 25)
        return;
    chMtxLock(&chrmtx);
    RTCDateTime rtctime = cardhandler.rtctime;
    chMtxUnlock(&chrmtx);
    uint32_t sec = rtctime.millisecond / 1000;
    chsnprintf(str, size, "%4d.%02d.%02d. %02d:%02d:%02d", rtctime.year+1980, rtctime.month, rtctime.day,
                                                sec/3600,  (sec%3600/60), (sec%3600)%60);
}

/** \brief Says date and time in RtCDateTime structure format.
  *
  * \param date Pointer to date structure, NULL save.
  */
void getDate(RTCDateTime *date){
    if (!date)
        return;
    chMtxLock(&chrmtx);
    *date = cardhandler.rtctime;
    chMtxUnlock(&chrmtx);
}

/** \brief Says date and time in millisecond since midnight.
  *
  * \param date Pointer to time variable, NULL save.
  */
void getTime(uint32_t *time){
    if (!time)
        return;
    chMtxLock(&chrmtx);
    *time = cardhandler.rtctime.millisecond;
    chMtxUnlock(&chrmtx);
}

/** \brief Set RTC date and time from human date and time.
  *
  * \param date     Pointer to Human date structure, NULL save.
  */
void setDate(struct HumanDate *date){
    RTCDateTime rtctime;
    chMtxLock(&chrmtx);
    rtctime.year = date->year-1980;
    rtctime.month = date->month;
    rtctime.day = date->day;
    rtctime.millisecond = date->hour*3600;
    rtctime.millisecond += date->min*60;
    rtctime.millisecond += date->sec;
    rtctime.millisecond *= 1000;
    chMtxUnlock(&chrmtx);
    rtcSetTime(&RTCD1, &rtctime);
};


/** \brief Log file buffer user interface, show log file buffer
  *        parameters and errors.
  */
void cmd_logbuff(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    chprintf(chp, "Log file buffer size: %d buffer item\r\n", innerBufferSize(&logfilequeue));
    chprintf(chp, "Log file buffer item size: %d byte\r\n", FILE_BUFFER_ITEM_SIZE);
    chprintf(chp, "Log file buffer free items: %d free item\r\n", innerBufferFreeItem(&logfilequeue));
    chprintf(chp, "Log file buffer full items: %d full item\r\n", innerBufferFullItem(&logfilequeue));
    chprintf(chp, "Log file buffer underflow: %d\r\n", innerBufferUnderflow(&logfilequeue));
    chprintf(chp, "Log file buffer overflow: %d\r\n", innerBufferOverflow(&logfilequeue));
    chprintf(chp, "Log file buffer postoverflow: %d\r\n", innerBufferPostOverflow(&logfilequeue));
    chprintf(chp, "Log file buffer malloc_error: %d\r\n", innerBufferMallocError(&logfilequeue));
    chprintf(chp, "Log file buffer pool_error: %d\r\n", innerBufferMallocError(&logfilequeue));
}

/** \brief Result file buffer user interface, show log file buffer
  *        parameters and errors.
  */
void cmd_resultfilebuff(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    chprintf(chp, "Result file buffer size: %d buffer item\r\n", innerBufferSize(&resfilequeue));
    chprintf(chp, "Result file buffer item size: %d byte\r\n", FILE_BUFFER_ITEM_SIZE);
    chprintf(chp, "Result file buffer free items: %d free item\r\n", innerBufferFreeItem(&resfilequeue));
    chprintf(chp, "Result file buffer full items: %d full item\r\n", innerBufferFullItem(&resfilequeue));
    chprintf(chp, "Result file buffer underflow: %d\r\n", innerBufferUnderflow(&resfilequeue));
    chprintf(chp, "Result file buffer overflow: %d\r\n", innerBufferOverflow(&resfilequeue));
    chprintf(chp, "Result file buffer postoverflow: %d\r\n", innerBufferPostOverflow(&resfilequeue));
    chprintf(chp, "Result file buffer malloc_error: %d\r\n", innerBufferMallocError(&resfilequeue));
    chprintf(chp, "Result file buffer pool_error: %d\r\n", innerBufferPoolError(&resfilequeue));
}

/** \brief SD Card user interface, show state and free space.
  */
void cmd_sdc(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    chMtxLock(&chrmtx);
    sdc_state_t state = cardhandler.state;
    uint64_t freespace = cardhandler.freespace;
    chMtxUnlock(&chrmtx);
    switch(state){
        case SDC_NOTINSERTED:        chprintf(chp, "SD Card not inserted\r\n");
                                   break;
        case SDC_ERROR:            chprintf(chp, "SD Card error\r\n");
                                   break;
        case SDC_BUSY:             chprintf(chp, "SD Card busy\r\n");
                                   break;
        case SDC_READY:            chprintf(chp, "SD Card ready\r\n");
                                   break;
        case SDC_FULL:             chprintf(chp, "SD Card full\r\n");
    }
    chprintf(chp, "Free space: %ld byte\r\n", freespace);
}


/** \brief Initializes cardhandler thread.
  *         - Start SDC Driver.
  *         - SDC monitor timer init.
  *         - Result and log file buffer init.
  *         - Creates cardhandler thread.
  *
  */
void cardhandlerInit(void){
    sdcStart(&SDCD1, NULL);
    tmr_init(&SDCD1);
    innerBufferInit(&resfilequeue, &resfilepool, resfilebuff, FILE_BUFFER_SIZE);
    innerBufferInit(&logfilequeue, &logfilepool, logfilebuff, FILE_BUFFER_SIZE);
    chThdCreateStatic(waThreadcardhandler, sizeof(waThreadcardhandler), NORMALPRIO, Threadcardhandler, NULL);
}
