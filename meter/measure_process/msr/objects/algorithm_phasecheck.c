
/*
* Copyright (c) 2019,珠海科技股份有限公司
* All rights reserved.
* 
* 文件名称：algorithm_phasecheck.c
* 文件标识：
* 描    述：
* 
* 当前版本：V1.0
* 作    者：raoy
* 更改说明：
* 完成日期：20190425
* 运行环境：
操作系统: 
硬件平台：
固件库  ：V3.1.2
* 修改记录 
*  作者       时间         版本      修改描述
*  raoy   2019.04.25       V1.00      创建
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h> 
#include <math.h>
#include "Rn7302.h"
#include "Rn7302File.h"
#include "db_struct.h"
#include "algorithm_phasecheck.h"

#define PHASE_NUM 48
const uint16_t phase[PHASE_NUM][3] = 
{
    {1,3,2},
    {2,3,1},
    {2,1,3},
    {3,1,2},
    {3,2,1},
    {1,2,3},
    {1,0x8003,2},
    {1,3,0x8002},
    {1,0x8003,0x8002},
    {0x8001,3,2},
    {0x8001,0x8003,2},
    {0x8001,3,0x8002},
    {0x8001,0x8003,0x8002},
    {2,0x8003,1},
    {2,3,0x8001},
    {2,0x8003,0x8001},
    {0x8002,3,1},
    {0x8002,0x8003,1},
    {0x8002,3,0x8001},
    {0x8002,0x8003,0x8001},
    {2,0x8001,3},
    {2,1,0x8003},
    {2,0x8001,0x8003},
    {0x8002,1,3},
    {0x8002,0x8001,3},
    {0x8002,1,0x8003},
    {0x8002,0x8001,0x8003},
    {3,0x8001,2},
    {3,1,0x8002},
    {3,0x8001,0x8002},
    {0x8003,1,2},
    {0x8003,0x8001,2},
    {0x8003,1,0x8002},
    {0x8003,0x8001,0x8002},
    {3,0x8002,1},
    {3,2,0x8001},
    {3,0x8002,0x8001},
    {0x8003,2,1},
    {0x8003,0x8002,1},
    {0x8003,2,0x8001},
    {0x8003,0x8002,0x8001},
    {1,0x8002,3},
    {1,2,0x8003},
    {1,0x8002,0x8003},
    {0x8001,2,3},
    {0x8001,0x8002,3},
    {0x8001,2,0x8003},
    {0x8001,0x8002,0x8003},
};


/*********************************************************************************************************
** Function name:       rms_algorithm_HandPhaseAdjust
** Descriptions:        手动相序调整
** input parameters:    
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void rms_Rn7302Write(uint16_t IChselValue)
{
    int32_t value;

    value = RN7302_CMD_WR_EN; //写使能位
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN,value);

    if(-1 != MeterChipReadReg(RN7302_REG_EMMCFG, RN7302_LEN_EMMCFG,  &value))
    {
        value = value | 0x20000;
        MeterChipWriteReg(RN7302_REG_EMMCFG, RN7302_LEN_EMMCFG,  value);
        
        value = RN7302_REG_TFEEC_EN_ENABLE;
        MeterChipWriteReg(RN7302_REG_TFEEC_EN, RN7302_LEN_TFEEC_EN, value);
        MeterChipWriteReg(RN7302_REG_ICHSEL, RN7302_LEN_ICHSEL, IChselValue);

        value = RN7302_CMD_WR_DIS; // 写保护
        MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN,value);
    }  
}

int32_t power_adjust(struct db_database_measure_base_type *pMeasureBase)
{
    struct db_basic_realtime_type  *pRealtimePrimary = &(pMeasureBase->db_measure.numeric.fullwave.primary);
    if(/*(pRealtimePrimary->P_IndCap == 0x00) && */((pRealtimePrimary->PFa >= 800) && (pRealtimePrimary->PFb >= 800) && (pRealtimePrimary->PFc >= 800)))
    {
        return 0;
    }
    return -1;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_AutoPhaseAdjust
** Descriptions:        自动相序调整
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_AutoPhaseAdjust(struct db_database_measure_base_type *pMeasureBase, int num)
{
    uint16_t    IChselValue = 0;
    struct db_cfg_user_para         *pCfgUser = &(pMeasureBase->db_cfg.user);
    
    IChselValue = (phase[num][0] & 0x07) | ((phase[num][1] & 0x07) << 2) | ((phase[num][2] & 0x07) << 4);
    rms_Rn7302Write(IChselValue);
    pCfgUser->base.PhaseAdjust1.value = phase[num][0];
    pCfgUser->base.PhaseAdjust2.value = phase[num][1];
    pCfgUser->base.PhaseAdjust3.value = phase[num][2];
    return num;    
}



/*********************************************************************************************************
** Function name:       rms_algorithm_HandPhaseAdjust
** Descriptions:        手动相序调整
** input parameters:    
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void rms_algorithm_HandPhaseAdjust(struct db_database_measure_base_type *pMeasureBase)
{
    Rn7302Init(pMeasureBase);
}



