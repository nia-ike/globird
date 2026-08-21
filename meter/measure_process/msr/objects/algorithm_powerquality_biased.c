/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_powerquality_biased.c
*  文件标识：
*  描    述：偏差
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
#include <stdlib.h>
#include <fcntl.h> 
#include "math.h"
#include "algorithm_powerquality_biased.h"

/*********************************************************************************************************
** Function name:       rms_algorithm_biased_frequency
** Descriptions:        频率偏差= 实测频率 -标称频率,精确到0.01Hz，
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_biased_frequency(struct db_database_measure_base_type *pstVmem, struct db_powerquality_basic_type *out, uint32_t pt, uint32_t ct)
{
    struct db_basic_realtime_type       *pRealtime = &(pstVmem->db_measure.numeric.fullwave.primary);
    struct db_cfg_user_para                *pCfg = &(pstVmem->db_cfg.user);
    
    uint32_t hz = 500000;
    if(pCfg->refer.refer_frequency.value != 0)
    {
        hz =  pCfg->refer.refer_frequency.value;
    }
    else
    {
        hz = 500000;
    }

    out->OffsetHz = pRealtime->Hz - hz;
//    printf("OffsetHz:  %d  %d\n",pRealtimeOther->OffsetHz, pRealtimePrimary->Hz);
    return 0;
}


/*********************************************************************************************************
** Function name:       biased_adjust
** Descriptions:        
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int16_t biased_adjust(int16_t* offset, uint32_t value, uint32_t rated)
{
    int16_t temp;
    temp = (int32_t)(value - rated) * 10000 / (int32_t)rated;
    if(value != 0)
    {
        *offset = temp;
    }
    else
    {
        *offset = 0;       
    }
    return 0;
}

/*********************************************************************************************************
** Function name:       rms_algorithm_biased_voltage
** Descriptions:        电压偏差(%) =（实测电压– 标称电压）/标称电压 × 100%                       
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_biased_voltage(struct db_database_measure_base_type *pstVmem, struct db_powerquality_basic_type *out, uint32_t pt, uint32_t ct)
{
    uint16_t                        wireMode               = 0;
    struct db_cfg_sys_para                   *pCfgSys = &(pstVmem->db_cfg.sys);                         /* 获取功率死区值 */
    struct db_cfg_user_para                 *pCfgUser = &(pstVmem->db_cfg.user);                        /* 获取接线模式 */
    struct db_realtime_secondary_type          *pRealtime = &(pstVmem->db_measure.numeric.fullwave.secondary); /* 电压偏差以配置的基准值比对，因此需要二次侧值*/
    uint16_t RatedVoltage = 0;

    wireMode = (pCfgUser->base.WiringMode.value & 0x03);
    if(pCfgUser->refer.refer_voltage.value != 0)
    {
        RatedVoltage =  pCfgUser->refer.refer_voltage.value;
    }
    else
    {
        RatedVoltage = pCfgSys->interface.RatedVoltage.value * 100;
    }

    switch (wireMode)
    {
    case P3W3:
        {
            biased_adjust(&out->OffsetVab, pRealtime->Vab, (RatedVoltage * 1.732050807));// sqrt(3)
            biased_adjust(&out->OffsetVbc, pRealtime->Vbc, (RatedVoltage * 1.732050807));// sqrt(3)
            biased_adjust(&out->OffsetVca, pRealtime->Vca, (RatedVoltage * 1.732050807));// sqrt(3)
            break;  
        }  
    case P3W4:
        {
            biased_adjust(&out->OffsetVa,  pRealtime->Va,  RatedVoltage);
            biased_adjust(&out->OffsetVb,  pRealtime->Vb,  RatedVoltage);
            biased_adjust(&out->OffsetVc,  pRealtime->Vc,  RatedVoltage);
            biased_adjust(&out->OffsetVab, pRealtime->Vab, (RatedVoltage * 1.732050807));// sqrt(3)
            biased_adjust(&out->OffsetVbc, pRealtime->Vbc, (RatedVoltage * 1.732050807));// sqrt(3)
            biased_adjust(&out->OffsetVca, pRealtime->Vca, (RatedVoltage * 1.732050807));// sqrt(3)

            break;    
        }
    default:
        break;
    }

    return 0;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_biased_loadrate
** Descriptions:        总负荷率
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_biased_loadrate(struct db_database_measure_base_type *pstVmem, struct db_powerquality_basic_type *out, uint32_t pt, uint32_t ct)
{
    uint16_t                         wireMode               = 0;
    struct db_realtime_secondary_type   *pRealtimeSecondary = &(pstVmem->db_measure.numeric.fullwave.secondary);
    struct db_cfg_sys_para                         *pCfgSys = &(pstVmem->db_cfg.sys);                         /* 获取功率死区值 */
    struct db_cfg_user_para                       *pCfgUser = &(pstVmem->db_cfg.user);                        /* 获取接线模式 */

    uint32_t RatedPower=0;

    wireMode = (pCfgUser->base.WiringMode.value & 0x03);
    if((pCfgUser->refer.refer_voltage.value != 0) && (pCfgUser->refer.refer_current.value != 0))
    {
        RatedPower = (uint64_t)pCfgUser->refer.refer_voltage.value * pCfgUser->refer.refer_current.value / 10000;
    }
    else
    {
        RatedPower = (uint64_t)pCfgSys->interface.RatedVoltage.value * pCfgSys->interface.RatedCurrent.value * 10;
    }


    switch (wireMode)
    {
    case P3W3: 
        {
            out->loadrate = abs(pRealtimeSecondary->Ptot) * 10000 / (RatedPower * 1.732050807);// sqrt(3)
            break;    
        }
    case P3W4:
        {
            out->loadrate = abs(pRealtimeSecondary->Ptot) * 10000 / (RatedPower * 3);
            break;    
        }
    default:
        break;
    }

    return 0;
}




/*********************************************************************************************************
** Function name:       rms_algorithm_VoltagePeakCf
** Descriptions:        电压波峰系数
                        CF=1.414*∑Uh/U1
                        Uh为第h次谐波方均根值，U1为基波方均根值，N为最高谐波次数,从第1次谐波开始
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_VoltagePeakCf(struct db_database_measure_base_type *pMeasureBase)
{
    int16_t i = 0;
    uint16_t wireMode = 0;
    uint64_t temp = 0;
    struct db_cfg_user_para                       *pCfgUser = &(pMeasureBase->db_cfg.user);       
    struct db_measure_powerquality_type      *pPowerQuality = &(pMeasureBase->db_measure.powerquality); 
    struct db_basic_realtime_type                *pRealtime = &(pMeasureBase->db_measure.numeric.fullwave.primary); 

    wireMode = (pCfgUser->base.WiringMode.value & 0x03);

    switch (wireMode)
    {
        case P3W3:
        {
            if(pRealtime->Vab != 0)
            {
                temp = 0;
                if(pPowerQuality->harmonic.THD.THDVa == 0)
                {
                    pPowerQuality->basic.WaveMaxFactorUa = 1414;
                }
                else
                {
                    for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += pPowerQuality->harmonic.HR.HRVa[i];
                    }

                    if(pPowerQuality->harmonic.HR.HRVa[1])
                        pPowerQuality->basic.WaveMaxFactorUa = 1414 * temp / pPowerQuality->harmonic.HR.HRVa[1];
                }
            }
            else
            {
                pPowerQuality->basic.WaveMaxFactorUa = 0;
            }

            if(pRealtime->Vbc != 0)
            {
                temp = 0;
                if(pPowerQuality->harmonic.THD.THDVb == 0)
                {
                    pPowerQuality->basic.WaveMaxFactorUb = 1414;
                }
                else
                {
                    for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += pPowerQuality->harmonic.HR.HRVb[i];
                    }

                    if(pPowerQuality->harmonic.HR.HRVb[1])
                        pPowerQuality->basic.WaveMaxFactorUb = 1414 * temp / pPowerQuality->harmonic.HR.HRVb[1];
                }
            }
            else
            {
                pPowerQuality->basic.WaveMaxFactorUb = 0;
            }

            if(pRealtime->Vca != 0)
            {
                temp = 0;
                if(pPowerQuality->harmonic.THD.THDVc == 0)
                {
                    pPowerQuality->basic.WaveMaxFactorUc = 1414;
                }
                else
                {
                    for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += pPowerQuality->harmonic.HR.HRVc[i];
                    }

                    if(pPowerQuality->harmonic.HR.HRVc[1])
                        pPowerQuality->basic.WaveMaxFactorUc = 1414 * temp / pPowerQuality->harmonic.HR.HRVc[1];
                }
            }
            else
            {
                pPowerQuality->basic.WaveMaxFactorUc = 0;
            }
            break;            
        }
        case P3W4:
        {
            if(pRealtime->Va != 0)
            {
                temp = 0;
                if(pPowerQuality->harmonic.THD.THDVa == 0)
                {
                    pPowerQuality->basic.WaveMaxFactorUa = 1414;
                }
                else
                {
                    for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += pPowerQuality->harmonic.HR.HRVa[i];
                    }

                    if(pPowerQuality->harmonic.HR.HRVa[1])
                        pPowerQuality->basic.WaveMaxFactorUa = 1414 * temp / pPowerQuality->harmonic.HR.HRVa[1];
                }
            }
            else
            {
                pPowerQuality->basic.WaveMaxFactorUa = 0;
            }

            if(pRealtime->Vb != 0)
            {
                temp = 0;
                if(pPowerQuality->harmonic.THD.THDVb == 0)
                {
                    pPowerQuality->basic.WaveMaxFactorUb = 1414;
                }
                else
                {
                    for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += pPowerQuality->harmonic.HR.HRVb[i];
                    }

                    if(pPowerQuality->harmonic.HR.HRVb[1])
                        pPowerQuality->basic.WaveMaxFactorUb = 1414 * temp / pPowerQuality->harmonic.HR.HRVb[1];
                }
            }
            else
            {
                pPowerQuality->basic.WaveMaxFactorUb = 0;
            }

            if(pRealtime->Vc != 0)
            {
                temp = 0;
                if(pPowerQuality->harmonic.THD.THDVc == 0)
                {
                    pPowerQuality->basic.WaveMaxFactorUc = 1414;
                }
                else
                {
                    for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += pPowerQuality->harmonic.HR.HRVc[i];
                    }
                    if(pPowerQuality->harmonic.HR.HRVc[1])
                        pPowerQuality->basic.WaveMaxFactorUc = 1414 * temp / pPowerQuality->harmonic.HR.HRVc[1];
                }
            }
            else
            {
                pPowerQuality->basic.WaveMaxFactorUc = 0;
            }
            break;            
        }
    }
    return 0;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_CurrentPeakCf
** Descriptions:        电流波峰系数
                        CF=1.414*∑Uh/U1
                        Uh为第h次谐波方均根值，U1为基波方均根值，N为最高谐波次数,从第1次谐波开始
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_CurrentPeakCf(struct db_database_measure_base_type *pMeasureBase)
{
    int16_t i = 0;
    uint16_t wireMode = 0;
    uint64_t temp = 0;
    struct db_cfg_user_para                       *pCfgUser = &(pMeasureBase->db_cfg.user);       
    struct db_measure_powerquality_type      *pPowerQuality = &(pMeasureBase->db_measure.powerquality); 
    struct db_basic_realtime_type                *pRealtime = &(pMeasureBase->db_measure.numeric.fullwave.primary); 

    wireMode = (pCfgUser->base.WiringMode.value & 0x03);

    switch (wireMode)
    {
        case P3W3:
        case P3W4:
        {
            if(pRealtime->Ia != 0)
            {
                temp = 0;
                if(pPowerQuality->harmonic.THD.THDIa == 0)
                {
                    pPowerQuality->basic.WaveMaxFactorIa = 1414;
                }
                else
                {
                    for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += pPowerQuality->harmonic.HR.HRIa[i];
                    }

                    if(pPowerQuality->harmonic.HR.HRIa[1])
                        pPowerQuality->basic.WaveMaxFactorIa = 1414 * temp / pPowerQuality->harmonic.HR.HRIa[1];
                }
            }
            else
            {
                pPowerQuality->basic.WaveMaxFactorIa = 0;
            }

            if(pRealtime->Ib != 0)
            {
                if(wireMode == P3W4)
                {
                    temp = 0;
                    if(pPowerQuality->harmonic.THD.THDIb == 0)
                    {
                        pPowerQuality->basic.WaveMaxFactorIb = 1414;
                    }
                    else
                    {
                        for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                        {
                            temp += pPowerQuality->harmonic.HR.HRIb[i];
                        }

                        if(pPowerQuality->harmonic.HR.HRIb[1])
                            pPowerQuality->basic.WaveMaxFactorIb = 1414 * temp / pPowerQuality->harmonic.HR.HRIb[1];
                    }
                }
                else
                {
                    pPowerQuality->basic.WaveMaxFactorIb = 1414;
                }
            }
            else
            {
                pPowerQuality->basic.WaveMaxFactorIb = 0;
            }

            if(pRealtime->Ic != 0)
            {
                temp = 0;
                if(pPowerQuality->harmonic.THD.THDIc == 0)
                {
                    pPowerQuality->basic.WaveMaxFactorIc = 1414;
                }
                else
                {
                    for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += pPowerQuality->harmonic.HR.HRIc[i];
                    }

                    if(pPowerQuality->harmonic.HR.HRIc[1])
                        pPowerQuality->basic.WaveMaxFactorIc = 1414 * temp / pPowerQuality->harmonic.HR.HRIc[1];
                }
            }
            else
            {
                pPowerQuality->basic.WaveMaxFactorIc = 0;
            }
            break; 
           
        }
    }
    return 0;
}



/*********************************************************************************************************
** Function name:       rms_algorithm_current_kfactor
** Descriptions:        电流K系数KF
                        KFactor=∑(Ih*h)^2/∑(Ih)^2
                        Ih为第h次谐波电流含有率，h为谐波次数，N为最高谐波次数,从第1次谐波开始
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_current_kfactor(struct db_database_measure_base_type *pMeasureBase)
{
    int16_t i = 0;
    uint16_t wireMode = 0;
    uint64_t temp = 0,temp1 = 0;
    struct db_cfg_user_para                       *pCfgUser = &(pMeasureBase->db_cfg.user);       
    struct db_measure_powerquality_type      *pPowerQuality = &(pMeasureBase->db_measure.powerquality); 
    struct db_basic_realtime_type                *pRealtime = &(pMeasureBase->db_measure.numeric.fullwave.primary); 

    wireMode = (pCfgUser->base.WiringMode.value & 0x03);
    switch (wireMode)
    {
        case P3W3:
        case P3W4:
        {
            if(pRealtime->Ia != 0)
            {
                temp = 0;
                temp1 = 0;
                if(pPowerQuality->harmonic.THD.THDIa == 0)
                {
                    pPowerQuality->basic.KFactorIa = 100;
                }
                else
                {
                    for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += (uint64_t)pPowerQuality->harmonic.HR.HRIa[i] * i * pPowerQuality->harmonic.HR.HRIa[i] * i;
                        temp1 += (uint64_t)pPowerQuality->harmonic.HR.HRIa[i] * pPowerQuality->harmonic.HR.HRIa[i];
                    }

                    if(temp1)
                    pPowerQuality->basic.KFactorIa = (double)temp / temp1 * 100;
                }
            }
            else
            {
                pPowerQuality->basic.KFactorIa = 0;            
            }

            if(pRealtime->Ib != 0)
            {
                temp = 0;
                temp1 = 0;
                if(pPowerQuality->harmonic.THD.THDIb == 0)
                {
                    pPowerQuality->basic.KFactorIb = 100;
                }
                else
                {
                    for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += (uint64_t)pPowerQuality->harmonic.HR.HRIb[i] * i * pPowerQuality->harmonic.HR.HRIb[i] * i;
                        temp1 += (uint64_t)pPowerQuality->harmonic.HR.HRIb[i] * pPowerQuality->harmonic.HR.HRIb[i];
                    }

                    if(temp1)
                    pPowerQuality->basic.KFactorIb = (double)temp / temp1 * 100;
                }
            }
            else
            {
                pPowerQuality->basic.KFactorIb = 0;            
            }

            if(pRealtime->Ic != 0)
            {
                temp = 0;
                temp1 = 0;
                if(pPowerQuality->harmonic.THD.THDIc == 0)
                {
                    pPowerQuality->basic.KFactorIc = 100;
                }
                else
                {
                    for(i = 1; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += (uint64_t)pPowerQuality->harmonic.HR.HRIc[i] * i * pPowerQuality->harmonic.HR.HRIc[i] * i ;
                        temp1 += (uint64_t)pPowerQuality->harmonic.HR.HRIc[i] * pPowerQuality->harmonic.HR.HRIc[i];
                    }

                    if(temp1)
                    pPowerQuality->basic.KFactorIc = (double)temp / temp1 * 100;
                }
            }
            else
            {
                pPowerQuality->basic.KFactorIc = 0;            
            }
            break;            
        }
    }
    return 0;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_current_tdd
** Descriptions:        电流TDD
                        谐波需量畸变率
                        TDD=sqrt(∑(Ih)^2) / I1
                        Ih为第h次谐波电流幅值，I1为额定电流
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_current_tdd(struct db_database_measure_base_type *pstVmem)
{
    int16_t i = 0;
    uint32_t tmpVal = 0;
    uint16_t wireMode = 0;
    uint64_t temp = 0;
    struct db_cfg_sys_para                         *pCfgSys = &(pstVmem->db_cfg.sys);
    struct db_cfg_user_para                       *pCfgUser = &(pstVmem->db_cfg.user);       
    struct db_measure_powerquality_type      *pPowerQuality = &(pstVmem->db_measure.powerquality); 
    struct db_numeric_fundwave_type              *pFundwave = &(pstVmem->db_measure.numeric.fundwave); 
    struct db_basic_realtime_type                *pRealtime = &(pstVmem->db_measure.numeric.fullwave.primary); 

    wireMode = (pCfgUser->base.WiringMode.value & 0x03);

    switch (wireMode)
    {  
        case P3W4:
        {
            if(pRealtime->Ia != 0)
            {
                temp = 0;
                if(pPowerQuality->harmonic.THD.THDIa == 0)
                {
                    pPowerQuality->basic.TDDIa = 0;
                }
                else
                {
                    for(i = 2; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += (uint64_t)pPowerQuality->harmonic.HR.HRIa[i] * pPowerQuality->harmonic.HR.HRIa[i] * pFundwave->secondary.Ia *pFundwave->secondary.Ia / 1000000;
                    }
                    pPowerQuality->basic.TDDIa = sqrt(temp) / pCfgSys->interface.RatedCurrent.value / 100;
                }
            }
            else
            {
                pPowerQuality->basic.TDDIa = 0;
            }
            if(pRealtime->Ib != 0)
            {
                temp = 0;
                if(pPowerQuality->harmonic.THD.THDIb == 0)
                {
                    pPowerQuality->basic.TDDIb = 0;
                }
                else
                {
                    for(i = 2; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += (uint64_t)pPowerQuality->harmonic.HR.HRIb[i] * pPowerQuality->harmonic.HR.HRIb[i] * pFundwave->secondary.Ib * pFundwave->secondary.Ib / 1000000;
                    }
                    pPowerQuality->basic.TDDIb = (double)sqrt(temp) / pCfgSys->interface.RatedCurrent.value / 100;
                }
            }
            else
            {
                pPowerQuality->basic.TDDIb = 0;
            }

            if(pRealtime->Ic != 0)
            {
                temp = 0;
                if(pPowerQuality->harmonic.THD.THDIc == 0)
                {
                    pPowerQuality->basic.TDDIc = 0;
                }
                else
                {
                    for(i = 2; i < DB_MSR_HARMONIC_0_63; i++)
                    {
                        temp += (uint64_t)pPowerQuality->harmonic.HR.HRIc[i] * pPowerQuality->harmonic.HR.HRIc[i] * pFundwave->secondary.Ic * pFundwave->secondary.Ic / 1000000;
                    }
                    pPowerQuality->basic.TDDIc = (double)sqrt(temp) / pCfgSys->interface.RatedCurrent.value / 100;
                }
            }
            else
            {
                pPowerQuality->basic.TDDIc = 0;
            }
            break;            
        }
    }
    return 0;
}





/*********************************************************************************************************
** Function name:       rms_algorithm_biased
** Descriptions:        频率偏差= 实测频率 -标称频率,精确到0.01Hz，
                        电压偏差(%) =（实测电压– 标称电压）/标称电压 × 100%                       
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_biased(struct db_database_measure_base_type *pstVmem, struct db_powerquality_basic_type *out, uint32_t pt, uint32_t ct)
{
    rms_algorithm_biased_voltage(pstVmem, out, pt, ct);
    rms_algorithm_biased_frequency(pstVmem, out, pt, ct);
    rms_algorithm_biased_loadrate(pstVmem, out, pt, ct);

    rms_algorithm_VoltagePeakCf(pstVmem);
    rms_algorithm_CurrentPeakCf(pstVmem);
    rms_algorithm_current_kfactor(pstVmem);
    rms_algorithm_current_tdd(pstVmem);
    return 0;
}

