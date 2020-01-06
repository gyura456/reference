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
/** \file lcdcontrol.c
  * \brief  Lcd controller thread source.
  */

#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <gfx.h>
#include <lcdcontrol.h>
#include <appconf.h>
#include <cardhandler.h>
#include <errorhandler.h>
#include <numkeys.h>


#if LCDCONTROL_STACK_SIZE < 128
    #error Minimum task stack size is 128!
#endif

#if LCDCONTROL_SLEEP_TIME_US < 1
    #error task sleep time must be at least 1
#endif

#if LCDCONTROL_SLEEP_TIME_US < 100
    #warning task sleep time seems to be to few!
#endif

static THD_WORKING_AREA(waThreadlcdcontrol, LCDCONTROL_STACK_SIZE);
static MUTEX_DECL(lcdmtx);

/*===========================================================================*/
/* Draw job queue.                                                           */
/*===========================================================================*/
/** \brief Drawing function pointer typedef.
  */
typedef void (*drawfunc)(void);

/** \brief Drawing job queue item
  */
struct drawitem{
    STAILQ_ENTRY(drawitem) entries;
    drawfunc func;
};

/** \brief Memory pool of drawing job queue.
  *
  * \{
  */
static struct drawitem drawjobs[DRAW_JOB_QUEUE_SIZE];
static MEMORYPOOL_DECL(drawjobpool, sizeof(struct drawitem), NULL);
/**\} */

/** \brief Drawing job queue private area.
  */
static struct{
    STAILQ_HEAD(drawhaed, drawitem) head;
    struct drawhead *headp;
    uint8_t itemnum;
    uint8_t free_item;
    uint8_t overflow;
    uint8_t underflow;
}drawjobqueue;

/** \brief Initializes drawing job queue.
  */
static void drawjobQueueInit(void){
    bzero(&drawjobqueue, sizeof(drawjobqueue));
    STAILQ_INIT(&drawjobqueue.head);
    chPoolLoadArray(&drawjobpool, drawjobs, DRAW_JOB_QUEUE_SIZE);
    drawjobqueue.free_item = DRAW_JOB_QUEUE_SIZE;
}

/** \brief Add new drawing job to queue.
  *
  * \param funcptr  Pointer to drawing function, NULL save.
  */
static void addDrawJob(void *funcptr){
    if (!funcptr)
        return;
    chMtxLock(&lcdmtx);
    struct drawitem *job = chPoolAlloc(&drawjobpool);
    if (job){
        drawjobqueue.free_item--;
        job->func = funcptr;
        if (STAILQ_EMPTY(&drawjobqueue.head))
            STAILQ_INSERT_HEAD(&drawjobqueue.head, job, entries);
        else
            STAILQ_INSERT_TAIL(&drawjobqueue.head, job, entries);
        drawjobqueue.itemnum++;
    }
    else
        drawjobqueue.underflow++;
    chMtxUnlock(&lcdmtx);
};

/** \brief Get new drawing job from queue.
  *
  * \return Pointer to drawing job queue item, or NULL if the queue is empty.
  */
static struct drawitem *getDrawJob(void){
    if(STAILQ_EMPTY(&drawjobqueue.head))
        return NULL;
    chMtxLock(&lcdmtx);
    struct drawitem *job = STAILQ_FIRST(&drawjobqueue.head);
    STAILQ_REMOVE_HEAD(&drawjobqueue.head, entries);
    drawjobqueue.itemnum--;
    chMtxUnlock(&lcdmtx);
    return job;
}

/** \brief Put back drawing job queue item into the queue.
  *
  * \param item  Pointer to drawing job queue item, NULL save.
  */
static void freeDrawJob(struct drawitem *item){
    if (!item)
        return;
    if (drawjobqueue.free_item == DRAW_JOB_QUEUE_SIZE){
        drawjobqueue.overflow++;
        return;
    }
    chMtxLock(&lcdmtx);
    item->func = NULL;
    chPoolFree(&drawjobpool, item);
    drawjobqueue.free_item++;
    chMtxUnlock(&lcdmtx);
    return;
}

/*===========================================================================*/
/* Displayed objects                                                         */
/*===========================================================================*/

/** \brief Structure for displayed objects.
  */
static struct{
    GTabsetObject tabset;
    GLabelObject date;
    GLabelObject sdc;
    /*Sterilzer Page*/
    GLabelObject curr_temp[CHANNEL_NUM];
    GLabelObject ster_state;
    GButtonObject ster_start;
    GButtonObject ster_stop;
    GProgressbarObject heatpower[CHANNEL_NUM];
    GProgressbarObject steriletemps;
    /*Result Page */
    GLabelObject res_date;
    GLabelObject final_result;
    GLabelObject res_begin;
    GLabelObject res_end;
    GLabelObject reslist_header;
    GListObject res_list;
    GButtonObject res_print;
    /*Errors Page*/
    GListObject err_list;
    /*Time Page */
    GKeyboardObject keyboard;
    GTexteditObject setyear;
    GTexteditObject setmonth;
    GTexteditObject setday;
    GTexteditObject sethour;
    GTexteditObject setmin;
    GTexteditObject setsec;
    GLabelObject setdatelabel;
    GButtonObject setdatebtn;
}go;

/** \brief Structure for handlers of displayed  objects.
  */
static struct{
    GHandle tabset;
    GHandle date;
    char datestr[25];
    GHandle sdc;
    GHandle sterilizer;
    GHandle result;
    GHandle errors;
    GHandle time;
    /*Sterilzer Page*/
    GWidgetStyle statestyle;
    GHandle curr_temp[CHANNEL_NUM];
    char curr_tempstr[CHANNEL_NUM][50];
    GHandle ster_state;
    GHandle ster_start;
    GHandle ster_stop;
    GHandle heatpower[CHANNEL_NUM];
    GHandle steriletemps;
    /* Result Page */
    GHandle res_date;
    GHandle res_begin;
    GHandle res_end;
    GHandle final_result;
    GWidgetStyle finalresstyle;
    GHandle reslist_header;
    GHandle res_list;
    GHandle res_print;
    /*Errors Page*/
    GHandle err_list;
    /*Time Page*/
    GHandle keyboard;
    GHandle setyear;
    GHandle setmonth;
    GHandle setday;
    GHandle sethour;
    GHandle setmin;
    GHandle setsec;
    GHandle setdatelabel;
    GHandle setdatebtn;
    /*Button listener*/
    GListener gbl;
}gh;


/*===========================================================================*/
/*  Lcd controller thread data                                               */
/*===========================================================================*/

/** \brief Structure for thread data.
  */
static struct{
    sdc_state_t sdc_state;
    int16_t curr_temp[CHANNEL_NUM];
    uint8_t sensorstate[CHANNEL_NUM];
    pwmcnt_t dutycycle[CHANNEL_NUM];
    fuzzyreg_state_t fuzzyreg_state;
    sterilizer_state_t ster_state;
    uint8_t errlistsize;
    RTCDateTime res_start;
    uint64_t res_end;
    bool finalresult;
}appdata;

/*===========================================================================*/
/* Local functions                                                           */
/*===========================================================================*/

/** \brief Creates sterilizer tabset page.
  *
  * \param wip  Pointer to widget init object, NULL save.
  */
static inline void createPageSterilizer(GWidgetInit *wip){
    if (!wip)
        return;
    /* State label init */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 125; wip->g.y = 15;
    wip->g.height = 45 ; wip->g.width = 300;
    wip->g.parent =gh.sterilizer;
    wip->customDraw = gwinLabelDrawJustifiedCenter;
    gh.ster_state = gwinLabelCreate(&go.ster_state, wip);
    gwinSetFont(gh.ster_state, gdispOpenFont("DejaVuSans32"));
    gh.statestyle = WhiteWidgetStyle;
    gwinSetStyle(gh.ster_state, &gh.statestyle);

    /* Tempreatue channel 0 label init */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 160; wip->g.y = 75;
    wip->g.height = 45 ; wip->g.width = 200;
    wip->g.parent =gh.sterilizer;
    gh.curr_temp[0] = gwinLabelCreate(&go.curr_temp[0], wip);
    gwinSetFont(gh.curr_temp[0], gdispOpenFont("DejaVuSans32"));

    /* Channel 1 label init */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 160; wip->g.y = 130;
    wip->g.height = 45 ; wip->g.width = 200;
    wip->g.parent =gh.sterilizer;
    gh.curr_temp[1] = gwinLabelCreate(&go.curr_temp[1], wip);
    gwinSetFont(gh.curr_temp[1], gdispOpenFont("DejaVuSans32"));

    /* Channel 2 label init */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 160; wip->g.y = 185;
    wip->g.height = 45 ; wip->g.width = 200;
    wip->g.parent =gh.sterilizer;
    gh.curr_temp[2] = gwinLabelCreate(&go.curr_temp[2], wip);
    gwinSetFont(gh.curr_temp[2], gdispOpenFont("DejaVuSans32"));

    /* Strerilizer start stop switch */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 5; wip->g.y = 5;
    wip->g.height = 40 ; wip->g.width = 80; wip->text = "Start";
    wip->g.parent =gh.sterilizer;
    gh.ster_start = gwinButtonCreate(&go.ster_start, wip);
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 5; wip->g.y = 50;
    wip->g.height = 40 ; wip->g.width = 80; wip->text = "Stop";
    wip->g.parent =gh.sterilizer;
    gh.ster_stop = gwinButtonCreate(&go.ster_stop, wip);

    /* Heat power progressbars */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 370; wip->g.y = 75;
    wip->g.height = 45 ; wip->g.width = 100; wip->text = "CH0";
    wip->g.parent =gh.sterilizer;
    gh.heatpower[0] = gwinProgressbarCreate(&go.heatpower[0], wip);

    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 370; wip->g.y = 130;
    wip->g.height = 45 ; wip->g.width = 100; wip->text = "CH1";
    wip->g.parent =gh.sterilizer;
    gh.heatpower[1] = gwinProgressbarCreate(&go.heatpower[1], wip);

    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 370; wip->g.y = 185;
    wip->g.height = 45 ; wip->g.width = 100; wip->text = "CH2";
    wip->g.parent =gh.sterilizer;
    gh.heatpower[2] = gwinProgressbarCreate(&go.heatpower[2], wip);

    /* sterile temps progressbar */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 5; wip->g.y = 110;
    wip->g.height = 100 ; wip->g.width = 150; wip->text = "OK:";
    wip->g.parent =gh.sterilizer;
    gh.steriletemps = gwinProgressbarCreate(&go.steriletemps, wip);
    gwinSetFont(gh.steriletemps, gdispOpenFont("DejaVuSans20"));
    gwinProgressbarSetRange(gh.steriletemps, 0, RESULT_LIST_SIZE);

}

/** \brief Creates result tabset page.
  *
  * \param wip  Pointer to widget init object, NULL save.
  */
static inline void createPageResult(GWidgetInit *wip){
    if (!wip)
        return;
    /* Result Date */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 5; wip->g.y = 5;
    wip->g.height = 15 ; wip->g.width = 100;
    wip->g.parent =gh.result;
    wip->text = "Date:";
    gh.res_date = gwinLabelCreate(&go.res_date, wip);
    /* Result begin time */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 5; wip->g.y = 25;
    wip->g.height = 15 ; wip->g.width = 100;
    wip->g.parent =gh.result;
    wip->text = "Start:";
    gh.res_begin = gwinLabelCreate(&go.res_begin, wip);

    /* Result end time */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 120; wip->g.y = 25;
    wip->g.height = 15 ; wip->g.width = 100;
    wip->g.parent =gh.result;
    wip->text = "End:";
    gh.res_end = gwinLabelCreate(&go.res_end, wip);

    /* Final result */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 120; wip->g.y = 5;
    wip->g.height = 15 ; wip->g.width = 100;
    wip->g.parent =gh.result;
    wip->customDraw = gwinLabelDrawJustifiedCenter;
    wip->text = "Result:";
    gh.final_result = gwinLabelCreate(&go.final_result, wip);
    gh.finalresstyle = WhiteWidgetStyle;
    gh.finalresstyle.enabled.text = White;
    gwinSetStyle(gh.final_result, &gh.finalresstyle);

    /* Result list header */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 10; wip->g.y = 45;
    wip->g.height = 15 ; wip->g.width = 480;
    wip->g.parent =gh.result;
    wip->text = "Nr.\tTime\tCH0\tCH1\tCH2\tStatus";
    gh.reslist_header = gwinLabelCreate(&go.reslist_header, wip);

    /* Result list */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 5; wip->g.y = 60; wip->g.width = 470; wip->g.height = 180;
    wip->g.parent = gh.result;
    gh.res_list = gwinListCreate(&go.res_list, wip, FALSE);
    gwinListSetScroll(gh.res_list, scrollSmooth);

    /* Print button */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 380; wip->g.y = 5;
    wip->g.height = 40 ; wip->g.width = 80; wip->text = "Print";
    wip->g.parent =gh.result;
    gh.res_print = gwinButtonCreate(&go.res_print, wip);
}

/** \brief Creates error tabset page.
  *
  * \param windp Pointer to window init object, NULL save.
  * \param wip   Pointer to widget init object, NULL save.
  */
static inline void createPageErrors (GWidgetInit *wip){
    if (!(wip))
        return;
    /* Error list */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 0; wip->g.y = 0; wip->g.width = 480; wip->g.height = 240;
    wip->g.parent = gh.errors;
    gh.err_list = gwinListCreate(&go.err_list, wip, FALSE);
    gwinListSetScroll(gh.err_list, scrollSmooth);
    gwinSetFont(gh.err_list, gdispOpenFont("DejaVuSans20"));
}

/** \brief Creates time tabset page.
  *
  * \param wip  Pointer to widget init object, NULL save.
  */
static inline void createPageTime(GWidgetInit *wip){
    if(!wip)
        return;
    /* Numeric keys */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 0; wip->g.y = 60;
    wip->g.height = 180 ; wip->g.width = 480;
    wip->g.parent =gh.time;
    gh.keyboard = gwinKeyboardCreate(&go.keyboard, wip);
    gwinKeyboardSetLayout(gh.keyboard, &NumKeys);

    /* Text edit labels */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 0; wip->g.y = 0;
    wip->g.height = 20 ; wip->g.width = 480;
    wip->g.parent =gh.time;
    wip->text = "Year:\tMonth:        Day:          Hour:          Min:            Sec:";
    gh.setdatelabel = gwinLabelCreate(&go.setdatelabel, wip);

    /* Year text edit */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 0; wip->g.y = 20;
    wip->g.height = 40 ; wip->g.width = 60;
    wip->g.parent =gh.time;
    gh.setyear = gwinTexteditCreate(&go.setyear, wip, 4);
    gwinSetFont(gh.setyear, gdispOpenFont("DejaVuSans20"));
    gwinSetText(gh.setyear, "", TRUE);

    /* Month text edit */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 80; wip->g.y = 20;
    wip->g.height = 40 ; wip->g.width = 40;
    wip->g.parent =gh.time;
    gh.setmonth = gwinTexteditCreate(&go.setmonth, wip, 2);
    gwinSetFont(gh.setmonth, gdispOpenFont("DejaVuSans20"));
    gwinSetText(gh.setmonth, "", TRUE);

    /* Day text edit */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 140; wip->g.y = 20;
    wip->g.height = 40 ; wip->g.width = 40;
    wip->g.parent =gh.time;
    gh.setday = gwinTexteditCreate(&go.setday, wip, 2);
    gwinSetFont(gh.setday, gdispOpenFont("DejaVuSans20"));
    gwinSetText(gh.setday, "", TRUE);

    /* Hour text edit */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 200; wip->g.y = 20;
    wip->g.height = 40 ; wip->g.width = 40;
    wip->g.parent =gh.time;
    gh.sethour = gwinTexteditCreate(&go.sethour, wip, 2);
    gwinSetFont(gh.sethour, gdispOpenFont("DejaVuSans20"));
    gwinSetText(gh.sethour, "", TRUE);

    /* Min text edit */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 260; wip->g.y = 20;
    wip->g.height = 40 ; wip->g.width = 40;
    wip->g.parent =gh.time;
    gh.setmin = gwinTexteditCreate(&go.setmin, wip, 2);
    gwinSetFont(gh.setmin, gdispOpenFont("DejaVuSans20"));
    gwinSetText(gh.setmin, "", TRUE);

    /* Sec text edit */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 320; wip->g.y = 20;
    wip->g.height = 40 ; wip->g.width = 40;
    wip->g.parent =gh.time;
    gh.setsec = gwinTexteditCreate(&go.setsec, wip, 2);
    gwinSetFont(gh.setsec, gdispOpenFont("DejaVuSans20"));
    gwinSetText(gh.setsec, "", TRUE);

    /* Set button */
    gwinWidgetClearInit(wip);
    wip->g.show = TRUE;
    wip->g.x = 420; wip->g.y = 20;
    wip->g.height = 40 ; wip->g.width = 60;
    wip->g.parent =gh.time;
    wip->text = "Set";
    gh.setdatebtn = gwinButtonCreate(&go.setdatebtn, wip);
}

/** \brief Creates GUI, set default setting, create tabset object,
  *         time and SDC state labels.
  */
static void createGUI(void){
    GWidgetInit wi;

    gwinSetDefaultFont(gdispOpenFont("UI2"));
    gwinSetDefaultStyle(&WhiteWidgetStyle, FALSE);
    gdispClear(White);
    /* Time label init*/
    gwinWidgetClearInit(&wi);
    wi.g.show = TRUE;
    wi.g.x = 360; wi.g.y = 0;
    wi.g.height = GWIN_TABSET_TABHEIGHT-1 ; wi.g.width = 120;
    gh.date = gwinLabelCreate(&go.date, &wi);
    /* SDCard label init*/
    gwinWidgetClearInit(&wi);
    wi.g.show = TRUE;
    wi.g.x = 240; wi.g.y = 0;
    wi.g.height = GWIN_TABSET_TABHEIGHT-1 ; wi.g.width = 120;
    gh.sdc = gwinLabelCreate(&go.sdc, &wi);
    /* Tabset init */
    gwinWidgetClearInit(&wi);
    wi.g.show = TRUE;
    wi.g.x = 0; wi.g.y = 0;
    wi.g.height = gdispGetHeight(); wi.g.width = gdispGetWidth();
    gh.tabset = gwinTabsetCreate(&go.tabset, &wi, 0);
    gh.sterilizer = gwinTabsetAddTab(gh.tabset, "Sterilizer", FALSE);
    gh.result = gwinTabsetAddTab(gh.tabset, "Result", FALSE);
    gh.errors = gwinTabsetAddTab(gh.tabset, "Errors", FALSE);
    gh.time = gwinTabsetAddTab(gh.tabset, "Time", FALSE);
    createPageSterilizer(&wi);
    createPageTime(&wi);
    createPageErrors(&wi);
    createPageResult(&wi);

}

/** \brief Set human date into the RTC from time page.
  */
static void setHumanDate(void){
    struct HumanDate date;
    date.year = atoi(gwinGetText(gh.setyear));
    date.month = atoi(gwinGetText(gh.setmonth));
    date.day = atoi(gwinGetText(gh.setday));
    date.hour = atoi(gwinGetText(gh.sethour));
    date.min = atoi(gwinGetText(gh.setmin));
    date.sec = atoi(gwinGetText(gh.setsec));
    setDate(&date);
    gwinSetText(gh.setyear, "", TRUE);
    gwinSetText(gh.setmonth, "", TRUE);
    gwinSetText(gh.setday, "", TRUE);
    gwinSetText(gh.sethour, "", TRUE);
    gwinSetText(gh.setmin, "", TRUE);
    gwinSetText(gh.setsec, "", TRUE);
}

/** \brief Draws date.
  */
static void drawDate(void){
    getDateStr(gh.datestr, sizeof(gh.datestr));
    gwinSetText(gh.date, gh.datestr, FALSE);
}

/** \brief Draws temperatures.
  */
static void drawTempLables(void){
    uint8_t i;
    for(i=0; i<CHANNEL_NUM; i++){
        switch (appdata.sensorstate[i]){
            case SENSOR_INIT:   chsnprintf(gh.curr_tempstr[i], sizeof(gh.curr_tempstr[i]), "T%d: N/A", i);
                                break;
            case SENSOR_OK:     chsnprintf(gh.curr_tempstr[i], sizeof(gh.curr_tempstr[i]), "T%d: %3.1f C", i, (float)appdata.curr_temp[i]*SENSOR_TEMP_QUANTUM);
                                break;
            case SENSOR_ERROR:  chsnprintf(gh.curr_tempstr[i], sizeof(gh.curr_tempstr[i]), "T%d: Error", i);
                                break;
        }
        gwinSetText(gh.curr_temp[i], gh.curr_tempstr[i], FALSE);
    }
}

/** \brief Draws pwm channel duty cycles.
  */
static void drawHeatPower(void){
    uint8_t i;
    for (i=0; i<CHANNEL_NUM; i++){
        if (appdata.fuzzyreg_state != FUZZYREG_DISABLE){
            gwinProgressbarSetPosition(gh.heatpower[i], (appdata.dutycycle[i]/100)-1);
            gwinProgressbarIncrement(gh.heatpower[i]);
            gwinPrintg(gh.heatpower[i], "CH%d: %d%%", i, appdata.dutycycle[i]/100);
        }
        else{
            gwinProgressbarSetPosition(gh.heatpower[i], (appdata.dutycycle[i]/100)-1);
            gwinProgressbarIncrement(gh.heatpower[i]);
            gwinPrintg(gh.heatpower[i], "CH%d: %d%%", i, appdata.dutycycle[i]/100);
            gwinDisable(gh.heatpower[i]);
        }
    }
}

/** \brief Draws sterile temperatures number.
  */
static void drawSterileTemps(void){
    uint8_t num = gwinListItemCount(gh.res_list);
    gwinProgressbarSetPosition(gh.steriletemps, num-1);
    if (num)
        gwinProgressbarIncrement(gh.steriletemps);
    gwinPrintg(gh.steriletemps, "OK: %d/%d", gwinListItemCount(gh.res_list), RESULT_LIST_SIZE);
}

/** \brief Draws sterilization start time..
  */
static void drawResultStart(void){
    uint32_t sec = appdata.res_start.millisecond / 1000;
    gwinPrintg(gh.res_date, "Date: %d.%02d.%02d", appdata.res_start.year+1980, appdata.res_start.month, appdata.res_start.day);
    gwinPrintg(gh.res_begin, "Start: %02d:%02d:%02d", sec/3600,  (sec%3600/60), (sec%3600)%60);
    gwinPrintg(gh.final_result, "");
    gh.finalresstyle.background = White;
    gwinSetStyle(gh.final_result, &gh.finalresstyle);
    gwinPrintg(gh.res_end, "End:");
}

/** \brief Draws sterilization end time and final result.
  */
static void drawResultEnd(void){
    uint32_t sec = appdata.res_end / 1000;
    gwinPrintg(gh.res_end, "End: %02d:%02d:%02d", sec/3600,  (sec%3600/60), (sec%3600)%60);
    if (appdata.finalresult){
        gh.finalresstyle.background = Green;
        gwinPrintg(gh.final_result, "Result: SUCCESS");
    }
    else{
        gh.finalresstyle.background = Red;
        gwinPrintg(gh.final_result, "Result: FAILURE");
    }
    gwinSetStyle(gh.final_result, &gh.finalresstyle);
}

/** \brief Draws sterilizer state.
  */
static void drawSterilizerState(void){
    switch(appdata.ster_state){
        case STERILIZER_INIT: gwinPrintg(gh.ster_state, "State: Initalizing");
                                gh.statestyle.background = Gray;
                                gh.statestyle.enabled.text = White;
                                gwinDisable(gh.ster_stop);
                                gwinDisable(gh.ster_start);
                                gwinDisable(gh.res_print);
                                break;
        case STERILIZER_STOP: gwinPrintg(gh.ster_state, "State: Stop");
                                gh.statestyle.background = Gray;
                                gh.statestyle.enabled.text = White;
                                gwinDisable(gh.ster_stop);
                                gwinEnable(gh.ster_start);
                                gwinEnable(gh.res_print);
                                break;
        case STERILIZER_ACTIVE: gwinPrintg(gh.ster_state, "State: In Progress");
                                gh.statestyle.background = Yellow;
                                gh.statestyle.enabled.text = Black;
                                gwinDisable(gh.ster_start);
                                gwinEnable(gh.ster_stop);
                                gwinDisable(gh.res_print);
                                break;
        case STERILIZER_ERROR:  gwinPrintg(gh.ster_state, "State: Error");
                                gh.statestyle.background = Red;
                                gh.statestyle.enabled.text = White;
                                gwinDisable(gh.ster_start);
                                gwinDisable(gh.ster_stop);
                                break;
        case STERILIZER_SAVE:   gwinPrintg(gh.ster_state, "State: Save");
                                gh.statestyle.background = Yellow;
                                gh.statestyle.enabled.text = Black;
                                break;
        case STERILIZER_PRINT:  gwinPrintg(gh.ster_state, "State: Print");
                                gh.statestyle.background = Yellow;
                                gh.statestyle.enabled.text = Black;
                                gwinDisable(gh.res_print);
        default:                break;
    }
    gwinSetStyle(gh.ster_state, &gh.statestyle);
}

/** \brief Draws SDC state.
  */
static void drawSdcState(void){
    switch(appdata.sdc_state){
        case SDC_NOTINSERTED: gwinPrintg(gh.sdc, "SDCard: Not Inserted");
                            break;
        case SDC_ERROR:     gwinPrintg(gh.sdc, "SDCard: Error");
                            break;
        case SDC_BUSY:      gwinPrintg(gh.sdc, "SDCard: Busy");
                            break;
        case SDC_READY:     gwinPrintg(gh.sdc, "SDCard: Ready");
                            break;
        case SDC_FULL:      gwinPrintg(gh.sdc, "SDCard: Full");
        default:            break;
    }
}

/** \brief lcdcontrol thread function.
  *         - Handles drawing job queue.
  *         - Executes drawing jobs.
  *         - Handles button events.
  */
__attribute__((noreturn))
static THD_FUNCTION(Threadlcdcontrol, arg) {
    (void) arg;
    chRegSetThreadName("lcdcontrol");
    GEvent *pe;
    struct drawitem *jobptr;
    geventListenerInit(&gh.gbl);
    gwinAttachListener(&gh.gbl);
    createGUI();
    drawSterileTemps();
    while(TRUE) {
        drawDate();
        while(!STAILQ_EMPTY(&drawjobqueue.head)){
            jobptr = getDrawJob();
            if (!jobptr)
                break;
            else
                jobptr->func();
            freeDrawJob(jobptr);
        }
        pe = geventEventWait(&gh.gbl, MS2ST(10));
        if (pe){
            switch(pe->type){
                case GEVENT_GWIN_BUTTON:    if (((GEventGWinButton*)pe)->gwin == gh.ster_start)
                                                sendMailtoSterilizer(START_STERILIZER);
                                            if(((GEventGWinButton*)pe)->gwin == gh.ster_stop)
                                                sendMailtoSterilizer(STOP_STERILZER);
                                            if (((GEventGWinButton*)pe)->gwin == gh.setdatebtn)
                                                setHumanDate();
                                            if (((GEventGWinButton*)pe)->gwin == gh.res_print)
                                                sendMailtoSterilizer(PRINT_RESULT_LIST);
                                            break;
                case GEVENT_GWIN_TABSET:    drawSdcState();
                default:                    break;

            }
        }
    }
    chThdExit(1);
}

/*===========================================================================*/
/* Exported functions                                                        */
/*===========================================================================*/

/** \brief Set sensor state.
  *
  *  \param state   Pointer to array of sensor state, NULL save.
  */
void setSensorState(sensor_state_t *state){
    if (!state)
        return;
    uint8_t i;
    chMtxLock(&lcdmtx);
    for(i=0; i<CHANNEL_NUM; i++){
        appdata.sensorstate[i] = state[i];
    }
    chMtxUnlock(&lcdmtx);
    addDrawJob(drawTempLables);

}

/** \brief Displays current temperature.
  *
  *  \param temp   Pointer to temperature array, NULL save.
  */
void displayCurrentTemp(int16_t *temp){
    if (!temp)
        return;
    uint8_t i;
    chMtxLock(&lcdmtx);
    for(i=0; i<CHANNEL_NUM; i++){
        appdata.curr_temp[i] = temp[i];
    }
    chMtxUnlock(&lcdmtx);
    addDrawJob(drawTempLables);
}

/** \brief Displays pwm channels duty cycle.
  *
  *  \param dutycycle   Pointer to duty cycle array, NULL save.
  */
void displayHeatPower(pwmcnt_t *dutycycle){
    if (!dutycycle)
        return;
    uint8_t i;
    chMtxLock(&lcdmtx);
    for(i=0; i<CHANNEL_NUM; i++){
        appdata.dutycycle[i] = dutycycle[i];
    }
    chMtxUnlock(&lcdmtx);
    addDrawJob(drawHeatPower);
}

/** \brief Set fuzzy regulator state.
  *
  *  \param state   Pointer to fuzzy regulator state, NULL save.
  */
void setFuzzyregState(fuzzyreg_state_t *state){
    if (!state)
        return;
    chMtxLock(&lcdmtx);
    appdata.fuzzyreg_state = *state;
    chMtxUnlock(&lcdmtx);
}

/** \brief Displays sterilizer state
  *
  *  \param state   Pointer to sterilizer state,  NULL save.
  */
void displaySterilizerState(sterilizer_state_t *state){
    if (!state)
        return;
    chMtxLock(&lcdmtx);
    appdata.ster_state = *state;
    chMtxUnlock(&lcdmtx);
    addDrawJob(drawSterilizerState);
}

/** \brief Displays error list item.
  *
  *  \param item   Pointer to string of error list item, NULL save.
  */
void displayErrorListItem(char *item){
    if (!item)
        return;
    chMtxLock(&lcdmtx);
    gwinListAddItem(gh.err_list, item, FALSE);
    appdata.errlistsize++;
    chMtxUnlock(&lcdmtx);
}

/** \brief Displays result list item.
  *
  *  \param item   Pointer to string of result list item, NULL save.
  */
void displayResultListItem(char *item){
    if (!item)
        return;
    chMtxLock(&lcdmtx);
    gwinListAddItem(gh.res_list, item, FALSE);
    chMtxUnlock(&lcdmtx);
    addDrawJob(drawSterileTemps);
}

/** \brief Clears displayed result list.
  */
void destroyDisplayedResultList(void){
    chMtxLock(&lcdmtx);
    gwinListDeleteAll(gh.res_list);
    chMtxUnlock(&lcdmtx);
    addDrawJob(drawSterileTemps);
}

/** \brief Displays switch result page.
  */
void switchToresultPage(void){
    gwinTabsetSetTab(gh.result);
    addDrawJob(drawSdcState);
}

/** \brief Displays sterilization start time.
  *
  * \param start    Pointer to start time in RTCDateTIme structure format, NULL save.
  */
void displayResultStart(RTCDateTime *start){
    if (!start)
        return;
    chMtxLock(&lcdmtx);
    appdata.res_start = *start;
    chMtxUnlock(&lcdmtx);
    addDrawJob(drawResultStart);
}

/** \brief Displays sterilization end time and final result.
  *
  * \param endtime      Pointer to end time, NULL save.
  * \param finalresult  Bool variable if final result.
  */
void displayResultEnd(uint32_t *endtime, bool finalresult){
    if (!endtime)
        return;
    chMtxLock(&lcdmtx);
    appdata.res_end = *endtime;
    appdata.finalresult = finalresult;
    chMtxUnlock(&lcdmtx);
    addDrawJob(drawResultEnd);
}

/** \brief Displays SDC state.
  *
  * \param state    Pointer to sdc state, NULL save.
  */
void displaySdcState(sdc_state_t *state){
    if (!state)
        return;
    chMtxLock(&lcdmtx);
    appdata.sdc_state = *state;
    chMtxUnlock(&lcdmtx);
    addDrawJob(drawSdcState);
 }


/** \brief Drawing job queue user interface
  *
  */
void cmd_drawjob(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    chprintf(chp, "Drawing job queue size: %d\n\r", DRAW_JOB_QUEUE_SIZE);
    chprintf(chp, "FreeItems: %d\n\r", drawjobqueue.free_item);
    chprintf(chp, "Jobs: %d\n\r", drawjobqueue.itemnum);
    chprintf(chp, "Overflow: %d\n\r", drawjobqueue.overflow);
    chprintf(chp, "Underflow: %d\n\r", drawjobqueue.underflow);
    uint8_t i;
    for(i=0; i<CHANNEL_NUM; i++)
        chprintf(chp, "T%d/0: %3.1f\n\r", i, appdata.curr_temp[i]*SENSOR_TEMP_QUANTUM);
}

/** \brief Initializes lcdcontrol
  *             - Drawing job queue init.
  *             - Creates lcdcontrol thread.
  */
void lcdcontrolInit(void){
    bzero(&appdata, sizeof(appdata));
    drawjobQueueInit();
    chThdCreateStatic(waThreadlcdcontrol, sizeof(waThreadlcdcontrol), NORMALPRIO, Threadlcdcontrol, NULL);
}
