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
/** \file
  * \brief
  *
  * \author Gyorgy Stercz
  */
#ifndef APPCONF_H_INCLUDED
#define APPCONF_H_INCLUDED

#include <adt7410.h>
/*===========================================================================*/
/* Mailbox massages                                                          */
/*===========================================================================*/

#define SENSOR0_ERR_MSG                      1
#define SENSOR1_ERR_MSG                      2
#define SENSOR2_ERR_MSG                      3
#define CRIT_DTEMP_ERR_MSG                   4
#define CRIT_TEMP_ERR_MSG                    5
#define FUZZY_LOGIC_ERR_MSG                  6
#define CH0_FUSE_ERROR                       7
#define CH1_FUSE_ERROR                       8
#define CH2_FUSE_ERROR                       9
#define SENSOR_INIT_END                      10
#define START_STERILIZER                     11
#define STOP_STERILZER                       12
#define STOPERROR_STERILIZER                 13
#define FUZZY_REG_START_MSG                  14
#define FUZZY_REG_STOP_MSG                   15
#define FUZZY_REG_DISABLE_MSG                16
#define PRINT_RESULT_LIST                    17

/*===========================================================================*/
/* Tempreader thread definitions.                                            */
/*===========================================================================*/
#define TEMP_SAMPLE_TIME_MS                 1000
#define CHANNEL_NUM                         3
#define TEMPSENSOR_ADDR_BASE                0x48
#define SENSOR_FIRST_CONVERSION_TIME_MS     6
#define SENSOR_TIMEOUT_MS                   4
#define SENSOR_CONFIG_REG_INIT              (CT_PIN_POL_HIGH | INT_PIN_POL_HIGH | COMPARATOR_MODE | ONE_SPS_MODE | RESOLUTION_16_BIT)
#define SENSOR_TEMP_QUANTUM                 0.0078125
#define H_DELTA                             4
#define RUNNING_AVG_FIFO_SIZE               16

/*===========================================================================*/
/* Regulator thread definitions.                                             */
/*===========================================================================*/

#define FUZZYREG_MAILBOX_SIZE               10
#define TEMP_FIFO_SIZE                      4
#define PWM_CLOCK                           10000
#define PWM_COUNT                           10000
#define PWM_STEP                            PWM_CLOCK/100

#define HEAT_CH0                            {&PWMD3, 0}
#define HEAT_CH1                            {&PWMD5, 3}
#define HEAT_CH2                            {&PWMD1, 0}
#define HEAT_CHANNELS                       {HEAT_CH0, HEAT_CH1, HEAT_CH2}

#define HEAT_CH0_CFG                        {PWM_CLOCK, \
                                             PWM_COUNT, \
                                             setDutyCycleCH0,  \
                                            {       \
                                                {PWM_OUTPUT_ACTIVE_HIGH, NULL}, \
                                                {PWM_OUTPUT_DISABLED, NULL},    \
                                                {PWM_OUTPUT_DISABLED, NULL},    \
                                                {PWM_OUTPUT_DISABLED, NULL}     \
                                            },  \
                                            0,  \
                                            0}  \

#define HEAT_CH1_CFG                        {PWM_CLOCK, \
                                             PWM_COUNT, \
                                             setDutyCycleCH1,  \
                                            {       \
                                                {PWM_OUTPUT_DISABLED, NULL},    \
                                                {PWM_OUTPUT_DISABLED, NULL},    \
                                                {PWM_OUTPUT_DISABLED, NULL},    \
                                                {PWM_OUTPUT_ACTIVE_HIGH, NULL}  \
                                            },  \
                                            0,  \
                                            0} \

#define HEAT_CH2_CFG                        {PWM_CLOCK, \
                                             PWM_COUNT, \
                                             setDutyCycleCH2,  \
                                            {       \
                                                {PWM_OUTPUT_ACTIVE_HIGH, NULL},  \
                                                {PWM_OUTPUT_DISABLED, NULL},    \
                                                {PWM_OUTPUT_DISABLED, NULL},    \
                                                {PWM_OUTPUT_DISABLED, NULL}     \
                                            },  \
                                            0,  \
                                            0}  \

#define HEAT_CHANNELS_CFG                   {HEAT_CH0_CFG, HEAT_CH1_CFG, HEAT_CH2_CFG}

#define TEMP_MELTING {8320, 9600, 8320, 8320, fuzzyficHalfTrapezeTypeMf}
#define TEMP_COLD {8320, 10880, 9600, 9600, fuzzyficTriangleTypeMf}
#define TEMP_MEDIUM {9600, 14848, 10880, 10880, fuzzyficTriangleTypeMf}
#define TEMP_HOT {10880, 14848, 14848, 14848, fuzzyficHalfTrapezeTypeMf}
#define TEMP_STERILE {14592, 14848, 14592, 14848, fuzzyficTrapezeTypeMf}

#define TEMP_INPUT_MFS {TEMP_MELTING, TEMP_COLD, TEMP_MEDIUM, TEMP_HOT, TEMP_STERILE}

#define DELTATEMP_NEG {-1, 0, -1, -1, fuzzyficHalfTrapezeTypeMf}
#define DELTATEMP_ZERO  {-1, 2, 0, 0,fuzzyficTriangleTypeMf}
#define DELTATEMP_SPOS {0, 4, 2, 2,fuzzyficTriangleTypeMf}
#define DELTATEMP_POS {2, 6, 4, 4,fuzzyficTriangleTypeMf}
#define DELTATEMP_VPOS {4, 6, 6, 6, fuzzyficHalfTrapezeTypeMf}

#define DTEMP_INPUT_MFS {DELTATEMP_NEG, DELTATEMP_ZERO, DELTATEMP_SPOS, DELTATEMP_POS, DELTATEMP_VPOS}

#define FUZZY_INPUT_NUM                     2

#define PWM_OFF {0}
#define PWM_SMALL {25}
#define PWM_HALF {50}
#define PWM_WIDE {75}
#define PWM_FULL {100}

#define PWM_OUTPUT_MFS {PWM_OFF, PWM_SMALL, PWM_HALF, PWM_WIDE, PWM_FULL}

#define FUZZY_RULES  { \
                      {&temp_mships.melting, NULL, &pwm_mships.full}, \
                      {&temp_mships.cold, &dtemp_mships.neg, &pwm_mships.full}, \
                      {&temp_mships.cold, &dtemp_mships.zero, &pwm_mships.full}, \
                      {&temp_mships.cold, &dtemp_mships.spos, &pwm_mships.wide}, \
                      {&temp_mships.cold, &dtemp_mships.pos, &pwm_mships.half}, \
                      {&temp_mships.cold, &dtemp_mships.vpos, &pwm_mships.small}, \
                      {&temp_mships.medium, &dtemp_mships.neg, &pwm_mships.full}, \
                      {&temp_mships.medium, &dtemp_mships.zero, &pwm_mships.wide}, \
                      {&temp_mships.medium, &dtemp_mships.spos, &pwm_mships.half}, \
                      {&temp_mships.medium, &dtemp_mships.pos, &pwm_mships.small}, \
                      {&temp_mships.medium, &dtemp_mships.vpos, &pwm_mships.off}, \
                      {&temp_mships.hot, NULL, &pwm_mships.off}, \
                      {&temp_mships.sterile, &dtemp_mships.neg, &pwm_mships.wide}, \
                      {&temp_mships.sterile, &dtemp_mships.zero, &pwm_mships.small}, \
                    }

#define FUZZY_RULES_NUM                     14
#define CRITICAL_TEMP                       16000
#define MELTING_END_TEMP                    9600
#define CRITICAL_TG_ALPHA                   0.1
/*===========================================================================*/
/* Sterilizer thread definitions.                                            */
/*===========================================================================*/
#define STERILIZER_MAILBOX_SIZE             10
#define STERILE_TEMP                        14592
#define STERILIZER_DMA_NUM                  2
#define STERILIZER_DMA_STREAM_NUM           1
#define STERILIZER_DMA_STREAM               STM32_DMA_STREAM_ID(STERILIZER_DMA_NUM, STERILIZER_DMA_STREAM_NUM)
#define STERILIZER_DMA_CHANNEL              3
#define STERILIZER_DMA_PRIORITY             3
#define STERILIZER_DMA_IRQ_PRIORITY         5

#define STERLIZER_SAVE_INTERVAL_S           60
#define RESULT_LIST_SIZE                    61
#define NUM_OF_TEMP_SWING                   5

/*===========================================================================*/
/* Errorhandler thread definitions.                                          */
/*===========================================================================*/

#define ERR_HANDL_MAILBOX_SIZE              20
#define ERROR_LIST_MAX_SIZE                 20

/*===========================================================================*/
/* Lcdcontol thread definitions.                                             */
/*===========================================================================*/

#define DRAW_JOB_QUEUE_SIZE                 20

/*===========================================================================*/
/* Printer thread definitions.                                               */
/*===========================================================================*/

#define PRINTER_BUFFER_ITEM_SIZE            50
#define PRINTER_BUFFER_SIZE                 10

/*===========================================================================*/
/* Cardhandler thread definitions.                                           */
/*===========================================================================*/

#define FILE_BUFFER_ITEM_SIZE               50
#define FILE_BUFFER_SIZE                    10

#define SDC_POLLING_INTERVAL                10
#define SDC_POLLING_DELAY_MS                10


#endif // APPCONF_H_INCLUDED
