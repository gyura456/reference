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
/** \file tempreader
  * \brief Reads temperature, and puts it into the tempFIFO.
  *
  * \author Gyorgy Stercz
  */
#ifndef TEMPREADER_H_INCLUDED
#define TEMPREADER_H_INCLUDED

#include <appconf.h>

#define TEMPREADER_STACK_SIZE 128

#define TEMPREADER_CMD_NAME "sensorerror"
#define TEMPREADER_CMD {TEMPREADER_CMD_NAME, cmd_tempreader}

/** \brief Enumeration of tempreader thread states.
  */
typedef enum{TEMPREADER_INIT=0, TEMPREADER_OK}tempreader_state_t;

/** \brief Enumeration of sensor states.
  */
typedef enum{SENSOR_INIT= 0, SENSOR_OK, SENSOR_ERROR}sensor_state_t;

/** \brief tempreader user interface
  *
  */
void cmd_tempreader(BaseSequentialStream *chp, int argc, char *argv[]);

/** \brief Initializes tempreader
  *         - Start i2c driver.
  *         - GPT driver start.
  *         - Creates tempreader thread.
  */
void tempreaderInit(void);
#endif // TEMPREADER_H_INCLUDED
