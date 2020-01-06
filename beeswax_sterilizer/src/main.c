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
/** \file main.c
  * \brief beeswax_sterilizer application main thread.
  *
  * \author Gyorgy Stercz
  */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ch.h>
#include <hal.h>
#include <ch_test.h>
#include <gpiosetup.h>
#include <stmlib.h>
#include <gfx.h>

#include <chprintf.h>
#include <shell.h>
#include <usbcfg.h>

#include <tempreader.h>
#include <lcdcontrol.h>
#include <sterilizer.h>
#include <errorhandler.h>
#include <cardhandler.h>
#include <printer.h>
#include <regulator.h>
/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

/** \brief Shell commands
  */
static const ShellCommand commands[] = {
    TEMPREADER_CMD,
    DRAWJOB_QUEUE_CMD,
    RESULTLIST_CMD,
    LOG_BUFFER_CMD,
    RESULT_FILE_BUFFER_CMD,
    SDC_CMD,
    PRINT_BUFF_CMD,
    TEMPFIFO_CMD,
    FUZYYERROR_CMD,
    ERRORLIST_CMD,
    {NULL, NULL}
};

/** \brief Structure of Shell configuration
  */
static const ShellConfig shell_cfg1 = {
    (BaseSequentialStream *)&SDU1,
    commands
};

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

static thread_t *shelltp = NULL;

/** \brief Shell exit event.
 */
static void ShellHandler(eventid_t id) {

    (void)id;
    if (chThdTerminatedX(shelltp)) {
    chThdRelease(shelltp);
    shelltp = NULL;
    }
}

/** \brief  Initializes a serial-over-USB CDC driver.
  *         Activates the USB driver and then the USB bus pull-up on D+.
  *         Note, a delay is inserted in order to not have to disconnect the cable
  *         after a reset.
  *         Shell manager initialization.
  */

static void connectConsole(void){
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);
    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1500);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);
    shellInit();
}


/** \brief  Application entry point.
  *         System initializations.
  *          - HAL initialization, this also initializes the configured device drivers
  *            and performs the board-specific initializations.
  *          - Kernel initialization, the main() function becomes a thread and the
  *            RTOS is active.
  *         Normal main() thread activity, handling shell start/exit.
  */
int main(void) {
    static const evhandler_t evhndl[] = {
    ShellHandler
    };
    event_listener_t el0;
    chSysInit();
    halInit();
    gpioInit();
    stmlibInit();
    gfxInit();
    connectConsole();
    chEvtRegister(&shell_terminated, &el0, 0);

    lcdcontrolInit();
    regulatorInit();
    tempreaderInit();
    sterilizerInit();
    cardhandlerInit();
    printerInit();
    errorhandlerInit();
    while (true) {
        if (!shelltp && (SDU1.config->usbp->state == USB_ACTIVE)) {
            shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                    "shell", NORMALPRIO + 1,
                                    shellThread, (void *)&shell_cfg1);
        }
        chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, MS2ST(500)));
    }
}

