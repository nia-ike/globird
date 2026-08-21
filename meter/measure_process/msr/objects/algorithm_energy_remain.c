/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_energy_remain.c
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
#include "Rn7302.h"
#include "Rn7302File.h"
#include "db_cfg.h"
#include <stdint.h>
#include <math.h>
#include "algorithm_energy_remain.h"
#include "msr_process.h"
#include "Rn7302_read.h"


#define ENERGY_PRECISION     10 // 1000     //电能保留三位小数

struct db_realtime_energy_type FullNetEnergy;
struct db_realtime_energy_type FundNetEnergy;


/*********************************************************************************************************
** Function name:       energy_remain_handle
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      0 - less than 0.001
                        1 -  >= 0.001
*********************************************************************************************************/
static int32_t energy_remain_handle(uint64_t *remain, uint64_t *net, uint32_t value, struct db_database_measure_base_type *pMeasureBase, uint32_t pt, uint32_t ct, int32_t startPQS)
{
    uint64_t ValueTmp = 0, RemainTmp = 0, NetTmp = 0;
    struct db_cfg_user_para     *pCfgUser = &(pMeasureBase->db_cfg.user);

    /* P/Q/S = 0, don't caculate the energy  */
    /*屏蔽在只有电压，没有电流情况下，运行一段时间后，rn7302的计量芯片中视在电度有值的情况*/
    if(startPQS == 0)
    {
        *net = 0;
        return 0;
    }
        
    if(pCfgUser->pulseout.pulse[0].constant.value == 0)
    {
        return 0;
    }
    /* 
        △E电能增量异常处理：
        脉冲阈值(每s) = ((220V * 500A * 3 / 1000)) * 3600imp/kWh / 3600s = 330个脉冲
        现msr_process的调度周期为250ms，按照10倍冗余脉冲阈值设计：
        脉冲阈值(每250ms+10倍冗余) = 330 / 4 * 10 = 825个脉冲
     */
    if(value > 825)
    {
        *net = 0;
        return 0;
    }

    //pt,ct的计算因子均为0.01；
    ValueTmp = (uint64_t)value * ct * pt;
    RemainTmp = ValueTmp + (*remain);
    *remain = (RemainTmp) % (ENERGY_PRECISION * pCfgUser->pulseout.pulse[0].constant.value);
    NetTmp = (RemainTmp) / (ENERGY_PRECISION * pCfgUser->pulseout.pulse[0].constant.value);
    *net = (uint64_t)NetTmp;

    if(NetTmp > 0)
        return 1;
    else 
        return 0;
}



/*********************************************************************************************************
** Function name:       
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_RemainEnergy(struct db_database_measure_base_type *pMeasureBase, struct db_realtime_energy_type *out,  void *input, void *pNet, uint32_t pt, uint32_t ct)
{
    struct db_cfg_user_para                    *pCfgUser = &(pMeasureBase->db_cfg.user);
    struct rn7302_emu_energy                 *pEmuEnergy = (struct rn7302_emu_energy*)input;
    struct db_realtime_energy_type           *pNetEnergy = (struct db_realtime_energy_type*)pNet;

    energy_remain_handle(&out->kWhImp[0],   &pNetEnergy->kWhImp[0],   pEmuEnergy->PosEPa, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Pa);
    energy_remain_handle(&out->kWhImp[1],   &pNetEnergy->kWhImp[1],   pEmuEnergy->PosEPb, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Pb);
    energy_remain_handle(&out->kWhImp[2],   &pNetEnergy->kWhImp[2],   pEmuEnergy->PosEPc, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Pc);
    energy_remain_handle(&out->kWhImp[3],   &pNetEnergy->kWhImp[3],   pEmuEnergy->PosEPt, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Ptot);
    energy_remain_handle(&out->kWhExp[0],   &pNetEnergy->kWhExp[0],   pEmuEnergy->NegEPa, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Pa);
    energy_remain_handle(&out->kWhExp[1],   &pNetEnergy->kWhExp[1],   pEmuEnergy->NegEPb, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Pb);
    energy_remain_handle(&out->kWhExp[2],   &pNetEnergy->kWhExp[2],   pEmuEnergy->NegEPc, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Pc);
    energy_remain_handle(&out->kWhExp[3],   &pNetEnergy->kWhExp[3],   pEmuEnergy->NegEPt, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Ptot);
    energy_remain_handle(&out->kvarhImp[0], &pNetEnergy->kvarhImp[0], pEmuEnergy->PosEQa, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Qa);
    energy_remain_handle(&out->kvarhImp[1], &pNetEnergy->kvarhImp[1], pEmuEnergy->PosEQb, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Qb);
    energy_remain_handle(&out->kvarhImp[2], &pNetEnergy->kvarhImp[2], pEmuEnergy->PosEQc, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Qc);
    energy_remain_handle(&out->kvarhImp[3], &pNetEnergy->kvarhImp[3], pEmuEnergy->PosEQt, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Qtot);
    energy_remain_handle(&out->kvarhExp[0], &pNetEnergy->kvarhExp[0], pEmuEnergy->NegEQa, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Qa);
    energy_remain_handle(&out->kvarhExp[1], &pNetEnergy->kvarhExp[1], pEmuEnergy->NegEQb, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Qb);
    energy_remain_handle(&out->kvarhExp[2], &pNetEnergy->kvarhExp[2], pEmuEnergy->NegEQc, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Qc);
    energy_remain_handle(&out->kvarhExp[3], &pNetEnergy->kvarhExp[3], pEmuEnergy->NegEQt, pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Qtot);
    energy_remain_handle(&out->kVAh[0],     &pNetEnergy->kVAh[0],     pEmuEnergy->ESa,    pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Sa);
    energy_remain_handle(&out->kVAh[1],     &pNetEnergy->kVAh[1],     pEmuEnergy->ESb,    pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Sb);
    energy_remain_handle(&out->kVAh[2],     &pNetEnergy->kVAh[2],     pEmuEnergy->ESc,    pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Sc);
    energy_remain_handle(&out->kVAh[3],     &pNetEnergy->kVAh[3],     pEmuEnergy->Estv,   pMeasureBase, pt, ct, pMeasureBase->db_measure.numeric.fullwave.primary.Stot);

    pNetEnergy->kWhTot[0]   = pNetEnergy->kWhImp[0]   + pNetEnergy->kWhExp[0];
    pNetEnergy->kWhTot[1]   = pNetEnergy->kWhImp[1]   + pNetEnergy->kWhExp[1];
    pNetEnergy->kWhTot[2]   = pNetEnergy->kWhImp[2]   + pNetEnergy->kWhExp[2];
    pNetEnergy->kWhTot[3]   = pNetEnergy->kWhImp[3]   + pNetEnergy->kWhExp[3];
    pNetEnergy->kvarhTot[0] = pNetEnergy->kvarhImp[0] + pNetEnergy->kvarhExp[0];
    pNetEnergy->kvarhTot[1] = pNetEnergy->kvarhImp[1] + pNetEnergy->kvarhExp[1];
    pNetEnergy->kvarhTot[2] = pNetEnergy->kvarhImp[2] + pNetEnergy->kvarhExp[2];
    pNetEnergy->kvarhTot[3] = pNetEnergy->kvarhImp[3] + pNetEnergy->kvarhExp[3];

    if((pCfgUser->base.WiringMode.value & 0x03) == P3W4)
    {
        pNetEnergy->kWhImp[3]   = pNetEnergy->kWhImp[0]   + pNetEnergy->kWhImp[1]   + pNetEnergy->kWhImp[2];
        pNetEnergy->kWhExp[3]   = pNetEnergy->kWhExp[0]   + pNetEnergy->kWhExp[1]   + pNetEnergy->kWhExp[2];
        pNetEnergy->kvarhImp[3] = pNetEnergy->kvarhImp[0] + pNetEnergy->kvarhImp[1] + pNetEnergy->kvarhImp[2];
        pNetEnergy->kvarhExp[3] = pNetEnergy->kvarhExp[0] + pNetEnergy->kvarhExp[1] + pNetEnergy->kvarhExp[2];
        pNetEnergy->kVAh[3]     = pNetEnergy->kVAh[0]     + pNetEnergy->kVAh[1]     + pNetEnergy->kVAh[2];
    }


    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
struct db_realtime_energy_type *get_FullNetEnergy()
{    
    return &FullNetEnergy;
}
/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
struct db_realtime_energy_type *get_FundNetEnergy()
{    
    return &FundNetEnergy;
}