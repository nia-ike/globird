
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_currentangle.c
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
#include "Rn7302_register.h"
#include "algorithm_currentangle.h"

#define ANGLE_MULTIPLE              100
#define ANGLE_CONVERT(x)            (((double)(x) * 360 * ANGLE_MULTIPLE/ (1 << 24)) )

typedef struct current_angle_type {
    char register_name[64];
    uint16_t phase_adjust;
    uint16_t offset;
}current_angle_type_t;



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
static int32_t rms_algorithm_Judge(void* pstParameters)
{
    struct db_basic_realtime_type         *pRealtime = (struct db_basic_realtime_type *)pstParameters;

    if(pRealtime->Ia == 0)
    {
        pRealtime->J_Ia = 0;
    }
    if(pRealtime->Ib == 0)
    {
        pRealtime->J_Ib = 0;
    }
    if(pRealtime->Ic == 0)
    {
        pRealtime->J_Ic = 0;
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       rms_algorithm_CurrentAngle
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_currentangle(struct db_database_measure_base_type  *pMeasureBase, struct db_basic_realtime_type *out,  void *input, uint32_t pt, uint32_t ct)
{
    uint16_t                          wireMode = 0;
    struct db_cfg_user_para          *pCfgUser = &(pMeasureBase->db_cfg.user);                        /* 获取接线模式 */
    struct rn7302_emu                          *pEmu = (struct rn7302_emu*)input;

    uint32_t  tmpVal = 0;
    wireMode = (pCfgUser->base.WiringMode.value & 0x03);
    switch (wireMode)
    {
        case P3W3:
        {
//            tmpVal = ANGLE_CONVERT(pEmu->YIA);
//            tmpVal = 36000 - tmpVal;
//            out->J_Ia = (uint32_t)(3000 + tmpVal) % 36000;
//
//            tmpVal = ANGLE_CONVERT(pEmu->YIB);
//            tmpVal = 36000 - tmpVal;
//            out->J_Ib = (uint32_t)(tmpVal) % 36000;
//
//            tmpVal = ANGLE_CONVERT(pEmu->YIC);
//            tmpVal = 36000 - tmpVal;
//            out->J_Ic = (uint32_t)(3000 + tmpVal) % 36000;
//
//            rms_algorithm_Judge(out);

            out->J_Ia = 0;
            out->J_Ib = 0;
            out->J_Ic = 0;
            break;    
        }
        case P3W4:
        {
            tmpVal = ANGLE_CONVERT(pEmu->YIA);
            tmpVal = 36000 - tmpVal;
            out->J_Ia = (uint32_t)(tmpVal) % 36000;

            tmpVal = ANGLE_CONVERT(pEmu->YIB);
            tmpVal = 36000 - tmpVal;
            out->J_Ib = (uint32_t)(tmpVal) % 36000;

            tmpVal = ANGLE_CONVERT(pEmu->YIC);
            tmpVal = 36000 - tmpVal;
            out->J_Ic = (uint32_t)(tmpVal) % 36000;

            rms_algorithm_Judge(out);
            break;
        }
        default:
            break;
    }

    return 0;
}

