
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_voltageangle.c
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
#include <math.h>
#include "Rn7302.h"
#include "Rn7302File.h"
#include "algorithm_voltageangle.h"

#include "Rn7302_register.h"
#define ANGLE_MULTIPLE              100
#define ANGLE_J360_MAX              (360 * ANGLE_MULTIPLE)
#define ANGLE_CONVERT(x)            (((double)(x) * 360 * ANGLE_MULTIPLE / (1 << 24)) )

int32_t angle_calc(uint32_t value, int32_t angle)
{
    if (value) {
        return angle;
    }
    else {
        return 0;
    }
}

/* RN7302 相位寄存器 -> J_*（0.01 度，[0, 360)） */
int32_t angle_emu_calc(int32_t emu_y)
{
    uint32_t val = (uint32_t)ANGLE_CONVERT(emu_y);

    return (int32_t)((ANGLE_J360_MAX - val) % ANGLE_J360_MAX);
}

/*********************************************************************************************************
** Function name:       rms_algorithm_ActiverPowerJudge
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t rms_algorithm_Judge(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out)
{
    struct db_cfg_user_para         *pCfgUser = &(pMeasureBase->db_cfg.user);
    uint16_t                        wireMode               = 0;
    wireMode = (pCfgUser->base.WiringMode.value & 0x03);
    if(wireMode == P3W4)
    {
        if(out->Va == 0)
        {
            out->J_Va = 0;
        }
        if(out->Vb == 0)
        {
            out->J_Vb = 0;
        }
        if(out->Vc == 0)
        {
            out->J_Vc = 0;
        }
    }    
    else if(wireMode == P3W3)
    {
        if(out->Vab == 0)
        {
            out->J_Va = 0;
        }
        if(out->Vbc == 0)
        {
            out->J_Vb = 0;
        }
        if(out->Vca == 0)
        {
            out->J_Vc = 0;
        }
    }

    return 0;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_VoltageAngle
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_voltageangle(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out, void *input, uint32_t pt, uint32_t ct)
{
    uint16_t                             wireMode = 0;
    struct db_cfg_user_para             *pCfgUser = &(pMeasureBase->db_cfg.user);                        /* 获取接线模式 */
    struct rn7302_emu                          *pEmu = (struct rn7302_emu*)input;

    (void)pt;
    (void)ct;

    wireMode = (pCfgUser->base.WiringMode.value & 0x03);
    switch (wireMode)
    {
    case P3W3:
        {
            /* 角型模式下的电压角度在 linevoltage 中计算 */
            out->J_Va = 0;
            out->J_Vb = 0;
            out->J_Vc = 0;
            break;
        }
    case P3W4:
        {
            out->J_Va = angle_emu_calc(pEmu->YUA);
            out->J_Vb = angle_emu_calc(pEmu->YUB);
            out->J_Vc = angle_emu_calc(pEmu->YUC);
            rms_algorithm_Judge(pMeasureBase, out);
            break;
        }
    default:
        break;
    }

    return 0;
}

