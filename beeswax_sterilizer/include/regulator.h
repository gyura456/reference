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
/** \file regulator.h
  * \brief Fuzzy regulator thread.
  *        Manages temperature FIFO, handles PWM output with fuzzy logic.
  * \author Gyorgy Stercz
  */
#ifndef REGULATOR_H_INCLUDED
#define REGULATOR_H_INCLUDED

#include <appconf.h>
#include <inner_buffer.h>

#define REGULATOR_STACK_SIZE 512

#define REGULATOR_SLEEP_TIME_US 10000

#define SEC_IN_A_DAY 86400

#define TEMPFIFO_CMD_NAME "tempfifo"
#define TEMPFIFO_CMD {TEMPFIFO_CMD_NAME, cmd_tempfifo}

#define FUZZYERROR_CMD_NAME "fuzzyerror"
#define FUZYYERROR_CMD {FUZZYERROR_CMD_NAME, cmd_fuzzyerror}

#define min(a,b) (((a) < (b)) ? (a) : (b))

/** \brief Enumeration of fuzzy regulator states.
  */
typedef enum{FUZZYREG_STOP, FUZZYREG_ACTIVE, FUZZYREG_DISABLE}fuzzyreg_state_t;

/** \brief Heat channel typedef.
  */
typedef struct{
    PWMDriver *pwmp;
    uint8_t chnum;
}heat_channel_t;

/** \brief Input membership function typedef.
  */
typedef struct{
    int16_t rangefrom;
    int16_t rangeto;
    int16_t maxfrom;
    int16_t maxto;
    float(*fuzzyfic_func)(void *mfp, int16_t input);
}input_mf;

/** \brief Output membership function typedef.
  */
typedef struct{
    uint8_t maxpoint;
}output_mf;

/** \brief Fuzzy rule typedef.
  */
typedef struct{
    input_mf *if_side1;
    input_mf *if_side2;
    output_mf *then_side;
}fuzzy_rule;

/** \brief Structure of temperature input membership functions.
  */
struct Temp_mship{
    input_mf melting;
    input_mf cold;
    input_mf medium;
    input_mf hot;
    input_mf sterile;
};

/** \brief Structure of delta temperature input membership functions.
  */
struct DeltaTemp_mship{
    input_mf neg;
    input_mf zero;
    input_mf spos;
    input_mf pos;
    input_mf vpos;
};

/** \brief Structure of pwm output membership functions.
  */
struct PWM_mship{
    output_mf off;
    output_mf small;
    output_mf half;
    output_mf wide;
    output_mf full;
};

/** \brief Temperature typedef.
  */
typedef struct{
    uint32_t timestamp;
    int16_t temp[CHANNEL_NUM];
    int16_t dtemp[CHANNEL_NUM];
    bool is_sterile;
}temperature_t;

/** \brief Get an empty temperature FIFO item.
  *
  * \return Pointer to FIFO item or NULL, if the FIFO is full.
  */
struct inner_buffer_item *getTempFIFOItem(void);

/** \brief Put temperature into the FIFO.
  *
  * \param item     Pointer of the FIFO item, NULL save.
  */
void putTempToFIFO(struct inner_buffer_item *item);

/** \brief Says, that the temperature FIFO is full.
  *
  * \return TRUE, if the FIFO is full, FALSE if not full.
  */
int8_t isTempFIFOFull(void);

/** \brief Temperature FIFO user interface.
  */
void cmd_tempfifo(BaseSequentialStream *chp, int argc, char *argv[]);

/** \brief Fuzzy logic user interface.
  */
void cmd_fuzzyerror(BaseSequentialStream *chp, int argc, char *argv[]);

/** \brief Sends mailbox massage to regulator thread
  *
  * \param msg  Massage code.
  */
void sendMailToRegulator(msg_t msg);

/** \brief Sends disable high priority mailbox massage to regulator thread
  *
  * \param msg  Massage code.
  */
void sendDisableMailToRegluator(msg_t msg);

/** \brief Get current temperature.
  *
  * \param data     Pointer to temperature object, NULL save.
  */
void getCurrentTemp(temperature_t *data);

/** \brief Initializes regulator
  *         - Temperature FIFO init.
  *         - PWM driver init.
  *         - Creates regulator thread.
  */
void regulatorInit(void);

#endif // REGULATOR_H_INCLUDED
