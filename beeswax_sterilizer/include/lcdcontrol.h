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
/** \file lcdcontrol.h
  * \brief Lcd controler thread.
  *        Manages drawning operation tol LCD and touch sensor.
  * \author Gyorgy Stercz
  */
#ifndef LCDCONTROL_H_INCLUDED
#define LCDCONTROL_H_INCLUDED

#include <regulator.h>
#include <tempreader.h>
#include <sterilizer.h>
#include <cardhandler.h>

#define LCDCONTROL_STACK_SIZE 4096

#define LCDCONTROL_SLEEP_TIME_US 10000

#define MAIN_TABSET_PAGE_NUM

#define DRAWJOB_QUEUE_CMD_NAME "drawjob"
#define DRAWJOB_QUEUE_CMD {DRAWJOB_QUEUE_CMD_NAME, cmd_drawjob}

/** \brief Set sensor state.
  *
  *  \param state   Pointer to array of sensor state, NULL save.
  */
void setSensorState(sensor_state_t *state);

/** \brief Displays current temperature.
  *
  *  \param temp   Pointer to temperature array, NULL save.
  */
void displayCurrentTemp(int16_t *temp);

/** \brief Displays pwm channels duty cycle.
  *
  *  \param dutycycle   Pointer to duty cycle array, NULL save.
  */
void displayHeatPower(pwmcnt_t *dutycycle);

/** \brief Set fuzzy regulator state.
  *
  *  \param state   Pointer to fuzzy regulator state, NULL save.
  */
void setFuzzyregState(fuzzyreg_state_t *state);

/** \brief Displays sterilizer state
  *
  *  \param state   Pointer to sterilizer state,  NULL save.
  */
void displaySterilizerState(sterilizer_state_t *state);

/** \brief Display error list item.
  *
  *  \param item   Pointer to string of error list item, NULL save.
  */
void displayErrorListItem(char *item);

/** \brief Displays result list item.
  *
  *  \param item   Pointer to string of result list item, NULL save.
  */
void displayResultListItem(char *item);

/** \brief Clears displayed result list.
  */
void destroyDisplayedResultList(void);

/** \brief Displays switch result page.
  */
void switchToresultPage(void);

/** \brief Displayed sterilization start time.
  *
  * \param start    Pointer to start time in RTCDateTIme structure format, NULL save.
  */
void displayResultStart(RTCDateTime *start);

/** \brief Displays sterilization end time and final result.
  *
  * \param endtime      Pointer to end time, NULL save.
  * \param finalresult  Bool variable if final result.
  */
void displayResultEnd(uint32_t *endtime, bool finalresult);

/** \brief Displays SDC state.
  *
  * \param state    Pointer to sdc state, NULL save.
  */
void displaySdcState(sdc_state_t *state);

/** \brief Drawing job queue user interface
  *
  */
void cmd_drawjob(BaseSequentialStream *chp, int argc, char *argv[]);

/** \brief Initializes lcdcontrol
  *             - Drawing job queue init.
  *             - Creates lcdcontrol thread.
  */
void lcdcontrolInit(void);

#endif // LCDCONTROL_H_INCLUDED
