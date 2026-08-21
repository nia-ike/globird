
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
#define ANGLE_CONVERT(x)            (((double)(x) * 360 * ANGLE_MULTIPLE / (1 << 24)) )

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
    uint16_t                               tmpVal = 0;
    int32_t                                 value;
    struct db_cfg_user_para             *pCfgUser = &(pMeasureBase->db_cfg.user);                        /* 获取接线模式 */
    struct rn7302_emu                          *pEmu = (struct rn7302_emu*)input;

    wireMode = (pCfgUser->base.WiringMode.value & 0x03);
    switch (wireMode)
    {
    case P3W3:
        {
            out->J_Va = 0;
            out->J_Vb = 0;
            out->J_Vc = 0;
//            tmpVal = ANGLE_CONVERT(pEmu->YUA);
//            tmpVal = 36000 - tmpVal;
//            out->J_Va = (uint32_t)(3000 + tmpVal) % 36000;
//            //out->J_Va = (uint32_t)tmpVal % 36000;
//
//            tmpVal = ANGLE_CONVERT(pEmu->YUB);
//            tmpVal = 36000 - tmpVal;
//            out->J_Vb = (uint32_t)(36000 - tmpVal) % 36000;
//            //out->J_Vb = (uint32_t)tmpVal % 36000;
//            
//            tmpVal = ANGLE_CONVERT(pEmu->YUC);
//            tmpVal = 36000 - tmpVal;
//            out->J_Vc = (uint32_t)(3000 + tmpVal) % 36000;
//            //out->J_Vc = (uint32_t)tmpVal % 36000;
//            rms_algorithm_Judge(pMeasureBase, out);
            break;  
        }  
    case P3W4:
        {
            tmpVal = ANGLE_CONVERT(pEmu->YUA);
            tmpVal = 36000 - tmpVal;
            out->J_Va = tmpVal % 36000;

            tmpVal = ANGLE_CONVERT(pEmu->YUB);
            tmpVal = 36000 - tmpVal;
            out->J_Vb =  tmpVal % 36000;

            tmpVal = ANGLE_CONVERT(pEmu->YUC);
            tmpVal = 36000 - tmpVal;
            out->J_Vc = tmpVal % 36000;
            rms_algorithm_Judge(pMeasureBase, out);
            break;    
        }
    default:
        break;
    }

/*


    printf("J_Va: %d \n",out->db_realtime_other.J_Va);
    printf("J_Vb: %d \n",out->db_realtime_other.J_Vb);
    printf("J_Vc: %d \n",out->db_realtime_other.J_Vc);
*/   
    return 0;
}

