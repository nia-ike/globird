/*************************************************
  Copyright (C), 2003-2017, pilot Co., Ltd.
  File name:      Rn7302.h
  Author:       longzh    Version: 1.0       Date: 2017-6-9
  Description:  Rn7302 ����
  Others:         
  History:              
*************************************************/
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h> 
#include "math.h"
#include "Rn7302.h"
#include "Rn7302File.h"
#include <sys/time.h>
#include "debug.h"
#include "db_struct.h"
#include "algorithm_phasecheck.h"
#include "msr_process.h"
#include "algorithm_energy_remain.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#define    STATE_DEMO    (1 << 0) //demoģʽ
#define    STATE_POWER_OFF    (1 << 1) //����״̬
  
#define    READ_ADJUST_CNT_MAX    10
#define    PQ_DEAD_ZONE    5     //�й��޹���������
#define    VOLTAGE_TIMES    100
#define    CURRENT_TIMES    10000
#define    PI               3.1415926f
#define PLUSE_CONSTANT       3600




/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int32_t Rn7302Adjust_Default(struct db_database_measure_base_type *pMeasureBase)
{
    int factor = 0;
    int temp = 0;
    temp = RN7302_CMD_WR_EN; //写使能位
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);           
    //将默认系数写入CE    
    MeterChipWriteReg(RN7302_REG_GSUA, RN7302_LEN_GSUA, factor);
    MeterChipWriteReg(RN7302_REG_GSUB, RN7302_LEN_GSUB, factor);
    MeterChipWriteReg(RN7302_REG_GSUC, RN7302_LEN_GSUC, factor);  
    MeterChipWriteReg(RN7302_REG_GSIA, RN7302_LEN_GSIA, factor);
    MeterChipWriteReg(RN7302_REG_GSIB, RN7302_LEN_GSIB, factor);
    MeterChipWriteReg(RN7302_REG_GSIC, RN7302_LEN_GSIC, factor);
    MeterChipWriteReg(RN7302_REG_GSIN, RN7302_LEN_GSIN, factor);

    // 扩展位
    temp = (RN7302_CMD_WR_EN << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, temp);

#if (PHASE_ADJUST_SEL == PHASE_CHANNEL)
    // 相位
    MeterChipWriteReg(RN7302_REG_PHSUA, RN7302_LEN_EX_PHSUA, 0xa7 << 24);
    MeterChipWriteReg(RN7302_REG_PHSUB, RN7302_LEN_EX_PHSUB, 0xa7 << 24);
    MeterChipWriteReg(RN7302_REG_PHSUC, RN7302_LEN_EX_PHSUC, 0xa7 << 24);

    MeterChipWriteReg(RN7302_REG_PHSUA, RN7302_LEN_EX_PHSUA, (0xa7 << 24) | 0x200);
    MeterChipWriteReg(RN7302_REG_PHSUB, RN7302_LEN_EX_PHSUB, (0xa7 << 24) | 0x200);
    MeterChipWriteReg(RN7302_REG_PHSUC, RN7302_LEN_EX_PHSUC, (0xa7 << 24) | 0x200);

    MeterChipWriteReg(RN7302_REG_PHSIA, RN7302_LEN_EX_PHSIA, 0x20080200);
    MeterChipWriteReg(RN7302_REG_PHSIB, RN7302_LEN_EX_PHSIB, 0x20080200);
    MeterChipWriteReg(RN7302_REG_PHSIC, RN7302_LEN_EX_PHSIC, 0x20080200);

    MeterChipWriteReg(RN7302_REG_PRTH1L, RN7302_LEN_PRTH1L, 0);


#endif    
    temp = (RN7302_CMD_WR_DIS << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, temp);

    //全波
    MeterChipWriteReg(RN7302_REG_PA_PHSL, RN7302_LEN_PA_PHSL, factor);
    MeterChipWriteReg(RN7302_REG_PB_PHSL, RN7302_LEN_PB_PHSL, factor);
    MeterChipWriteReg(RN7302_REG_PC_PHSL, RN7302_LEN_PC_PHSL, factor);
    MeterChipWriteReg(RN7302_REG_QA_PHSL, RN7302_LEN_QA_PHSL, factor);
    MeterChipWriteReg(RN7302_REG_QB_PHSL, RN7302_LEN_QB_PHSL, factor);
    MeterChipWriteReg(RN7302_REG_QC_PHSL, RN7302_LEN_QC_PHSL, factor);
    //基波
    MeterChipWriteReg(RN7302_REG_FPA_PHS, RN7302_LEN_FPA_PHS, factor);
    MeterChipWriteReg(RN7302_REG_FPB_PHS, RN7302_LEN_FPB_PHS, factor);
    MeterChipWriteReg(RN7302_REG_FPC_PHS, RN7302_LEN_FPC_PHS, factor);
    MeterChipWriteReg(RN7302_REG_FQA_PHS, RN7302_LEN_FQA_PHS, factor);
    MeterChipWriteReg(RN7302_REG_FQB_PHS, RN7302_LEN_FQB_PHS, factor);
    MeterChipWriteReg(RN7302_REG_FQC_PHS, RN7302_LEN_FQC_PHS, factor);

    temp = RN7302_CMD_WR_DIS; // 写保护
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);
    return 0;
}

/*************************************************************************/
/** Function Name:     Rn7302Adjust_Offset_I                            **/
/** Purpose:                                                            **/
/** Params:            None.                                            **/
/** Return:            None.                                            **/
/** Notice:            None.                                            **/
/*************************************************************************/
int32_t Rn7302Adjust_Offset_I(struct db_database_measure_base_type *pMeasureBase)
{
#define I_READTIMES    (10)

    uint16_t    factor_offsetI[3] = {0,0,0};
    int32_t     temp = 0;
    int32_t     s32ValueTmpIa = 0;
    int32_t     s32ValueTmpIb = 0;
    int32_t     s32ValueTmpIc = 0;
    int32_t     s32Tmp = 0;

    struct db_cfg_sys_para               *pCfgSys = &pMeasureBase->db_cfg.sys;

    temp = RN7302_CMD_WR_EN; //写使能位
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);

    //电流有效值offset
    MeterChipWriteReg(RN7302_REG_IA_OS, RN7302_LEN_IA_OS,  factor_offsetI[0]);
    MeterChipWriteReg(RN7302_REG_IB_OS, RN7302_LEN_IB_OS,  factor_offsetI[1]);
    MeterChipWriteReg(RN7302_REG_IC_OS, RN7302_LEN_IC_OS,  factor_offsetI[2]);

    temp = RN7302_CMD_WR_DIS; //写保护
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);
    MSR_PORT_DELAY(1000);

    for(int i = 0; i < I_READTIMES; i++)
    {
        MeterChipReadReg(RN7302_REG_IA, RN7302_LEN_IA,  &s32Tmp);
        s32ValueTmpIa += s32Tmp;
        MeterChipReadReg(RN7302_REG_IB, RN7302_LEN_IB,  &s32Tmp);
        s32ValueTmpIb += s32Tmp;
        MeterChipReadReg(RN7302_REG_IC, RN7302_LEN_IC,  &s32Tmp);
        s32ValueTmpIc += s32Tmp;
		MSR_PORT_DELAY(250);
    }

    s32ValueTmpIa /= I_READTIMES;
    s32ValueTmpIb /= I_READTIMES;
    s32ValueTmpIc /= I_READTIMES;

    s32ValueTmpIa = -(((s32ValueTmpIa * s32ValueTmpIa) >> 14) & 0xFFFF);
    s32ValueTmpIb = -(((s32ValueTmpIb * s32ValueTmpIb) >> 14) & 0xFFFF);
    s32ValueTmpIc = -(((s32ValueTmpIc * s32ValueTmpIc) >> 14) & 0xFFFF);

    factor_offsetI[0] = (int16_t)s32ValueTmpIa;
    factor_offsetI[1] = (int16_t)s32ValueTmpIb;
    factor_offsetI[2] = (int16_t)s32ValueTmpIc;

    temp = RN7302_CMD_WR_EN; //写使能位
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);

    MeterChipWriteReg(RN7302_REG_IA_OS, RN7302_LEN_IA_OS,  factor_offsetI[0]);
    MeterChipWriteReg(RN7302_REG_IB_OS, RN7302_LEN_IB_OS,  factor_offsetI[1]);
    MeterChipWriteReg(RN7302_REG_IC_OS, RN7302_LEN_IC_OS,  factor_offsetI[2]);

    temp = RN7302_CMD_WR_DIS; //写保护
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);

    pCfgSys->adjust.Offset_Ia.value = factor_offsetI[0];
    pCfgSys->adjust.Offset_Ib.value = factor_offsetI[1];
    pCfgSys->adjust.Offset_Ic.value = factor_offsetI[2];

    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int32_t Rn7302Adjust_UI(struct db_database_measure_base_type *pMeasureBase)
{
    uint16_t factorU[3] = {0,0,0},factorI[4] = {0,0,0,0};
    int32_t s32Tmp = 0;
    int temp = 0;
    double Err = 0;
    
    struct db_cfg_sys_para               *pCfgSys = &(pMeasureBase->db_cfg.sys);
    
    //校准三相电压
    if(-1 != MeterChipReadReg(RN7302_REG_UA, RN7302_LEN_UA, &s32Tmp))
    {
        Err = ((double)s32Tmp - pCfgSys->adjustAdvalue.Adjust_Va.value) / pCfgSys->adjustAdvalue.Adjust_Va.value;
        Err = (double) ((-Err) / (1 + Err));
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorU[0] = (int16_t)Err ;
    }

    if(-1 != MeterChipReadReg(RN7302_REG_UB, RN7302_LEN_UB, &s32Tmp))
    {
        Err = ((double)s32Tmp - pCfgSys->adjustAdvalue.Adjust_Vb.value) / pCfgSys->adjustAdvalue.Adjust_Vb.value;
        Err = (double) ((-Err) / (1 + Err));
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorU[1] = (int16_t)Err ;
    }

    if(-1 != MeterChipReadReg(RN7302_REG_UC, RN7302_LEN_UC, &s32Tmp))
    {
        Err = ((double)s32Tmp - pCfgSys->adjustAdvalue.Adjust_Vc.value) / pCfgSys->adjustAdvalue.Adjust_Vc.value;
        Err = (double) ((-Err) / (1 + Err));
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorU[2] = (int16_t)Err ;
    }
    
    //校准三相电流
    if(-1 != MeterChipReadReg(RN7302_REG_IA, RN7302_LEN_IA, &s32Tmp))
    {
        Err = ((double)s32Tmp * 1 - pCfgSys->adjustAdvalue.Adjust_Ia.value) / pCfgSys->adjustAdvalue.Adjust_Ia.value;
        Err = (double) ((-Err) / (1 + Err));
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorI[0] = (int16_t)Err ;
    }

    if(-1 != MeterChipReadReg(RN7302_REG_IB, RN7302_LEN_IB, &s32Tmp))
    {
        Err = ((double)s32Tmp * 1 - pCfgSys->adjustAdvalue.Adjust_Ib.value) / pCfgSys->adjustAdvalue.Adjust_Ib.value;
        Err = (double) ((-Err) / (1 + Err));
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorI[1] = (int16_t)Err ;
    }

    if(-1 != MeterChipReadReg(RN7302_REG_IC, RN7302_LEN_IC, &s32Tmp))
    {
        Err = ((double)s32Tmp * 1 - pCfgSys->adjustAdvalue.Adjust_Ic.value) / pCfgSys->adjustAdvalue.Adjust_Ic.value;
        Err = (double) ((-Err) / (1 + Err));
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorI[2] = (int16_t)Err ;
    }

//    // 剩余电流固定使用 2000:1  15欧姆
//    if(-1 != MeterChipReadReg(RN7302_REG_IN, RN7302_LEN_IN, &s32Tmp))
//    {
//        Err = ((double)s32Tmp * 1 - C5_ADJ_CUR_VAL_5A) / (C5_ADJ_CUR_VAL_5A);
//        Err = (double) ((-Err) / (1 + Err));
//        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
//        factorI[3] = (int16_t)Err ; 
//    }
//
    temp = RN7302_CMD_WR_EN; //写使能位
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);

    MeterChipWriteReg(RN7302_REG_GSUA, RN7302_LEN_GSUA, factorU[0]);//电压
    MeterChipWriteReg(RN7302_REG_GSUB, RN7302_LEN_GSUB, factorU[1]);
    MeterChipWriteReg(RN7302_REG_GSUC, RN7302_LEN_GSUC, factorU[2]);  
    MeterChipWriteReg(RN7302_REG_GSIA, RN7302_LEN_GSIA, factorI[0]);//电流
    MeterChipWriteReg(RN7302_REG_GSIB, RN7302_LEN_GSIB, factorI[1]);
    MeterChipWriteReg(RN7302_REG_GSIC, RN7302_LEN_GSIC, factorI[2]);
//    MeterChipWriteReg(RN7302_REG_GSIN, RN7302_LEN_GSIN, factorI[3]);

    temp = RN7302_CMD_WR_DIS; //写保护
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);

    pCfgSys->adjust.GsUa.value = factorU[0];
    pCfgSys->adjust.GsUb.value = factorU[1];
    pCfgSys->adjust.GsUc.value = factorU[2];
    pCfgSys->adjust.GsIa.value = factorI[0];
    pCfgSys->adjust.GsIb.value = factorI[1];
    pCfgSys->adjust.GsIc.value = factorI[2];
    pCfgSys->adjust.GsIn.value = factorI[3];
//    pCfgSys->adjust.I0Adj.value = factorI[3];   

    return 0;

}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int32_t Rn7302Adjust_Angle(struct db_database_measure_base_type *pMeasureBase)
{
    uint32_t factorIAngle[3] = {0x200,0x200,0x200};
    uint32_t factorUAngle[3] = {0x200,0x200,0x200};
    int32_t s32TmpP;
    double Err ;
    int temp;
    struct db_cfg_sys_para *pCfgSys = &(pMeasureBase->db_cfg.sys);

    if(-1 != MeterChipReadReg(RN7302_REG_PA, RN7302_LEN_PA, &s32TmpP))
    {
        /* Angle */
        Err = ((double)s32TmpP - (double)pCfgSys->adjustAdvalue.Adjust_Pa.value / 2.0) / ((double)pCfgSys->adjustAdvalue.Adjust_Pa.value / 2.0);
        Err = asin((-Err/1.73205080756887)) * 180 / 3.1415926;
        temp = 0x200 - (int16_t)(Err/0.008789 + 0.5);
        factorIAngle[0] = temp | (temp << 10) | (temp << 20);
    }

    if(-1 != MeterChipReadReg(RN7302_REG_PB, RN7302_LEN_PB, &s32TmpP))
    {
        /* Angle */
        Err = ((double)s32TmpP - (double)pCfgSys->adjustAdvalue.Adjust_Pb.value / 2.0) / ((double)pCfgSys->adjustAdvalue.Adjust_Pb.value / 2.0);
        Err = asin((-Err/1.73205080756887)) * 180 / 3.1415926;
        temp = 0x200 - (int16_t)(Err/0.008789 + 0.5);
        factorIAngle[1] = temp | (temp << 10) | (temp << 20);
    }

    if(-1 != MeterChipReadReg(RN7302_REG_PC, RN7302_LEN_PC, &s32TmpP))
    {
        /* Angle */
        Err = ((double)s32TmpP - (double)pCfgSys->adjustAdvalue.Adjust_Pc.value / 2.0) / ((double)pCfgSys->adjustAdvalue.Adjust_Pc.value / 2.0);
        Err = asin((-Err/1.73205080756887)) * 180 / 3.1415926;
        temp = 0x200 - (int16_t)(Err/0.008789 + 0.5);
        factorIAngle[2] = temp | (temp << 10) | (temp << 20);
    }

    temp = RN7302_CMD_WR_EN; // 写使能位
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);    

    // 通道相位校正
    // 扩展位
    temp = (RN7302_CMD_WR_EN << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, temp);    


    /*计算相间角偏差。上面功率计算只是计算出同相角差相间角误委存在*/
    int jba, jca;
    jba =((float)(pMeasureBase->db_measure.numeric.fullwave.primary.J_Vb - 24000) / 0.8789);
    jca =((float)(pMeasureBase->db_measure.numeric.fullwave.primary.J_Vc - 12000) / 0.8789);
    /*
    通道相位校正
    操作步骤：首先将 PHSUA[31:0]写入 0xA700_0000，然后再写入相位校正值（比如 PHSUA[31:0]=0xA700_0200），注意要分两次操作。
    电压通道没有校准，直接写入 0x200 为相位偏移中间值
    */
    // 相位
    MeterChipWriteReg(RN7302_REG_PHSUA, RN7302_LEN_EX_PHSUA, 0xa7 << 24);
    MeterChipWriteReg(RN7302_REG_PHSUB, RN7302_LEN_EX_PHSUB, 0xa7 << 24);
    MeterChipWriteReg(RN7302_REG_PHSUC, RN7302_LEN_EX_PHSUC, 0xa7 << 24);

    MeterChipWriteReg(RN7302_REG_PHSUA, RN7302_LEN_EX_PHSUA, (0xa7 << 24) | 0x200);
    MeterChipWriteReg(RN7302_REG_PHSUB, RN7302_LEN_EX_PHSUB, ((0xa7 << 24) | 0x200) + jba);
    MeterChipWriteReg(RN7302_REG_PHSUC, RN7302_LEN_EX_PHSUC, ((0xa7 << 24) | 0x200) + jca);

    MeterChipWriteReg(RN7302_REG_PHSIA, RN7302_LEN_EX_PHSIA, factorIAngle[0]);
    MeterChipWriteReg(RN7302_REG_PHSIB, RN7302_LEN_EX_PHSIB, factorIAngle[1] + jba);
    MeterChipWriteReg(RN7302_REG_PHSIC, RN7302_LEN_EX_PHSIC, factorIAngle[2] + jca);

    temp = (RN7302_CMD_WR_DIS << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, temp);

    temp = RN7302_CMD_WR_DIS; // 写保护
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);

    pCfgSys->adjust.Phs_Ia.value = factorIAngle[0];
    pCfgSys->adjust.Phs_Ib.value = factorIAngle[1] + jba;
    pCfgSys->adjust.Phs_Ic.value = factorIAngle[2] + jca;
    pCfgSys->adjust.Phs_Ua.value = 0;
    pCfgSys->adjust.Phs_Ub.value = jba;
    pCfgSys->adjust.Phs_Uc.value = jca;
 
    
    return 0;
}




/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int32_t Rn7302Adjust_PQ(struct db_database_measure_base_type *pMeasureBase)
{
    uint16_t factorP[3] = {0,0,0};
    uint16_t factorQ[3] = {0,0,0};
    int32_t s32TmpP;
    int32_t s32TmpQ;
    int temp = 0;
    double Err = 0;
    
    struct db_cfg_sys_para *pCfgSys = &(pMeasureBase->db_cfg.sys);

        //功率相位校正
    if((-1 != MeterChipReadReg(RN7302_REG_PA, RN7302_LEN_PA,&s32TmpP)) && (-1 != MeterChipReadReg(RN7302_REG_QA, RN7302_LEN_QA, &s32TmpQ)))
    {
        /* P */
        Err = ((double)((pCfgSys->adjustAdvalue.Adjust_Pa.value * 0.5) - s32TmpP)) / s32TmpQ;
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorP[0] = (int16_t)(Err + 0.5);

        /* Q */
        Err = ((double)((pCfgSys->adjustAdvalue.Adjust_Pa.value * 0.866025404) - s32TmpQ)) / s32TmpP;
        Err = -Err;
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorQ[0] = (int16_t)(Err + 0.5);
    }

    if((-1 != MeterChipReadReg(RN7302_REG_PB, RN7302_LEN_PB,&s32TmpP)) && (-1 != MeterChipReadReg(RN7302_REG_QB, RN7302_LEN_QB, &s32TmpQ)))
    {
        Err = ((double)((pCfgSys->adjustAdvalue.Adjust_Pb.value * 0.5) - s32TmpP)) / s32TmpQ;
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorP[1] = (int16_t)(Err );

        Err = ((double)((pCfgSys->adjustAdvalue.Adjust_Pb.value * 0.866025404) - s32TmpQ)) / s32TmpP;
        Err = -Err;
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorQ[1] = (int16_t)(Err + 0.5);
    }

    if((-1 != MeterChipReadReg(RN7302_REG_PC, RN7302_LEN_PC,&s32TmpP)) && (-1 != MeterChipReadReg(RN7302_REG_QC, RN7302_LEN_QC, &s32TmpQ)))
    {
        Err = ((double)((pCfgSys->adjustAdvalue.Adjust_Pc.value * 0.5) - s32TmpP)) / s32TmpQ;
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorP[2] = (int16_t)(Err );

        Err = ((double)((pCfgSys->adjustAdvalue.Adjust_Pc.value * 0.866025404) - s32TmpQ)) / s32TmpP;
        Err = -Err;
        Err = Err > 0 ? (Err * 32768) : (Err * 32768 + 65536);
        factorQ[2] = (int16_t)(Err + 0.5);
    }

    temp = RN7302_CMD_WR_EN; // 写使能位
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);        

    //全波
    MeterChipWriteReg(RN7302_REG_PA_PHSL, RN7302_LEN_PA_PHSL, factorP[0]);
    MeterChipWriteReg(RN7302_REG_PB_PHSL, RN7302_LEN_PB_PHSL, factorP[1]);
    MeterChipWriteReg(RN7302_REG_PC_PHSL, RN7302_LEN_PC_PHSL, factorP[2]);
    MeterChipWriteReg(RN7302_REG_QA_PHSL, RN7302_LEN_QA_PHSL, factorQ[0]);
    MeterChipWriteReg(RN7302_REG_QB_PHSL, RN7302_LEN_QB_PHSL, factorQ[1]);
    MeterChipWriteReg(RN7302_REG_QC_PHSL, RN7302_LEN_QC_PHSL, factorQ[2]);
    //基波
    MeterChipWriteReg(RN7302_REG_FPA_PHS, RN7302_LEN_FPA_PHS, factorP[0]);
    MeterChipWriteReg(RN7302_REG_FPB_PHS, RN7302_LEN_FPB_PHS, factorP[1]);
    MeterChipWriteReg(RN7302_REG_FPC_PHS, RN7302_LEN_FPC_PHS, factorP[2]);
    MeterChipWriteReg(RN7302_REG_FQA_PHS, RN7302_LEN_FQA_PHS, factorQ[0]);
    MeterChipWriteReg(RN7302_REG_FQB_PHS, RN7302_LEN_FQB_PHS, factorQ[1]);
    MeterChipWriteReg(RN7302_REG_FQC_PHS, RN7302_LEN_FQC_PHS, factorQ[2]);

    temp = RN7302_CMD_WR_DIS; // 写保护
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, temp);

    pCfgSys->adjust.Pa_PhsL.value = factorP[0];
    pCfgSys->adjust.Pb_PhsL.value = factorP[1];
    pCfgSys->adjust.Pc_PhsL.value = factorP[2];
    pCfgSys->adjust.Qa_PhsL.value = factorQ[0];
    pCfgSys->adjust.Qb_PhsL.value = factorQ[1];
    pCfgSys->adjust.Qc_PhsL.value = factorQ[2];

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int32_t Rn7302Adjust_Default_wait(struct db_database_measure_base_type *pMeasureBase)
{
    int32_t U_1st[3] = {0,0,0}, U_2ed[3] = {0,0,0}, I_1st[4] = {0,0,0,0}, I_2ed[4] = {0,0,0,0};
    static int num = 10;
    struct db_cfg_sys_para *pCfgSys = &(pMeasureBase->db_cfg.sys);

    while (num--)
    {
        MSR_PORT_DELAY(300);
        MeterChipReadReg(RN7302_REG_UA, RN7302_LEN_UA, &U_1st[0]);
        MeterChipReadReg(RN7302_REG_UB, RN7302_LEN_UB, &U_1st[1]);
        MeterChipReadReg(RN7302_REG_UC, RN7302_LEN_UC, &U_1st[2]);
        MeterChipReadReg(RN7302_REG_IA, RN7302_LEN_IA, &I_1st[0]);
        MeterChipReadReg(RN7302_REG_IB, RN7302_LEN_IB, &I_1st[1]);
        MeterChipReadReg(RN7302_REG_IC, RN7302_LEN_IC, &I_1st[2]);
        MSR_PORT_DELAY(300);
        MeterChipReadReg(RN7302_REG_UA, RN7302_LEN_UA, &U_2ed[0]);
        MeterChipReadReg(RN7302_REG_UB, RN7302_LEN_UB, &U_2ed[1]);
        MeterChipReadReg(RN7302_REG_UC, RN7302_LEN_UC, &U_2ed[2]);
        MeterChipReadReg(RN7302_REG_IA, RN7302_LEN_IA, &I_2ed[0]);
        MeterChipReadReg(RN7302_REG_IB, RN7302_LEN_IB, &I_2ed[1]);
        MeterChipReadReg(RN7302_REG_IC, RN7302_LEN_IC, &I_2ed[2]);
        if((abs(U_1st[0] - U_2ed[0]) < (pCfgSys->adjustAdvalue.Adjust_Va.value / 1000)) && (abs(U_1st[1] - U_2ed[1]) < (pCfgSys->adjustAdvalue.Adjust_Vb.value / 1000)) && (abs(U_1st[2] - U_2ed[2]) < (pCfgSys->adjustAdvalue.Adjust_Vc.value / 1000)) )
        {
            return 1;
        }
    }
    return -1;        
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int32_t Rn7302Adjust_UI_wait(struct db_database_measure_base_type *pMeasureBase)
{
    int32_t U_1st[3] = {0,0,0}, U_2ed[3] = {0,0,0}, I_1st[4] = {0,0,0,0}, I_2ed[4] = {0,0,0,0};
    int num = 10;
    
    struct db_cfg_sys_para *pCfgSys = &(pMeasureBase->db_cfg.sys);

    while (num--)
    {
        MSR_PORT_DELAY(300);

        MeterChipReadReg(RN7302_REG_UA, RN7302_LEN_UA, &U_1st[0]);
        MeterChipReadReg(RN7302_REG_UB, RN7302_LEN_UB, &U_1st[1]);
        MeterChipReadReg(RN7302_REG_UC, RN7302_LEN_UC, &U_1st[2]);
        MeterChipReadReg(RN7302_REG_IA, RN7302_LEN_IA, &I_1st[0]);
        MeterChipReadReg(RN7302_REG_IB, RN7302_LEN_IB, &I_1st[1]);
        MeterChipReadReg(RN7302_REG_IC, RN7302_LEN_IC, &I_1st[2]);
		
        MSR_PORT_DELAY(300);
        MeterChipReadReg(RN7302_REG_UA, RN7302_LEN_UA, &U_2ed[0]);
        MeterChipReadReg(RN7302_REG_UB, RN7302_LEN_UB, &U_2ed[1]);
        MeterChipReadReg(RN7302_REG_UC, RN7302_LEN_UC, &U_2ed[2]);
        MeterChipReadReg(RN7302_REG_IA, RN7302_LEN_IA, &I_2ed[0]);
        MeterChipReadReg(RN7302_REG_IB, RN7302_LEN_IB, &I_2ed[1]);
        MeterChipReadReg(RN7302_REG_IC, RN7302_LEN_IC, &I_2ed[2]);
       
        if((abs(I_1st[0] - I_2ed[0]) < (pCfgSys->adjustAdvalue.Adjust_Ia.value / 10000)) && (abs(I_1st[1] - I_2ed[1]) < (pCfgSys->adjustAdvalue.Adjust_Ia.value / 10000)) && (abs(I_1st[2] - I_2ed[2]) < (pCfgSys->adjustAdvalue.Adjust_Ia.value / 10000)) )
        {
//            if((abs(I_2ed[0] - ADJ_CUR_VAL_5A) < (ADJ_CUR_VAL_5A / 10000)) && (abs(I_2ed[1] - ADJ_CUR_VAL_5A) < (ADJ_CUR_VAL_5A / 10000)) && (abs(I_2ed[2] - ADJ_CUR_VAL_5A) < (ADJ_CUR_VAL_5A / 10000)))
            {
                return 1;
            }
        }        
    }
    return -1;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int32_t Rn7302Adjust_Angle_wait(struct db_database_measure_base_type *pMeasureBase)
{
    MSR_PORT_DELAY(300);
    return 1;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int32_t Rn7302Adjust_PQ_wait(struct db_database_measure_base_type *pMeasureBase)
{
    int32_t P_1st[3] = {0,0,0}, P_2ed[3] = {0,0,0};
    int num = 10;

    struct db_cfg_sys_para *pCfgSys = &(pMeasureBase->db_cfg.sys);

    while (num--)
    {
        MSR_PORT_DELAY(300);
        MeterChipReadReg(RN7302_REG_PA, RN7302_LEN_PA, &P_1st[0]);
        MeterChipReadReg(RN7302_REG_PB, RN7302_LEN_PB, &P_1st[1]);
        MeterChipReadReg(RN7302_REG_PC, RN7302_LEN_PC, &P_1st[2]);
        
		MSR_PORT_DELAY(300);
        MeterChipReadReg(RN7302_REG_PA, RN7302_LEN_PA, &P_2ed[0]);
        MeterChipReadReg(RN7302_REG_PB, RN7302_LEN_PB, &P_2ed[1]);
        MeterChipReadReg(RN7302_REG_PC, RN7302_LEN_PC, &P_2ed[2]);

        if((abs(P_1st[0] - P_2ed[0]) < 10490) && (abs(P_1st[1] - P_2ed[1]) < 10490) && (abs(P_1st[2] - P_2ed[2]) < 10490) )
        {
            if((abs(P_2ed[0] - (int32_t)(pCfgSys->adjustAdvalue.Adjust_Pa.value / 2)) < (pCfgSys->adjustAdvalue.Adjust_Pa.value / 2 /1000)) && (abs(P_2ed[1] - (int32_t)(pCfgSys->adjustAdvalue.Adjust_Pb.value / 2)) < (pCfgSys->adjustAdvalue.Adjust_Pb.value / 2 /1000)) && (abs(P_2ed[2] - (int32_t)(pCfgSys->adjustAdvalue.Adjust_Pc.value / 2)) < (pCfgSys->adjustAdvalue.Adjust_Pc.value / 2 /1000)))
            {
                return 1;
            } 
        }        
    }
    return -1;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int32_t Rn7302Adjust(struct db_database_measure_base_type *pMeasureBase)
{
    int ret = 0;

    Rn7302Adjust_Default(pMeasureBase);
    ret = Rn7302Adjust_Default_wait(pMeasureBase);
    if(ret == 1)
    {
        Rn7302Adjust_UI(pMeasureBase);
        ret = Rn7302Adjust_UI_wait(pMeasureBase);
        if(ret == 1)
        {
printf("Rn7302Adjust_PQ\n");
#if PHASE_ADJUST_SEL == PHASE_CHANNEL

            Rn7302Adjust_Angle(pMeasureBase);
            Rn7302Adjust_Angle_wait(pMeasureBase);
#else
            Rn7302Adjust_PQ(pMeasureBase);
            ret = Rn7302Adjust_PQ_wait(pMeasureBase);
#endif
            if(ret == -1)
            {
            }
        }
        else if(ret == -1)
        {
        }
    }
    else if(ret == -1)
    {
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
void Rn7302_RST_AdjustFactor(struct db_database_measure_base_type *pMeasureBase)
{
    struct db_cfg_sys_para              *pCfgSys = &(pMeasureBase->db_cfg.sys);
    int tmp;
    int factor = 0;
    // 写使能位   
    tmp = RN7302_CMD_WR_EN; //写使能位
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp);           
    //将默认系数写入CE    
    MeterChipWriteReg(RN7302_REG_GSUA, RN7302_LEN_GSUA, factor);
    MeterChipWriteReg(RN7302_REG_GSUB, RN7302_LEN_GSUB, factor);
    MeterChipWriteReg(RN7302_REG_GSUC, RN7302_LEN_GSUC, factor);  
    MeterChipWriteReg(RN7302_REG_GSIA, RN7302_LEN_GSIA, factor);
    MeterChipWriteReg(RN7302_REG_GSIB, RN7302_LEN_GSIB, factor);
    MeterChipWriteReg(RN7302_REG_GSIC, RN7302_LEN_GSIC, factor);
//    MeterChipWriteReg(RN7302_REG_GSIN, RN7302_LEN_GSIN, factor);
#if PHASE_ADJUST_SEL == PHASE_CHANNEL
    // 扩展位
    tmp = (RN7302_CMD_WR_EN << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);

    //相位
    MeterChipWriteReg(RN7302_REG_PHSUA, RN7302_LEN_EX_PHSUA, 0xa7 << 24);
    MeterChipWriteReg(RN7302_REG_PHSUB, RN7302_LEN_EX_PHSUB, 0xa7 << 24);
    MeterChipWriteReg(RN7302_REG_PHSUC, RN7302_LEN_EX_PHSUC, 0xa7 << 24);

    MeterChipWriteReg(RN7302_REG_PHSUA, RN7302_LEN_EX_PHSUA, (0xa7 << 24) | 0x200);
    MeterChipWriteReg(RN7302_REG_PHSUB, RN7302_LEN_EX_PHSUB, (0xa7 << 24) | 0x200);
    MeterChipWriteReg(RN7302_REG_PHSUC, RN7302_LEN_EX_PHSUC, (0xa7 << 24) | 0x200);

	MeterChipWriteReg(RN7302_REG_PHSIA, RN7302_LEN_PHSIA, 0x20080200);
    MeterChipWriteReg(RN7302_REG_PHSIB, RN7302_LEN_PHSIB, 0x20080200);
    MeterChipWriteReg(RN7302_REG_PHSIC, RN7302_LEN_PHSIC, 0x20080200);

    MeterChipWriteReg(RN7302_REG_PRTH1L, RN7302_LEN_PRTH1L, 0);

    tmp = (RN7302_CMD_WR_DIS << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);

    tmp = RN7302_CMD_WR_DIS; // 写保护
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp); 

    pCfgSys->adjust.Phs_Ua.value = 0x80;
    pCfgSys->adjust.Phs_Ub.value = 0x80;
    pCfgSys->adjust.Phs_Uc.value = 0x80;
    pCfgSys->adjust.Phs_Ia.value = 0x200;
    pCfgSys->adjust.Phs_Ib.value = 0x200;
    pCfgSys->adjust.Phs_Ic.value = 0x200;
    pCfgSys->adjust.Phs_In.value = 0x200;

    pCfgSys->prthvalue.PhsxR1.value = 0;
    pCfgSys->prthvalue.PhsxR2.value = 0;
    pCfgSys->prthvalue.PhsxR3.value = 0;
    pCfgSys->prthvalue.Prth1L.value = 0;
    pCfgSys->prthvalue.Prth1H.value = 0;
    pCfgSys->prthvalue.Prth2L.value = 0;
    pCfgSys->prthvalue.Prth2H.value = 0;


//#else
    MeterChipWriteReg(RN7302_REG_PA_PHSL, RN7302_LEN_PA_PHSL, factor);
    MeterChipWriteReg(RN7302_REG_PB_PHSL, RN7302_LEN_PB_PHSL, factor);
    MeterChipWriteReg(RN7302_REG_PC_PHSL, RN7302_LEN_PC_PHSL, factor);
    MeterChipWriteReg(RN7302_REG_QA_PHSL, RN7302_LEN_QA_PHSL, factor);
    MeterChipWriteReg(RN7302_REG_QB_PHSL, RN7302_LEN_QB_PHSL, factor);
    MeterChipWriteReg(RN7302_REG_QC_PHSL, RN7302_LEN_QC_PHSL, factor);
    MeterChipWriteReg(RN7302_REG_IA_OS, RN7302_LEN_IA_OS,  factor);
    MeterChipWriteReg(RN7302_REG_IB_OS, RN7302_LEN_IB_OS,  factor);
    MeterChipWriteReg(RN7302_REG_IC_OS, RN7302_LEN_IC_OS,  factor);

    tmp = RN7302_CMD_WR_DIS; // 写保护
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp); 
    pCfgSys->adjust.Pa_PhsL.value = factor;
    pCfgSys->adjust.Pb_PhsL.value = factor;
    pCfgSys->adjust.Pc_PhsL.value = factor;
    pCfgSys->adjust.Qa_PhsL.value = factor;
    pCfgSys->adjust.Qb_PhsL.value = factor;
    pCfgSys->adjust.Qc_PhsL.value = factor;

#endif     
    pCfgSys->adjust.GsUa.value = factor;
    pCfgSys->adjust.GsUb.value = factor;
    pCfgSys->adjust.GsUc.value = factor;
    pCfgSys->adjust.GsIa.value = factor;
    pCfgSys->adjust.GsIb.value = factor;
    pCfgSys->adjust.GsIc.value = factor;
    pCfgSys->adjust.GsIn.value = factor;
    pCfgSys->adjust.I0Adj.value = factor;

    pCfgSys->adjust.Offset_Ia.value = factor;
    pCfgSys->adjust.Offset_Ib.value = factor;
    pCfgSys->adjust.Offset_Ic.value = factor;

}

	
/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void Rn7302AdjustObject(uint32_t reg, uint32_t nbyte, uint32_t value)
{
    int tmp;
    int32_t factor;

    if(-1 != MeterChipReadReg(reg, nbyte, &factor))
    {
        if(factor != value)
        {
            // 写使能位   
            tmp = RN7302_CMD_WR_EN; //写使能位
            MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp);           
            //将默认系数写入CE    
            MeterChipWriteReg(reg, nbyte, value);
            tmp = RN7302_CMD_WR_DIS; // 写保护
            MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, tmp);   
        }
    }
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void Rn7302_AdjustFactor(struct db_database_measure_base_type *pMeasureBase)
{
    struct db_cfg_sys_para              *pCfgSys = &(pMeasureBase->db_cfg.sys);
    int32_t tmp;
	
    Rn7302AdjustObject(RN7302_REG_HFConst1, 	RN7302_LEN_HFConst1,	pCfgSys->adjust.HfConst1.value); 
    Rn7302AdjustObject(RN7302_REG_GSUA,			RN7302_LEN_GSUA,		pCfgSys->adjust.GsUa.value);
    Rn7302AdjustObject(RN7302_REG_GSUB,			RN7302_LEN_GSUB,		pCfgSys->adjust.GsUb.value);
    Rn7302AdjustObject(RN7302_REG_GSUC,			RN7302_LEN_GSUC,		pCfgSys->adjust.GsUc.value);
    Rn7302AdjustObject(RN7302_REG_GSIA,			RN7302_LEN_GSIA,		pCfgSys->adjust.GsIa.value);
    Rn7302AdjustObject(RN7302_REG_GSIB,			RN7302_LEN_GSIB,		pCfgSys->adjust.GsIb.value);
    Rn7302AdjustObject(RN7302_REG_GSIC,			RN7302_LEN_GSIC,		pCfgSys->adjust.GsIc.value);
    Rn7302AdjustObject(RN7302_REG_GSIN,			RN7302_LEN_GSIN,		pCfgSys->adjust.GsIn.value);
#if PHASE_ADJUST_SEL == PHASE_CHANNEL
    // 通道相位校正
    // 扩展位
    tmp = (RN7302_CMD_WR_EN << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);

    MeterChipWriteReg(RN7302_REG_PHSUA, RN7302_LEN_EX_PHSUA, 0xa7 << 24);
    MeterChipWriteReg(RN7302_REG_PHSUB, RN7302_LEN_EX_PHSUB, 0xa7 << 24);
    MeterChipWriteReg(RN7302_REG_PHSUC, RN7302_LEN_EX_PHSUC, 0xa7 << 24);

    MeterChipWriteReg(RN7302_REG_PHSUA, RN7302_LEN_EX_PHSUA, (0xa7 << 24) | 0x200);
    MeterChipWriteReg(RN7302_REG_PHSUB, RN7302_LEN_EX_PHSUB, (0xa7 << 24) | 0x200);
    MeterChipWriteReg(RN7302_REG_PHSUC, RN7302_LEN_EX_PHSUC, (0xa7 << 24) | 0x200);

    MeterChipWriteReg(RN7302_REG_PHSIA, RN7302_LEN_EX_PHSIA, pCfgSys->adjust.Phs_Ia.value);
    MeterChipWriteReg(RN7302_REG_PHSIB, RN7302_LEN_EX_PHSIB, pCfgSys->adjust.Phs_Ib.value);
    MeterChipWriteReg(RN7302_REG_PHSIC, RN7302_LEN_EX_PHSIC, pCfgSys->adjust.Phs_Ic.value);

    tmp = (RN7302_CMD_WR_DIS << 8 ) | 0xaa;
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, tmp);


    Rn7302AdjustObject(RN7302_REG_PA_PHSL,		RN7302_LEN_PA_PHSL,		0);
    Rn7302AdjustObject(RN7302_REG_PB_PHSL,		RN7302_LEN_PB_PHSL,		0);
    Rn7302AdjustObject(RN7302_REG_PC_PHSL,		RN7302_LEN_PC_PHSL,		0);
    Rn7302AdjustObject(RN7302_REG_QA_PHSL,		RN7302_LEN_QA_PHSL,		0);
    Rn7302AdjustObject(RN7302_REG_QB_PHSL,		RN7302_LEN_QB_PHSL,		0);
    Rn7302AdjustObject(RN7302_REG_QC_PHSL,		RN7302_LEN_QC_PHSL,		0);
#else 
    Rn7302AdjustObject(RN7302_REG_PA_PHSL,		RN7302_LEN_PA_PHSL,		pCfgSys->adjust.Pa_PhsL.value);
    Rn7302AdjustObject(RN7302_REG_PB_PHSL,		RN7302_LEN_PB_PHSL,		pCfgSys->adjust.Pb_PhsL.value);
    Rn7302AdjustObject(RN7302_REG_PC_PHSL,		RN7302_LEN_PC_PHSL,		pCfgSys->adjust.Pc_PhsL.value);
    Rn7302AdjustObject(RN7302_REG_QA_PHSL,		RN7302_LEN_QA_PHSL,		pCfgSys->adjust.Qa_PhsL.value);
    Rn7302AdjustObject(RN7302_REG_QB_PHSL,		RN7302_LEN_QB_PHSL,		pCfgSys->adjust.Qb_PhsL.value);
    Rn7302AdjustObject(RN7302_REG_QC_PHSL,		RN7302_LEN_QC_PHSL,		pCfgSys->adjust.Qc_PhsL.value);
#endif
}




