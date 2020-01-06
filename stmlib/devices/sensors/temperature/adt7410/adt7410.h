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
/** \file adt7410.h
  * \brief ADT7410 temperature sensor definitons.
  *
  * \author Gyorgy Stercz
  */

#ifndef ADT7410_H_INCLUDED
#define ADT7410_H_INCLUDED

/** \brief Sensor register addresses
  *
  * \{
  */
#define TEMP_TOP8_BITS_REG 0x0
#define TEMP_BOTTOM8_BITS_REG 0x1
#define STATUS_REG 0x2
#define CONFIG_REG 0x3
#define T_HIGH_TOP8_BIT_REG 0x4
#define T_HIGH_BOTTOM8_BIT_REG 0x5
#define T_LOW_TOP8_BIT_REG 0x6
#define T_LOW_BOTTOM8_BIT_REG 0x7
#define T_CRIT_TOP8_BIT_REG 0x8
#define T_CRIT_BOTTOM8_BIT_REG 0x9
#define T_HYST_REG 0xA
#define ID_REG 0xB
#define SOFTWARE_RESET 0x2F
/**\} */
#endif // ADT7410_H_INCLUDED

/** \brief Configuration register bit definitions
  *
  * \{
  */
#define FAULT_NUM_1 0x0
#define FAULT_NUM_2 0x1
#define FAULT_NUM_3 0x2
#define FAULT_NUM_4 0x3
#define CT_PIN_POL_LOW 0x0
#define CT_PIN_POL_HIGH 0x4
#define INT_PIN_POL_LOW 0x0
#define INT_PIN_POL_HIGH 0x8
#define INTERUPT_MODE 0x0
#define COMPARATOR_MODE 0x10
#define CONTINOUS_CONVERSION 0x0
#define ONE_SHOT_CONV_MODE 0x20
#define ONE_SPS_MODE 0x40
#define SHUTDOWN_MODE 0x60
#define RESOLUTION_13_BIT 0x0
#define RESOLUTION_16_BIT 0x80
/**\} */
