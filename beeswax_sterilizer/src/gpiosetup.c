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
/** \file gpiosetup.c
  * \brief GPIO pin initialization source.
  */
#include <gpiosetup.h>

/** \brief Initializes pins of display on the stm32f746 discovery board.
  */
static void displayPinInit(void){
    struct GPIO_Pin lcdpins[] = LCD_PIN_TABLE;
    uint8_t i;
    for(i=0; i<LCD_PIN_NUM; i++)
        palSetPadMode(lcdpins[i].port, lcdpins[i].pin, lcdpins[i].mode);
        palSetPad(lcdpins[28].port, lcdpins[28].pin);
        palSetPad(lcdpins[29].port, lcdpins[29].pin);
}

/** \brief Initializes pins of sdram on the stm32f746 discovery board.
  */
static void sdramPinInit(void){
    struct GPIO_Pin sdrampins[] = SDRAM_PIN_TABLE;
    uint8_t i;
    for(i=0; i<SDRAM_PIN_NUM; i++)
        palSetPadMode(sdrampins[i].port, sdrampins[i].pin, sdrampins[i].mode);
}

/** \brief Initializes touch sensor I2C pins.
  */
static void touchsensorPinInit(void){
    struct GPIO_Pin scl = TOUCH_SCL;
    struct GPIO_Pin sda = TOUCH_SDA;
    palSetPadMode(scl.port, scl.pin, scl.mode);
    palSetPadMode(sda.port, sda.pin, sda.mode);
}

/** \brief Initializes Temperature sensor I2C pins.
  */
static void temperaturesensorPinInit(void){
    struct GPIO_Pin scl = TSENS_SCL;
    struct GPIO_Pin sda = TSENS_SDA;
    palSetPadMode(scl.port, scl.pin, scl.mode);
    palSetPadMode(sda.port, sda.pin, sda.mode);
}

/** \brief Initializes SD card pins.
  */
static void sdcardPinInit(void){
    struct GPIO_Pin sdcpins[] = SDC_PIN_TABLE;
    uint8_t i;
    for (i=0; i<SDC_PIN_NUM; i++)
        palSetPadMode(sdcpins[i].port, sdcpins[i].pin, sdcpins[i].mode);
}

/** \brief Initializes pins of PWM channels.
  */
static void pwmPinInit(void){
    struct GPIO_Pin ch0 = PWM_CH0;
    struct GPIO_Pin ch1 = PWM_CH1;
    struct GPIO_Pin ch2 = PWM_CH2;
    palSetPadMode(ch0.port, ch0.pin, ch0.mode);
    palSetPadMode(ch1.port, ch1.pin, ch1.mode);
    palSetPadMode(ch2.port, ch2.pin, ch2.mode);
}

/** \brief Initializes external interrupt pins.
  */
static void IntPinInit(void){
    struct GPIO_Pin ch0 = INT_CH0;
    struct GPIO_Pin ch1 = INT_CH1;
    struct GPIO_Pin ch2 = INT_CH2;
    palSetPadMode(ch0.port, ch0.pin, ch0.mode);
    palSetPadMode(ch1.port, ch1.pin, ch1.mode);
    palSetPadMode(ch2.port, ch2.pin, ch2.mode);
}

/** \brief Initializes pins of printer serial port.
  */
static void printerPinInit(void){
    struct GPIO_Pin printertx = PRINTER_TX;
    struct GPIO_Pin printerrx = PRINTER_RX;
    palSetPadMode(printertx.port, printertx.pin, printertx.mode);
    palSetPadMode(printerrx.port, printerrx.pin, printerrx.mode);
}


/** \brief Initializes GPIO pins.
  */
void gpioInit(void){
    displayPinInit();
    sdramPinInit();
    touchsensorPinInit();
    temperaturesensorPinInit();
    sdcardPinInit();
    pwmPinInit();
    printerPinInit();
    IntPinInit();
}

