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
/** \file sterilizer.h
  * \brief Mange sterilization, create result file,
  *        print result over printer buffer.
  * \author Gyorgy Stercz
  */
#ifndef STERILIZER_H_INCLUDED
#define STERILIZER_H_INCLUDED

#include <sys/queue.h>
#include <appconf.h>

#define STERILIZER_STACK_SIZE 1024

#define STERILIZER_SLEEP_TIME_US 10000

#define RESULTLIST_CMD_NAME "resultlist"
#define RESULTLIST_CMD {RESULTLIST_CMD_NAME, cmd_resultlist}

/** \brief Enumeration of sterilizer states.
  *
  */
typedef enum{STERILIZER_INIT=0, STERILIZER_STOP, STERILIZER_ACTIVE, STERILIZER_PRINT, STERILIZER_SAVE, STERILIZER_ERROR}sterilizer_state_t;

void sterilizerInit(void);

/** \brief result list user interface
  *
  */
void cmd_resultlist(BaseSequentialStream *chp, int argc, char *argv[]);

/** \brief Sends mailbox massage to sterilizer thread.
  *
  * \param msg Mailbox massage code.
  */
void sendMailtoSterilizer(msg_t msg);

/** \brief Initializes sterilizer
  *         -Result lis init.
  *         -DMA init.
  *         -Creates thread.
  */
void sterilizerInit(void);

#endif // STERILIZER_H_INCLUDED
