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
/** \file errorhandler
  * \brief Receives error massages for error mailbox,
  *         runs External interrupt driver for fuse error detection.
  * \author Gyorgy Stercz
  */
#ifndef ERRORHANDLER_H_INCLUDED
#define ERRORHANDLER_H_INCLUDED

#include <sys/queue.h>

#define ERRORHANDLER_STACK_SIZE 256

#define ERRORHANDLER_SLEEP_TIME_US 10000

#define ERRORLIST_CMD_NAME "errorlist"
#define ERRORLIST_CMD {ERRORLIST_CMD_NAME, cmd_errorlist}

/** \brief Error list item
  */
struct error_item{
    STAILQ_ENTRY(error_item) entries;
    char *error_str;
};

/** \brief Sends mailbox massage to errorhandler thread.
  *
  * \param msg  massage code.
  */
void sendErrMail(msg_t msg);

/** \brief Errorlist user interface
  *
  */
void cmd_errorlist(BaseSequentialStream *chp, int argc, char *argv[]);

/** \brief Initializes errorhandler
  *         - Error list init.
  *         - EXT driver start.
  *         - Creates errorhandler thread.
  */
void errorhandlerInit(void);

#endif // ERRORHANDLER_H_INCLUDED
