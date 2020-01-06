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
/** \file tempreader.c
  * \brief Tempreader thread source
  */

#include <stdlib.h>
#include <string.h>

#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <appconf.h>
#include <tempreader.h>
#include <errorhandler.h>
#include <cardhandler.h>
#include <regulator.h>
#include <lcdcontrol.h>


#if TEMPREADER_STACK_SIZE < 128
    #error Minimum task stack size is 128!
#endif


static THD_WORKING_AREA(waThreadtempreader, TEMPREADER_STACK_SIZE);
static MUTEX_DECL(trmtx);
/** \brief I2C configuration.
  *
  */
static I2CConfig i2c1cfg = {
    STM32_TIMINGR_PRESC(1U) |
    STM32_TIMINGR_SCLDEL(14U) | STM32_TIMINGR_SDADEL(3U) |
    STM32_TIMINGR_SCLH(99U) | STM32_TIMINGR_SCLL(135U),
    I2C_CR1_DNF,
    0
};

/** \brief Structure for thread data.
  */
static struct{
    uint8_t txbuff[2];
    uint8_t rxbuff[2];
    tempreader_state_t state;
    sensor_state_t sensorstate[CHANNEL_NUM];
    uint8_t sensor_error_code[CHANNEL_NUM];
    int16_t prev_temp[CHANNEL_NUM];
    RTCDateTime rtctime;
    uint8_t all_is_sterile;
    int16_t h_max[CHANNEL_NUM];
    int16_t h_min[CHANNEL_NUM];
    thread_t *tp;
    int16_t runavgfifo[CHANNEL_NUM][RUNNING_AVG_FIFO_SIZE];
    uint8_t runavgfifo_size;
}tempreader;




/** \brief Creates an event, when timer is ended.
  *
  */
static void gpt6cb(GPTDriver *gptp){
    (void)gptp;
    chSysLockFromISR();
    chEvtSignalI(tempreader.tp, (eventmask_t)1);
    chSysUnlockFromISR();
}

/** \brief Timer configuration.
  *
  */
static const GPTConfig gpt6cfg = {
    10000,          /** Timer clock 10kHz */
    gpt6cb,         /** Timer callback */
    0,
    0
};

/** \brief  Tempreader thread function.
  *            - Reads temperature sensors periodic.
  *            - Operates with running hysteresis and running avg.
  *            - Put temperature data into the TempFIFO.
  */
__attribute__((noreturn))
static THD_FUNCTION(Threadtempreader, arg) {
    (void) arg;
    chRegSetThreadName("tempreader");
    tempreader.tp = chThdGetSelfX();
    uint8_t ch, smpnum = 0;
    msg_t readmsg = 0;
    int16_t res = 0;
    int32_t ravg = 0;
    struct inner_buffer_item *item = NULL;
    temperature_t *temp = NULL;
    /* Sensor init */
    tempreader.txbuff[0] = CONFIG_REG;
    tempreader.txbuff[1] = SENSOR_CONFIG_REG_INIT;
    for (ch=0; ch<CHANNEL_NUM; ch++){
        tempreader.h_max[ch] = tempreader.h_min[ch] + H_DELTA;
        i2cAcquireBus(&I2CD1);
        readmsg = i2cMasterTransmitTimeout(&I2CD1, TEMPSENSOR_ADDR_BASE+ch, tempreader.txbuff, 2, NULL, 0, MS2ST(SENSOR_TIMEOUT_MS));
        i2cReleaseBus(&I2CD1);
        if (readmsg != MSG_OK){
            tempreader.sensorstate[ch] = SENSOR_ERROR;
            tempreader.sensor_error_code[ch] = i2cGetErrors(&I2CD1);
            sendErrMail(SENSOR0_ERR_MSG+ch);
            setSensorState(tempreader.sensorstate);
            }
    }
    setSensorState(tempreader.sensorstate);
    tempreader.txbuff[0] = TEMP_BOTTOM8_BITS_REG;
    tempreader.txbuff[1] = TEMP_TOP8_BITS_REG;
    /* wake up timer start */
    gptStartContinuous(&GPTD6, TEMP_SAMPLE_TIME_MS*10);
    while(TRUE) {
            chEvtWaitOne((eventmask_t)1);
            getDate(&tempreader.rtctime);
            for (ch=0; ch<CHANNEL_NUM; ch++){
                if(tempreader.sensorstate[ch] != SENSOR_ERROR){
                    i2cAcquireBus(&I2CD1);
                    readmsg = i2cMasterTransmitTimeout(&I2CD1, TEMPSENSOR_ADDR_BASE+ch, &tempreader.txbuff[0], 1,
                                                &tempreader.rxbuff[0], 1, MS2ST(SENSOR_TIMEOUT_MS));
                    readmsg = i2cMasterTransmitTimeout(&I2CD1, TEMPSENSOR_ADDR_BASE+ch, &tempreader.txbuff[1], 1,
                                                &tempreader.rxbuff[1], 1, MS2ST(SENSOR_TIMEOUT_MS));
                    i2cReleaseBus(&I2CD1);
                    if (readmsg != MSG_OK){
                        tempreader.sensorstate[ch] = SENSOR_ERROR;
                        tempreader.sensor_error_code[ch] = i2cGetErrors(&I2CD1);
                        sendErrMail(SENSOR0_ERR_MSG+ch);
                        setSensorState(tempreader.sensorstate);
                        continue;
                    }
                    res = (((uint16_t)tempreader.rxbuff[1] << 8) | ((uint16_t)tempreader.rxbuff[0] & 0xFF));
                }
            /* Running avg*/
            for(smpnum=RUNNING_AVG_FIFO_SIZE-1; smpnum>0; smpnum--)
                tempreader.runavgfifo[ch][smpnum] = tempreader.runavgfifo[ch][smpnum-1];
            tempreader.runavgfifo[ch][0] = res;
        }
        if (tempreader.runavgfifo_size < RUNNING_AVG_FIFO_SIZE){
            tempreader.runavgfifo_size++;
            continue;
        }
        /* Get new tempFIFO item. */
        item = getTempFIFOItem();
        if (item){
            temp = (temperature_t*)item->data;
            tempreader.all_is_sterile = 0;
            for (ch=0; ch<CHANNEL_NUM; ch++){
                if (tempreader.sensorstate[ch] == SENSOR_INIT){
                    tempreader.sensorstate[ch] = SENSOR_OK;
                }
                ravg = 0;
                for(smpnum=0; smpnum<RUNNING_AVG_FIFO_SIZE; smpnum++)
                    ravg += tempreader.runavgfifo[ch][smpnum];
                res = ravg/RUNNING_AVG_FIFO_SIZE;
                /* Running Hysteresis */
                if (res > tempreader.h_max[ch]){
                    tempreader.h_max[ch] = res;
                    tempreader.h_min[ch] = tempreader.h_max[ch] - H_DELTA;
                }
                if (res < tempreader.h_min[ch]){
                    tempreader.h_min[ch] = res;
                    tempreader.h_max[ch] = tempreader.h_min[ch] + H_DELTA;
                }
                res = tempreader.h_min[ch];
                if (res > STERILE_TEMP)
                    tempreader.all_is_sterile++;
                temp->dtemp[ch] = res - tempreader.prev_temp[ch];
                temp->temp[ch] = res;
                tempreader.prev_temp[ch] = res;
                }
            if (tempreader.all_is_sterile == 3)
                    temp->is_sterile = 1;
                else
                    temp->is_sterile = 0;
            temp->timestamp = tempreader.rtctime.millisecond;
            putTempToFIFO(item);
            if (tempreader.state == TEMPREADER_INIT){
                tempreader.state = TEMPREADER_OK;
                setSensorState(tempreader.sensorstate);
                sendMailtoSterilizer(SENSOR_INIT_END);
            }

        }
    }
    chThdExit(1);
}


/** \brief tempreader user interface
  *
  */
void cmd_tempreader(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    (void) chp;
    uint8_t i;
    uint8_t err[CHANNEL_NUM];
    chMtxLock(&trmtx);
    for(i=0; i<CHANNEL_NUM; i++)
        err[i] = tempreader.sensor_error_code[i];
    chMtxUnlock(&trmtx);
    for(i=0; i<CHANNEL_NUM; i++)
        chprintf(chp, "S%d error: %d\n\r", i, err[i]);

}



/** \brief Initializes tempreader
  *         - Start i2c driver.
  *         - GPT driver start.
  *         - Creates tempreader thread.
  */
void tempreaderInit(void){
    bzero(&tempreader, sizeof(tempreader));
    /* i2c bus and sensor init*/
    i2cStart(&I2CD1, &i2c1cfg);
    gptStart(&GPTD6,&gpt6cfg);
    chThdCreateStatic(waThreadtempreader, sizeof(waThreadtempreader), NORMALPRIO+20, Threadtempreader, NULL);
}
