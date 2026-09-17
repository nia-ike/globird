
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：msr_process.c
*  文件标识：
*  描    述：
*
*  当前版本：V1.0
*  作    者：raoy
*  运行环境：
*  操作系统: linux
*  硬件平台：f1c100s
*  固件库  ：
*  修改记录
*  作者       时间         版本      修改描述
*  raoy      2021       V1.00
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <math.h>
#include "Rn7302.h"
#include "Rn7302File.h"

#include "msr_process.h"
#include "db_struct.h"

#include "algorithm_fullwave.h"
#include "algorithm_fundwave.h"
#include "algorithm_halfwave.h"

#include "algorithm_powercaporind.h"
#include "algorithm_primary_to_secondary.h"

#include "algorithm_voltageangle.h"
#include "algorithm_currentangle.h"
#include "algorithm_demand.h"
#include "algorithm_demand_max.h"
#include "algorithm_unbalance.h"

#include "algorithm_demo.h"

#include "algorithm_powerquality_harmonic.h"
#include "algorithm_phasecheck.h"
#include "db_msr_msgevent_api.h"
#include "db_msgevent_api.h"
#include "algorithm_current_reverse.h"
#include "algorithm_powerquality_biased.h"
#include "algorithm_loadrun.h"

#include "algorithm_extremum.h"
#include "algorithm_halfwave.h"
#include "db_measure_info.h"

#include "debug.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>

#define TEMPERATURE_NUM 4
#define STATE_DEMO (1 << 0)      //demo模式
#define STATE_POWER_OFF (1 << 1) //掉电状态



uint16_t StateFlg = 0;
int waveform_fd = -1;



/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
** output parameters:   NONE
** Returned value:
*********************************************************************************************************/
//static void rn7302_ioctl_hardware_rst(int args)
//{
//    ioctl(args, RN7302_IOCTL_HARDWARE_RST, 0);
//}



/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
** output parameters:   NONE
** Returned value:
*********************************************************************************************************/
static void rms_algorithm_Rn7302(struct measure_ctx *pctx)
{
    struct db_database_type                        *pstVmem = (struct db_database_type *)pctx->pdb;
    struct db_database_measure_base_type         *pMeasure = &(pstVmem->db_measurebase[pctx->chip]);
    struct db_cfg_user_para                       *pCfgUser = &(pMeasure->db_cfg.user);
    struct db_basic_realtime_type                *pFullwave = &(pMeasure->db_measure.numeric.fullwave.primary);
    struct db_basic_realtime_type                *pFundwave = &(pMeasure->db_measure.numeric.fundwave.primary);
    struct db_powerquality_basic_type    *pPowerqualiteBase = &(pMeasure->db_measure.powerquality.basic);
    struct db_powerquality_unbalance_type       *pUnbalance = &(pMeasure->db_measure.powerquality.basic.unbalance);

    struct db_statistical_loadrun_type            *pLoadRun = &(pMeasure->db_measure.statistical.loadrun);
    struct db_statistical_extremum_type          *pExtremum = &(pMeasure->db_measure.statistical.extremum);
//    struct db_demand_base_type                     *pDemand = &(pMeasure->db_measure.statistical.demand.realtime);
    struct db_demand_max_type                   *pDemandMax = &(pMeasure->db_measure.statistical.demand.max);
    struct db_realtime_secondary_type   *pRealtimeSecondary = &(pMeasure->db_measure.numeric.fullwave.secondary);
    struct db_realtime_secondary_type  *pFunRealtimeSecondary = &(pMeasure->db_measure.numeric.fundwave.secondary);

    uint32_t                    ct = 1;
    uint32_t                    pt = 1;
    pt = pCfgUser->base.PT.value;
    ct = pCfgUser->base.CT.value;


    if (-1 == (Rn7302CheckProcess(pMeasure)))
    {
        //rn7302_ioctl_hardware_rst(fd);
        Rn7302Init(pMeasure);
        printf("RN7302 CHANGE!\n");
        return;
    }
    DEBUG_PRINT(DEBUG_OFF, "%s %d\r\n", __FILE__, __LINE__);

    if(1 != Rn7302_Config(pMeasure, MSR_TYPE_RN7302_GET_INIT_STATE, 0))
    {
        return;
    }
    if(pctx->test != 1)
    {
        rms_algorithm_fullwave(        pMeasure,         pFullwave,  pt,   ct);
        rms_algorithm_Primary_to_secondary(pMeasure, pRealtimeSecondary,    pt,   ct);
        rms_algorithm_fundwave(        pMeasure,         pFundwave,  pt,   ct);
        rms_algorithm_fundwave_secondary(pMeasure, pFunRealtimeSecondary,   pt,   ct);
        rms_algorithm_CurrentReverse(  pMeasure,         pFullwave,  pt,   ct);
        rms_algorithm_CurrentReverse(  pMeasure,         pFundwave,  pt,   ct);
        rms_algorithm_PowerCapOrInd(   pMeasure,         pFullwave,  pt,   ct);
        rms_algorithm_dmd_process(     pMeasure);
        rms_algorithm_demand_max(      pMeasure,        pDemandMax,  pt,   ct);
        rms_algorithm_extremum(        pMeasure,         pExtremum,  pt,   ct);
        rms_algorithm_biased(          pMeasure, pPowerqualiteBase,  pt,   ct);
        rms_algorithm_Unbalance(       pMeasure,        pUnbalance,         pt,   ct);
        rms_algorithm_loadrun(         pMeasure,          pLoadRun,  pt,   ct);
    }
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void *msr_thread_proc(struct measure_ctx *pctx)
{
    uint16_t ret;
    uint64_t TimeNum = 0;
    uint16_t PhaseAdjustNum = 0;
    struct db_database_type                        *pstVmem = (struct db_database_type *)pctx->pdb;
//    struct db_database_expand_type                 *pExpand = &(pstVmem->db_expand);
    struct db_database_measure_base_type          *pMeasure = &(pstVmem->db_measurebase[pctx->chip]);
    struct db_cfg_user_para                       *pCfgUser = &(pMeasure->db_cfg.user);
    struct db_basic_realtime_type          *pMeasurePrimary = &(pMeasure->db_measure.numeric.fullwave.primary);
    struct db_realtime_secondary_type   *pRealtimeSecondary = &(pMeasure->db_measure.numeric.fullwave.secondary);
    struct db_basic_realtime_type             *pMeasureHalf = &(pMeasure->db_measure.numeric.halfwave.primary);
    struct db_msr_msg_evt_general_t msg;
    uint16_t    hw = 0;
    uint32_t    ct = 1;
    uint32_t    pt = 1;
    uint32_t    refreshNum = 1;
    pt = pCfgUser->base.PT.value;
    ct = pCfgUser->base.CT.value;
    // 使用已经打开的设备文件描述符，避免重复打开同一设备
    waveform_fd = MeterChipGetFd();
    if (waveform_fd < 0) {
        printf("Device not initialized, cannot get device file descriptor\n");
        return NULL;
    }
    while (1)
    {
        /* block for event */
        ret = db_msr_msg_event_api_pend(pctx->chip, &msg);
        switch (msg.type)
        {
        case DB_MSR_EVENT_TYPE_ADJUST:
            Rn7302Adjust(pMeasure);
            db_msg_event_cmd(DB_EVENT_TYPE_SYS_RAM_TO_JSON);
            break;
        case DB_MSR_EVENT_TYPE_ADJUST_OFFSET:
            Rn7302Adjust_Offset_I(pMeasure);
            db_msg_event_cmd(DB_EVENT_TYPE_SYS_RAM_TO_JSON);
            break;
        case DB_MSR_EVENT_TYPE_DEMO:
            TimeNum = 2592000; //进入demo模式下，1小时后自动退出demo模式
            pMeasurePrimary->DemoFlag = 1;
            rms_algorithm_Demo(pMeasure);
            break;
        case DB_MSR_EVENT_TYPE_TIMER:
            refreshNum = (1000/pctx->refresh);
            refreshNum = refreshNum == 0 ? 1 : refreshNum;     //  halfwave = 10ms,  so HZ = 100
            hw++;

            if ((hw % HALFWAVE_NUM) == 1)
            {
                db_measure_info_halfwave_start(pMeasure);
            }
            rms_algorithm_halfwave(pMeasure, pMeasureHalf, hw);
            if ((hw % HALFWAVE_NUM) == 0)
            {
                db_measure_info_halfwave_done(pMeasure);
            }

            if((hw % refreshNum) == 0)
            {
                hw = 0;
                if (TimeNum == 0)
                {
                    db_measure_info_full_cycle_start(pMeasure);
                    rms_algorithm_Rn7302(pctx);
                    rms_algorithm_Primary_to_secondary(pMeasure,pRealtimeSecondary,pt,ct);
                    db_measure_info_full_cycle_done(pMeasure);
                    pMeasurePrimary->DemoFlag = 1;
                }
                else
                {
                    TimeNum--;
                }
            }
            else
            {
            }
            break;
        case DB_MSR_EVENT_TYPE_REINIT:
            Rn7302_Config(pMeasure, MSR_TYPE_RN7302_REINIT, 0);
            break;
        case DB_MSR_EVENT_TYPE_MAXDMD_DEFAULT:
            DemandMaxClear(pMeasure);
            break;
        case DB_MSR_EVENT_TYPE_EXTREME_DEFAULT:
            extreme_clear((uint8_t *)pMeasure);
            break;
        case DB_MSR_EVENT_TYPE_DEMO_END:
            TimeNum = 0; //退出demo模式
            pMeasurePrimary->DemoFlag = 1;
            break;
        case DB_MSR_EVENT_TYPE_ADJUST_RST:
            printf("msr_thread_proc DB_MSR_EVENT_TYPE_ADJUST_RST\n");
            Rn7302_Config(pMeasure, MSR_TYPE_RN7302_ADJUST_RST, 0);
            break;
        default:
            break;
        }
    }

    return NULL;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void *RMSTimerThread(void *param)
{
    struct measure_ctx *pctx = (struct measure_ctx *)param;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    /* Attempt to open non blocking the access dev */
    while (1)
    {
        db_msr_msg_event_cmd(pctx->chip, DB_MSR_EVENT_TYPE_TIMER);

        usleep(pctx->refresh * 1000);
    }

    return NULL;
}



/*********************************************************************************************************
** Function name:       msr_compat_old_version_adjust_config
** Descriptions:        兼容老版本配置：老版本选项是100A，功率校正是10A，新版本选项为5A，功率校正是5A校准
** input parameters:    pCfgSys - 系统配置参数指针
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void msr_compat_old_version_adjust_config(struct db_cfg_sys_para *pCfgSys)
{
    if(pCfgSys->adjust.Pa_PhsL.value != 0)
    {
        pCfgSys->adjustAdvalue.Adjust_V.value = 220;
        pCfgSys->adjustAdvalue.Adjust_I.value = 10;
        pCfgSys->adjustAdvalue.Adjust_P.value = 2200;
        pCfgSys->adjustAdvalue.Adjust_Ia.value = 10000000;
        pCfgSys->adjustAdvalue.Adjust_Ib.value = 10000000;
        pCfgSys->adjustAdvalue.Adjust_Ic.value = 10000000;
        pCfgSys->adjustAdvalue.Adjust_Pa.value = 13113022 * 2;
        pCfgSys->adjustAdvalue.Adjust_Pb.value = 13113022 * 2;
        pCfgSys->adjustAdvalue.Adjust_Pc.value = 13113022 * 2;
        pCfgSys->interface.RatedCurrent.value = CURRENT_RATED_K100;
    }
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
** output parameters:   NONE
** Returned value:
*********************************************************************************************************/
int32_t msr_init(struct measure_ctx *pctx)
{
    pthread_t m_hTimerThread;
    struct db_database_type   *pstVmem = (struct db_database_type *)pctx->pdb;
    struct db_database_measure_base_type    *pMeasure = &(pstVmem->db_measurebase[pctx->chip]);
    struct db_cfg_sys_para              *pCfgSys = &(pMeasure->db_cfg.sys);

    /*兼容 老版本选项是100A，功率校正是10A，新版本选项为5A，功率校正是5A校准*/
    msr_compat_old_version_adjust_config(pCfgSys);

    /* 初始化rn7302 */
    Rn7302HFConstSet(pMeasure);
    Rn7302ADCCFGSet(pMeasure);
    Rn7302PHSIxSet(pMeasure);
    Rn7302Init(pMeasure);
    rms_algorithm_dmd_init(pMeasure);
    LoadrunInit(pMeasure);
    db_measure_info_init(&pMeasure->info);
    /* 初始化定时器线程 */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (!pthread_create(&m_hTimerThread, &attr, RMSTimerThread, pctx) == 0)
    {
        m_hTimerThread = 0;
        printf("Create Timer Thread failed.\n");
    }
    /* 初始化谐波运算：SPI0 主动采集芯片波形缓存（一周波），与 HSDC/spiwave 无关 */
    msr_harmonic_init(pctx);

    return 0;
}
