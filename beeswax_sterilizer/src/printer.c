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
/** \file printer.c
  * \brief  Printer handler thread source.
  */

#include <stdlib.h>
#include <string.h>

#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <appconf.h>
#include <printer.h>

#if PRINTER_STACK_SIZE < 128
    #error Minimum task stack size is 128!
#endif

#if PRINTER_SLEEP_TIME_US < 1
    #error task sleep time must be at least 1
#endif

#if PRINTER_SLEEP_TIME_US < 100
    #warning task sleep time seems to be to few!
#endif

static THD_WORKING_AREA(waThreadprinter, PRINTER_STACK_SIZE);

/*===========================================================================*/
/* Printer buffer                                                            */
/*===========================================================================*/
/** \brief Printer buffer.
  * \{
  */
static struct pbuff_item printerbuffer[PRINTER_BUFFER_SIZE];
static MEMORYPOOL_DECL(printerpool, sizeof(struct pbuff_item), NULL);
static inner_buffer_t printerqueue;
/**\} */


/** \brief Get an empty buffer item from printer buffer.
  *
  * \return Pointer to buffer item or NULL, if the printer buffer is full.
  */
struct inner_buffer_item *getEmptyPrinterBuffer(void){
    return getEmptyInnerBufferItem(&printerqueue);
}

/** \brief Post a filled printer file buffer item.
  *
  * \param item     Pointer of the filled printer buffer item, NULL save.
  */
void postFullPrinterBuffer(struct inner_buffer_item *item){
    postFullInnerBufferItem(&printerqueue, item);
}

/** \brief Says, that the printer buffer is full.
  *
  * \return TRUE, if the printer buffer is full, FALSE if not full.
  */
bool isPrinterBufferFull(void){
    return isInnerBufferFull(&printerqueue);
}

/*===========================================================================*/
/* Serial port config                                                        */
/*===========================================================================*/
/** \brief Serial port configuration.
  */
static const SerialConfig spcfg = {
    9600,
    0,
    0,
    0
};

/*===========================================================================*/
/* Thread function                                                           */
/*===========================================================================*/
/** \brief Printer thread function.
  */
__attribute__((noreturn))
static THD_FUNCTION(Threadprinter, arg) {
    (void) arg;
    chRegSetThreadName("printer");
    struct inner_buffer_item *item;
    struct pbuff_item *buffer;
    while(TRUE) {
        while(!isInnerBufferEmpty(&printerqueue)){
            item = getFullInnerBufferItem(&printerqueue);
            if (!item)
                break;
            buffer = (struct pbuff_item*)item->data;
            sdWrite(&SD6, (uint8_t*)buffer->pbuff, buffer->element_num);
            bzero(buffer->pbuff, PRINTER_BUFFER_ITEM_SIZE);
            releaseEmptyInnerBufferItem(&printerqueue, item);
        }
        chThdSleepMicroseconds(PRINTER_SLEEP_TIME_US);

    }
    chThdExit(1);
}

/*===========================================================================*/
/* Exported functions                                                        */
/*===========================================================================*/
/** \brief printer user interface
  *
  */
void cmd_printbuff(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    chprintf(chp, "Printer buffer size: %d buffer item\r\n", innerBufferSize(&printerqueue));
    chprintf(chp, "Printer buffer item size: %d byte\r\n", PRINTER_BUFFER_ITEM_SIZE);
    chprintf(chp, "Printer buffer free items: %d free item\r\n", innerBufferFreeItem(&printerqueue));
    chprintf(chp, "Printer buffer full items: %d full item\r\n", innerBufferFullItem(&printerqueue));
    chprintf(chp, "Printer buffer underflow: %d\r\n", innerBufferUnderflow(&printerqueue));
    chprintf(chp, "Printer buffer overflow: %d\r\n", innerBufferOverflow(&printerqueue));
    chprintf(chp, "Printer buffer postoverflow: %d\r\n", innerBufferPostOverflow(&printerqueue));
    chprintf(chp, "Printer buffer malloc_error: %d\r\n", innerBufferMallocError(&printerqueue));
    chprintf(chp, "Printer buffer pool_error: %d\r\n", innerBufferPoolError(&printerqueue));
}

/** \brief Initializes printer thread.
  *         - Start Serial driver.
  *         - Printer buffer initialization.
  *         - Creates thread.
  */
void printerInit(void){
    sdStart(&SD6, &spcfg);
    innerBufferInit(&printerqueue, &printerpool, printerbuffer, PRINTER_BUFFER_SIZE);
    chThdCreateStatic(waThreadprinter, sizeof(waThreadprinter), NORMALPRIO, Threadprinter, NULL);
}
