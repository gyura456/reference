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
/** \file printer.h
  * \brief printer handler thread.
  *
  * \author Gyorgy Stercz
  */
#ifndef PRINTER_H_INCLUDED
#define PRINTER_H_INCLUDED

#include <inner_buffer.h>

#define PRINTER_STACK_SIZE 128

#define PRINTER_SLEEP_TIME_US 10000

#define PRINTBUFF_CMD_NAME "printbuff"
#define PRINT_BUFF_CMD {PRINTBUFF_CMD_NAME, cmd_printbuff}

/** \brief Printer buffer item.
  *
  */
struct pbuff_item{
/** Buffer item to printing */
    char pbuff[PRINTER_BUFFER_ITEM_SIZE];
/** Number of bytes in the buffer item */
    uint8_t element_num;
};

/** \brief Get an empty buffer item from printer buffer.
  *
  * \return Pointer to buffer item or NULL, if the printer buffer is full.
  */
struct inner_buffer_item *getEmptyPrinterBuffer(void);

/** \brief Post a filled printer file buffer item.
  *
  * \param item     Pointer of the filled printer buffer item, NULL save.
  */
void postFullPrinterBuffer(struct inner_buffer_item *item);

/** \brief Says, that the printer buffer is full.
  *
  * \return TRUE, if the printer buffer is full, FALSE if not full.
  */
bool isPrinterBufferFull(void);

/** \brief printer user interface
  *
  */
void cmd_printbuff(BaseSequentialStream *chp, int argc, char *argv[]);

/** \brief Initializes printer thread.
  *         - Start Serial driver.
  *         - Printer buffer initialization.
  *         - Creates thread.
  */
void printerInit(void);
#endif // PRINTER_H_INCLUDED
