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
/** \file cardhandler.h
  * \brief SDCard handler thread.
  *         - handles SD card (with FAT FS)
  *         - handles file write operations from
  *           result and log file buffer
  *         - reads date and time from RTC peripheral
  * \author Gyorgy Stercz
  */
#ifndef CARDHANDLER_H_INCLUDED
#define CARDHANDLER_H_INCLUDED

#include <inner_buffer.h>

#define CARDHANDLER_STACK_SIZE 256

#define CARDHANDLER_SLEEP_TIME_US 10000

#define LOG_BUFFER_CMD_NAME "logfilebuff"
#define LOG_BUFFER_CMD {LOG_BUFFER_CMD_NAME, cmd_logbuff}

#define RESULT_FILE_BUFFER_CMD_NAME "resfilebuff"
#define RESULT_FILE_BUFFER_CMD {RESULT_FILE_BUFFER_CMD_NAME, cmd_resultfilebuff}

#define SDC_CMD_NAME "sdc"
#define SDC_CMD {SDC_CMD_NAME, cmd_sdc}

/** \brief Structure of file buffer item.
  */
struct fbuff_item{
/** Buffer item to file writing */
    uint8_t fbuff[FILE_BUFFER_ITEM_SIZE];
/** number of bytes in the buffer item */
    uint8_t element_num;
};

/** \brief Enumeration of SD Card states
  */
typedef enum{SDC_NOTINSERTED, SDC_ERROR, SDC_BUSY, SDC_READY, SDC_FULL}sdc_state_t;

/** \brief Structure of Human date.
  */
struct HumanDate{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
};


/** \brief Get an empty buffer item from result file buffer.
  *
  * \return Pointer to buffer item or NULL, if the result file buffer is full.
  */
struct inner_buffer_item *getEmptyResultFileBuffer(void);

/** \brief Post a filled result file buffer item.
  *
  * \param item     Pointer of the filled result file buffer item, NULL save.
  */
void postFullResultFileBuffer(struct inner_buffer_item *item);

/** \brief Says, that the result file buffer is full.
  *
  * \return TRUE, if the result file buffer is full, FALSE if not full.
  */
int8_t isResultFileBufferFull(void);

/** \brief Get an empty buffer item from log file buffer.
  *
  * \return Pointer to buffer item or NULL, if the log file buffer is full.
  */
struct inner_buffer_item *getEmptyLogFileBuffer(void);

/** \brief Post a filled log file buffer item.
  *
  * \param item     Pointer of the filled log file buffer item, NULL save.
  */
void postFullLogFileBuffer(struct inner_buffer_item *item);

/** \brief Says, that the log file buffer is full.
  *
  * \return TRUE, if the log file buffer is full, FALSE if not full.
  */
int8_t isLogFileBufferFull(void);

/** \brief Creates new result file, if exist,
  *        will be overwritten.
  * \param filename     Pointer to string of the filename with full path, NULL save.
  * \note  The /results is always the beginning of the string!
  * \return Result of file open operation (see Chan FAT FS ff.h file)
  *         or UCHAR_MAX, if the pointer of the filename string is NULL.
  */
uint8_t openResultFile(const char* filename);

/** \brief Closes result file.
  */
void closeResultFile(void);

/** \brief Creates new log file, if exist,
  *        the data will be appended to the end.
  * \param filename     Pointer to string of the filename with full path, NULL save.
  * \note  The /logs is always the beginning of the string!
  * \return Result of file open operation (see Chan FAT FS ff.h file)
  *         or UCHAR_MAX, if the pointer of the filename string is NULL.
  */
uint8_t openLogFile(const char* filename);

/** \brief Closes log file.
  */
void closeLogFile(void);

/** \brief Creates Date and time string. Format: "yyyy.mm.dd hh:mm:ss".
  *
  * \param str  Pointer to char array of date string, NULL save.
  * \param size Size of char array of date string in size_t unit.
  *             Must be >=25.
  */
void getDateStr(char *str, size_t size);

/** \brief Says date and time in RtCDateTime structure format.
  *
  * \param date Pointer to date structure, NULL save.
  */
void getDate(RTCDateTime *date);

/** \brief Says date and time in millisecond since midnight.
  *
  * \param date Pointer to time variable, NULL save.
  */
void getTime(uint32_t *time);

/** \brief Set RTC date and time from human date and time.
  *
  * \param date     Pointer to Human date structure, NULL save.
  */
void setDate(struct HumanDate *date);

/** \brief Log file buffer user interface, show log file buffer
  *        parameters and errors.
  */
void cmd_logbuff(BaseSequentialStream *chp, int argc, char *argv[]);

/** \brief Result file buffer user interface, show log file buffer
  *        parameters and errors.
  */
void cmd_resultfilebuff(BaseSequentialStream *chp, int argc, char *argv[]);

/** \brief SD Card user interface, show state and free space.
  */
void cmd_sdc(BaseSequentialStream *chp, int argc, char *argv[]);

/** \brief Initializes cardhandler thread.
  *         - Start SDC Driver.
  *         - SDC monitor timer init.
  *         - Result and log file buffer init.
  *         - Creates cardhandler thread.
  *
  */
void cardhandlerInit(void);

#endif // CARDHANDLER_H_INCLUDED
