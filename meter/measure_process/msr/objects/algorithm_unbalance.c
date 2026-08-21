
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_unbalance.c
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
#include <unistd.h>

#include <string.h>
#include <stdlib.h>
 
#include "math.h"
#include "algorithm_unbalance.h"

#define  PI 3.14159265358979f
#define  THIRD_SQRT3  (sqrt(3)/3)//三分之根号三
#define  PI2P3        (PI*2/3) //三分之二π
#define  PI4P3        (PI*4/3) //三分之四π
#define  SQRRT3       (sqrt(3))
#define  VoltageLimit    2000//2000  // 20V 
#define  PI1P6        (PI/6) //六分之一π
#define  PI1P180      (PI/180) //π/180

#define P3W3 1
#define P3W4 0


/*********************************************************************************************************
** Function name:       UnbalanceVoltJudge
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t UnbalanceVoltJudge(struct db_database_measure_base_type *pMeasureBase, struct db_powerquality_unbalance_type *out)
{
    uint16_t                    mode = 0;
    struct db_cfg_user_para          *pCfgUser = &(pMeasureBase->db_cfg.user);
    
    mode = (pCfgUser->base.WiringMode.value & 0x03);

    if(P3W3 == mode)
    {
        out->Unbalance_V.UnbalanceValue.S0 = 0;
        out->Unbalance_V.UnbalanceValue.S1 = 0;
        out->Unbalance_V.UnbalanceValue.S2 = 0;
        out->Unbalance_V.UnbalanceFactor.I0 = 0;
        out->Unbalance_V.UnbalanceFactor.I1 = 0;
    }
    return 0;
}
/*******************************************************************************
函数名称：u32 UnbalanceVoltCal()
功    能：电压不平衡率计算
子 函 数：sqrt（）
调用限制：文件内部调用
输入参数：无
输出参数：无
返 回 值：无
说    明：0：ok，1：error
*******************************************************************************/
int32_t UnbalanceVoltCal(struct db_database_measure_base_type *pMeasureBase, struct db_powerquality_unbalance_type *out)
{
    float temp = 0;
    float V0cos = 0,V0sin = 0,V1cos = 0,V1sin = 0,V2cos = 0,V2sin = 0;
    float Aa = 0,Ab = 0,Ac = 0;

    struct db_basic_realtime_type                   *realtime = &(pMeasureBase->db_measure.numeric.fullwave.primary);
    struct db_basic_realtime_type                   *pfundwareRT = &(pMeasureBase->db_measure.numeric.fundwave.primary);

    Aa = ((float)(pfundwareRT->J_Va - pfundwareRT->J_Va)/100) * PI1P180 ;
    Ab = ((float)(pfundwareRT->J_Vb - pfundwareRT->J_Va)/100) * PI1P180;
    Ac = ((float)(pfundwareRT->J_Vc - pfundwareRT->J_Va)/100) * PI1P180;

    //零序
    V0cos = (realtime->Va * cos(Aa) + realtime->Vb * cos(Ab) + realtime->Vc * cos(Ac));
    V0sin = (realtime->Va * sin(Aa) + realtime->Vb * sin(Ab) + realtime->Vc * sin(Ac));

    //负序
    V1cos = (realtime->Va * cos(Aa) + realtime->Vb * cos(Ab - PI2P3) + realtime->Vc * cos(Ac - PI4P3));
    V1sin = (realtime->Va * sin(Aa) + realtime->Vb * sin(Ab - PI2P3) + realtime->Vc * sin(Ac - PI4P3));

    //正序
    V2cos = (realtime->Va * cos(Aa) + realtime->Vb * cos(Ab - PI4P3) + realtime->Vc * cos(Ac - PI2P3));
    V2sin = (realtime->Va * sin(Aa) + realtime->Vb * sin(Ab - PI4P3) + realtime->Vc * sin(Ac - PI2P3));

    out->Unbalance_V.UnbalanceValue.S0 = sqrt(V0cos * V0cos + V0sin * V0sin) / 3;//����
    out->Unbalance_V.UnbalanceValue.S1 = sqrt(V1cos * V1cos + V1sin * V1sin) / 3;//����
    out->Unbalance_V.UnbalanceValue.S2 = sqrt(V2cos * V2cos + V2sin * V2sin) / 3;//����

    if(out->Unbalance_V.UnbalanceValue.S2 < 0.00001)
    {
        out->Unbalance_V.UnbalanceValue.S2 = 0;
    }

    temp =(float) out->Unbalance_V.UnbalanceValue.S1 / out->Unbalance_V.UnbalanceValue.S2;
    if(temp > 1.0)
    {
        temp = 1.0;
    }
    if(out->Unbalance_V.UnbalanceValue.S1 < 0.0001)
    {
        temp = 0.0;
    }
    out->Unbalance_V.UnbalanceFactor.I1 = temp *10000;
    
    temp = (float)out->Unbalance_V.UnbalanceValue.S0 / out->Unbalance_V.UnbalanceValue.S2;
    if(temp >1.0)
    {
       temp =1.0;
    }
    if(out->Unbalance_V.UnbalanceValue.S0 < 0.0001)
    {
        temp = 0.0;
    }
    out->Unbalance_V.UnbalanceFactor.I0 = temp*10000;
    UnbalanceVoltJudge(pMeasureBase, out);
/*
*/
    return 0;
}
/*********************************************************************************************************
** Function name:       UnbalanceCurrentJudge
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t UnbalanceCurrentJudge(struct db_database_measure_base_type *pMeasureBase, struct db_powerquality_unbalance_type *out)
{
    struct db_cfg_user_para      *pCfgUser = &(pMeasureBase->db_cfg.user);
       
    if (P3W3 == (pCfgUser->base.WiringMode.value & 0x03))
    {
        out->Unbalance_I.UnbalanceValue.S0 = 0;
        out->Unbalance_I.UnbalanceValue.S1 = 0;
        out->Unbalance_I.UnbalanceValue.S2 = 0;
        out->Unbalance_I.UnbalanceValue.S0 = 0;
        out->Unbalance_I.UnbalanceFactor.I0 = 0;
    }
    return 0;
}
/*******************************************************************************
函数名称：u32 UnbalanceCurrenttCal()
功    能：电流不平衡率计算
子 函 数：sqrt（）
调用限制：文件内部调用
输入参数：无
输出参数：无
返 回 值：无
说    明：0：ok，1：error
*******************************************************************************/
static int32_t UnbalanceCurrentCal(struct db_database_measure_base_type *pMeasureBase, struct db_powerquality_unbalance_type *out)
{
    float temp = 0;
    float I0cos= 0,I0sin = 0,I1cos = 0,I1sin = 0,I2cos = 0,I2sin = 0;
    float Aa = 0,Ab = 0,Ac = 0;
    uint16_t                    mode = 0;
    struct db_basic_realtime_type                   *realtime = &(pMeasureBase->db_measure.numeric.fullwave.primary);
    struct db_basic_realtime_type                   *pfundwareRT = &(pMeasureBase->db_measure.numeric.fundwave.primary);
    
    
    Aa = ((float)(pfundwareRT->J_Ia - pfundwareRT->J_Ia)/100) * PI1P180 ;
    Ab = ((float)(pfundwareRT->J_Ib - pfundwareRT->J_Ia)/100) * PI1P180 ;
    Ac = ((float)(pfundwareRT->J_Ic - pfundwareRT->J_Ia)/100) * PI1P180 ;

    if(P3W3 == mode)
    {
        //零序
        out->Unbalance_I.UnbalanceValue.S0 = 0;
        out->Unbalance_I.UnbalanceValue.S1 = 0;
        out->Unbalance_I.UnbalanceValue.S2 = 0;
        out->Unbalance_I.UnbalanceFactor.I0 = 0;
        out->Unbalance_I.UnbalanceFactor.I1 = 0;
        return 0;
    }
    
    //零序
    I0cos =  (realtime->Ia * cos(Aa) + realtime->Ib * cos(Ab) + realtime->Ic * cos(Ac));
    I0sin =  (realtime->Ia * sin(Aa) + realtime->Ib * sin(Ab) + realtime->Ic * sin(Ac));
    out->Unbalance_I.UnbalanceValue.S0 = sqrt(I0cos * I0cos + I0sin * I0sin) * 10 / 3;  //����
    
    //负序
    I1cos =  (realtime->Ia * cos(Aa) + realtime->Ib * cos(Ab - PI2P3) + realtime->Ic * cos(Ac - PI4P3));    
    I1sin =  (realtime->Ia * sin(Aa) + realtime->Ib * sin(Ab - PI2P3) + realtime->Ic * sin(Ac - PI4P3));
    
    //正序
    I2cos =  (realtime->Ia * cos(Aa) + realtime->Ib * cos(Ab - PI4P3) + realtime->Ic * cos(Ac - PI2P3));   
    I2sin =  (realtime->Ia * sin(Aa) + realtime->Ib * sin(Ab - PI4P3) + realtime->Ic * sin(Ac - PI2P3));
    
    out->Unbalance_I.UnbalanceValue.S1 = sqrt(I1cos * I1cos + I1sin * I1sin) * 10 / 3;  //����
    out->Unbalance_I.UnbalanceValue.S2 = sqrt(I2cos * I2cos + I2sin * I2sin) * 10 / 3;  //����
/*       
 
*/
    
/*
*/
    if(out->Unbalance_I.UnbalanceValue.S2 < 0.00001)
    {
        out->Unbalance_I.UnbalanceValue.S2 = 0;
    }
    temp = (float)out->Unbalance_I.UnbalanceValue.S1 / out->Unbalance_I.UnbalanceValue.S2;
    if(temp >1.0)
    {
         temp =1.0;
    }
    if(out->Unbalance_I.UnbalanceValue.S1 < 0.0001)
    {
        temp = 0.0;
    }
    out->Unbalance_I.UnbalanceFactor.I1= temp * 10000;

    temp = (float)out->Unbalance_I.UnbalanceValue.S0 / out->Unbalance_I.UnbalanceValue.S2;
    if(temp >1.0)
    {
       temp =1.0;
    }
    if(out->Unbalance_I.UnbalanceValue.S0 < 0.0001)
    {
        temp = 0.0;
    }
    out->Unbalance_I.UnbalanceFactor.I0 = temp * 10000;
    
    UnbalanceCurrentJudge(pMeasureBase, out);
/*

*/
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
int32_t rms_algorithm_Unbalance(struct db_database_measure_base_type *pMeasureBase, struct db_powerquality_unbalance_type *out, uint32_t pt, uint32_t ct)
{
    UnbalanceVoltCal(pMeasureBase, out);
    UnbalanceCurrentCal(pMeasureBase, out);
    return 0;
}

