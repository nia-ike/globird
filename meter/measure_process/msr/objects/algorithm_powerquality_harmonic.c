/*
*  Copyright (c) 2019,
*  All rights reserved.
*
*  文件名称：algorithm_powerquality_harmonic.c
*  文件标识：
*  描    述：偏差
*
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: rtthread
*  硬件平台：M4
*  固件库  ：
*  修改记录
*  作者       时间         版本      修改描述
*  chenzy      2021       V1.00
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
 

#include "msr_process.h"
#include "algorithm_powerquality_harmonic.h"

#include "arm_math.h"
#include "arm_const_structs.h"
#include "msr_process.h"
#include "Rn7302.h"
#include "Rn7302_phasecheck.h"


/* 谐波计算死区值 */
#define  VoltageLimit    2000  // 20V 
#define  CurrentLimit    400   // 40mA   

/* THD计算死区值 */
#define  THD_U           50
#define  THD_I           50

/* HR计算死区值 */
#define  HR_U            50   // 0.5%  
#define  HR_I            50   // 0.5%  

/* RMS计算死区值 */
#define  RMS_U           50    // 0.5V 
#define  RMS_I           100   // 10mA


/* rn7302设备缓存波形 */
static struct rn7302_waveform_buffer_t   waveform;
/* 谐波运算的点数缓存 */
static struct hm_fft_piont_buffer        rfft_points;


const float Fftcoefficient[THD_CNT]=                    
{
    1.0000,         // 0次 直流分量
    1.0000,         // 1次 基波 
    1.0005,         // 2次谐波
    1.0027,
    1.0054,
    1.0087,
    1.0129,
    1.0175,
    1.0229,
    1.029,      //10
    1.0362,
    1.0438,
    1.0524,
    1.0618,
    1.0718,
    1.0827,
    1.0944,
    1.1075,
    1.121,
    1.136,      //20
    1.1517,
    1.1685,
    1.1862,
    1.2051,
    1.2255,
    1.2475,
    1.27,
    1.2942,
    1.3199,
    1.3468,     //30
    1.3758,
    1.407,
    1.4421,
    1.4714,
    1.5107,
    1.5483,
    1.5873,
    1.6312,
    1.6727,
    1.7199,     //40
    1.77,
    1.8231,
    1.8797,
    1.9448,
    2.0074,
    2.074,
    2.1534,
    2.2126,
    2.2919,
    2.3762,     //50
    2.467,
    2.5639,
    2.6825,
    2.8061,
    2.9350,
    3.0041,
    3.1401,
    3.2816,
    3.4315,
    3.5859,     //60
    3.7486,
    4.0000,
    4.0986,
    4.2998,     //64   
};

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/




/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:           从驱动buffer中获取n个半周波到                                               **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            电流按 PhaseAdjust 重映射物理通道后再送入 FFT        **/
/*************************************************************************/
static int msr_harmonic_rfft_data(struct hm_fft_piont_buffer *prfft,
                                  struct rn7302_waveform_buffer_t *pwaveform,
                                  struct db_cfg_user_para *pCfgUser)
{
    int i;
    const int *I_phys[3] = {
        pwaveform->Ia,
        pwaveform->Ib,
        pwaveform->Ic,
    };
    uint16_t idx_a = Rn7302_phase_adj_to_ch_idx(pCfgUser->base.PhaseAdjust1.value);
    uint16_t idx_b = Rn7302_phase_adj_to_ch_idx(pCfgUser->base.PhaseAdjust2.value);
    uint16_t idx_c = Rn7302_phase_adj_to_ch_idx(pCfgUser->base.PhaseAdjust3.value);

    /* 将rn7302设备缓存波形数据复制到谐波运算的点数缓存 */
    for(i=0 ;i<WF_SAMPLES_CACHE_FRAME_SIZE; i++)
    { 
        prfft->Ua[i] = (float)pwaveform->Ua[i];
        prfft->Ub[i] = (float)pwaveform->Ub[i];
        prfft->Uc[i] = (float)pwaveform->Uc[i];
        /* 相序调整：逻辑相电流取自对应物理通道波形 */
        prfft->Ia[i] = (float)I_phys[idx_a][i];
        prfft->Ib[i] = (float)I_phys[idx_b][i];
        prfft->Ic[i] = (float)I_phys[idx_c][i];
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
static int msr_harmonic_rfft(float *p_in, uint32_t in_length, float *p_out, uint32_t out_length)
{
    arm_rfft_fast_instance_f32 S __attribute__((aligned(16)));

    /*
        init rfft f32

        input data size : length
    */
    arm_rfft_fast_init_f32(&S, in_length);

    /* 点实序列快速FFT ,  0 正变换 */
    arm_rfft_fast_f32(&S, p_in, p_out, 0);

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static void msr_harmonic_angle(int16_t *pangle, float r, float i)
{
     *pangle = atan2(i, r) * 180 / PI;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static void msr_harmonic_angle_clear(int16_t *pangle)
{
     *pangle = 0;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:           谐波畸变率                                                **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int msr_harmonic_thd(float mag[], uint32_t mag_n, uint16_t *thd, uint16_t *tohd, uint16_t *tehd, uint16_t deadzone)
{
    int i;
    uint16_t value;
    uint64_t n_rms, sum_rms, sum_odd, sum_even;
 
    if(mag_n != THD_CNT)
        return -1;

    n_rms = 0;
    sum_rms = 0;
    sum_odd = 0;
    sum_even = 0;
    /* 计算谐波畸变率：THD=√(∑(Amph/Amp1)^2 * 100%) */

    /*
        ∑(x^2):  Thd, Tohd, Tehd
    */
    for (i = 2; i < THD_CNT; i++)
    {
        n_rms = mag[i] * mag[i];

        sum_rms += n_rms;
        
        if(i & 1)
            sum_odd += n_rms;
        else 
            sum_even += n_rms;
    }

    /* 
        平方和 开根 / 基波值 = 含有率
        * 100000 = 100 * 100，  其中100是 100%， 另外100 是通讯保留2位小数
    */
    value = sqrt((float)sum_rms) * 10000 / mag[1];
    if(value < deadzone)
        *thd = 0;
    else 
        *thd = value;

    value = sqrt((float)sum_odd) * 10000 / mag[1];
    if(value < deadzone)
        *tohd = 0;
    else 
        *tohd = value;

    value = sqrt((float)sum_even) * 10000 / mag[1];
    if(value < deadzone)
        *tehd = 0;
    else 
        *tehd = value;

    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:           谐波畸变率                                                **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int msr_harmonic_thd_clear(float mag[], uint32_t mag_n, uint16_t *thd, uint16_t *tohd, uint16_t *tehd, uint16_t deadzone)
{
    *thd = 0;
    *tohd = 0;
    *tehd = 0;

    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:           校正                                                **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int msr_harmonic_coe(float mag[], uint32_t mag_n)
{
    int i;
    if(mag_n != THD_CNT)
        return -1;

    for (i=0; i < THD_CNT; i++)
    {
        mag[i] = mag[i] * Fftcoefficient[i];
    }

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:           谐波含有率                                                **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int msr_harmonic_ratio(float mag[], uint16_t hr[], uint32_t mag_n, uint16_t deadzone)
{
    int i;
    uint16_t hr_val;
    
    if(mag_n != THD_CNT)
        return -1;
    /* 
        计算谐波含有率：HR=Amph/Amp1 * 100% 
        从二次谐波开始计算含有率
        */

    for (i=0; i < THD_CNT; i++)
    {
        /*
            
             * 100000 = 100 * 100，  其中100是 100%， 另外100 是通讯保留2位小数
        */
        hr_val = (uint64_t)mag[i] * 10000 / mag[1];
        if(hr_val < deadzone)
            hr_val = 0;

        hr[i] = hr_val;
    }

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:           谐波含有率                                                **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int msr_harmonic_ratio_clear(float mag[], uint16_t hr[], uint32_t mag_n, uint16_t deadzone)
{
    int i;
    
    if(mag_n != THD_CNT)
        return -1;
    /* 
        计算谐波含有率：HR=Amph/Amp1 * 100% 
        从二次谐波开始计算含有率
        */

    for (i=0; i < THD_CNT; i++)
    {
        hr[i] = 0;
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
static uint32_t msr_harmonic_to_rms(uint16_t hr, uint32_t fundval)
{
    if(hr == 0)
        return 0;
    else 
        return (uint64_t)fundval * hr / 10000;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:           谐波含有率                                                **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int msr_harmonic_volt_rms(struct db_database_measure_base_type *pMeasureBase)
{
    int i;
    uint16_t wire_mode;
    struct db_cfg_user_para              *pCfgUser  = &pMeasureBase->db_cfg.user;
    struct db_powerquality_harmonic_type *pharmonic = &pMeasureBase->db_measure.powerquality.harmonic;
    struct db_basic_realtime_type *pRealtime = &(pMeasureBase->db_measure.numeric.fundwave.primary);

    wire_mode = pCfgUser->base.WiringMode.value & 3;
    switch (wire_mode)
    {
    case P3W3:
        for(i=0; i<DB_MSR_HARMONIC_0_63; i++)
        {
            pharmonic->RMS.VaValue[i] = msr_harmonic_to_rms(pharmonic->HR.HRVa[i], pRealtime->Vab);
            pharmonic->RMS.VbValue[i] = 0;
            pharmonic->RMS.VcValue[i] = msr_harmonic_to_rms(pharmonic->HR.HRVc[i], pRealtime->Vca);

        }
        break;
    case P3W4:
        for(i=0; i<DB_MSR_HARMONIC_0_63; i++)
        {
            pharmonic->RMS.VaValue[i] = msr_harmonic_to_rms(pharmonic->HR.HRVa[i], pRealtime->Va);
            pharmonic->RMS.VbValue[i] = msr_harmonic_to_rms(pharmonic->HR.HRVb[i], pRealtime->Vb);
            pharmonic->RMS.VcValue[i] = msr_harmonic_to_rms(pharmonic->HR.HRVc[i], pRealtime->Vc);

        }
        break;
    default:
        break;
    }

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:           谐波含有率                                                **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static int msr_harmonic_current_rms(struct db_database_measure_base_type *pMeasureBase)
{
    int i;
    uint16_t wire_mode;
    struct db_cfg_user_para              *pCfgUser  = &pMeasureBase->db_cfg.user;
    struct db_powerquality_harmonic_type *pharmonic = &pMeasureBase->db_measure.powerquality.harmonic;
    struct db_basic_realtime_type *pRealtime = &(pMeasureBase->db_measure.numeric.fundwave.primary);

    wire_mode = pCfgUser->base.WiringMode.value & 3;
    switch (wire_mode)
    {
    case P3W3:
    case P3W4:
        for(i=0; i<DB_MSR_HARMONIC_0_63; i++)
        {
            pharmonic->RMS.IaValue[i] = msr_harmonic_to_rms(pharmonic->HR.HRIa[i], pRealtime->Ia);
            pharmonic->RMS.IbValue[i] = msr_harmonic_to_rms(pharmonic->HR.HRIb[i], pRealtime->Ib);
            pharmonic->RMS.IcValue[i] = msr_harmonic_to_rms(pharmonic->HR.HRIc[i], pRealtime->Ic);

        }
        break;
    default:
        break;
    }

    return 0;
}


/*********************************************************************************************************
** Function name:       rn7302_ioctl_waveform_start
** Descriptions:        启动rn7302设备缓存波形
** input parameters:       pctx - 测量上下文
** output parameters:      NONE
** Returned value:        0 - 成功，-1 - 失败
*********************************************************************************************************/
static int rn7302_ioctl_waveform_start(struct measure_ctx *pctx)
{
    /* 启动rn7302设备缓存波形 */
    ioctl(pctx->fd_device, RN7302_IOCTL_WAVEFORM_START, 0);
    return 0;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static int rn7302_ioctl_waveform_read(struct measure_ctx *pctx, struct rn7302_waveform_buffer_t *pwaveform)
{
    /* 读取rn7302设备缓存波形 */
    
    return ioctl(pctx->fd_device, RN8306_IOCTL_WAVEFORM_READ, pwaveform);
}


/*********************************************************************************************************
** Function name:       msr_harmonic_thread_proc_entry
** Descriptions:        谐波运算线程入口
** input parameters:    args - 测量上下文
** output parameters:   NONE
** Returned value:      void
*********************************************************************************************************/
static void *msr_harmonic_thread_proc_entry(void *args)
{
    int ret;
    uint16_t wire_mode;
    struct measure_ctx                                *pctx = (struct measure_ctx *)args;
    struct db_database_type                        *pstVmem = (struct db_database_type *)pctx->pdb;
    struct db_database_measure_base_type          *pMeasure = &(pstVmem->db_measurebase[pctx->chip]);
    struct db_cfg_user_para                       *pCfgUser = &(pMeasure->db_cfg.user);
    struct db_powerquality_harmonic_type          *pharmonic= &pMeasure->db_measure.powerquality.harmonic;
    
    wire_mode = pCfgUser->base.WiringMode.value & 3;
    while(1)
    {
        /* 启动rn7302设备缓存波形 */
        rn7302_ioctl_waveform_start(pctx);
        /* 延时30ms, 缓存1个周波 */
        MSR_PORT_DELAY(200);
        /* 读取rn7302设备缓存波形 */
        ret = rn7302_ioctl_waveform_read(pctx, &waveform);
        if(ret < 0)
        {
            printf("rn7302_ioctl_waveform_read failed\n");
            MSR_PORT_DELAY(1000);
            continue;
        }
        /* 将rn7302设备缓存波形数据复制到谐波运算的点数缓存（电流按相序重映射） */
        msr_harmonic_rfft_data(&rfft_points, &waveform, pCfgUser);

        /* 开始计时 */
//        struct timespec start_time, end_time;
//        clock_gettime(CLOCK_MONOTONIC, &start_time);

        if(pMeasure->db_measure.numeric.fullwave.secondary.Va > VoltageLimit)
        {
             /*
                rfft 结果放置 rfft_points.output，  数据格式为   R  I  R  I....  
            */
            msr_harmonic_rfft(rfft_points.Ua, sizeof(rfft_points.Ua)/sizeof(float), rfft_points.output, sizeof(rfft_points.Ua)/sizeof(float));
            /* 计算幅值 ，实部虚部求模后存放回 rfft_points.Ua */
            arm_cmplx_mag_f32(rfft_points.output, rfft_points.Ua, sizeof(rfft_points.Ua)/sizeof(float));
            /* 衰减补偿校正。注意 ：不能开启 RN7302_REG_DMA_WAVECON2 的硬件补偿 */
            msr_harmonic_coe(rfft_points.Ua, THD_CNT);

            msr_harmonic_thd(rfft_points.Ua, THD_CNT, &pharmonic->THD.THDVa, &pharmonic->THD.TOHDVa, &pharmonic->THD.TEHDVa, THD_U);
            msr_harmonic_ratio(rfft_points.Ua, pharmonic->HR.HRVa, THD_CNT, HR_U);
            // 0 1 -- DC   2 3 -- fundance R I
            msr_harmonic_angle(&pharmonic->Angle.VaAngle, rfft_points.output[2], rfft_points.output[3]);

//            printf("pharmonic->HR.HRVa[3]:%d\n", pharmonic->HR.HRVa[3]);
//            printf("rfft_points.Ua[1]:%f  rfft_points.Ua[3]:%f\n", rfft_points.Ua[1], rfft_points.Ua[3]);
//
//            if(pharmonic->HR.HRVa[3] > 1000)
//            {
//                printf("==============================================\n");
//                for(int i=0; i<128; i++)
//                {
//                    printf("%d\n", waveform.Ua[i]);
//                }
//
//                printf("---------------------------------------------\n");
//            }
        }
        else 
        {
            msr_harmonic_thd_clear(rfft_points.Ua, THD_CNT, &pharmonic->THD.THDVa, &pharmonic->THD.TOHDVa, &pharmonic->THD.TEHDVa, THD_U);
            msr_harmonic_ratio_clear(rfft_points.Ua, pharmonic->HR.HRVa, THD_CNT, HR_U);
            msr_harmonic_angle_clear(&pharmonic->Angle.VaAngle);
        }
        
        if(pMeasure->db_measure.numeric.fullwave.secondary.Vb > VoltageLimit)
        {
            if(wire_mode == P3W4)
            {
                msr_harmonic_rfft(rfft_points.Ub, sizeof(rfft_points.Ub)/sizeof(float), rfft_points.output, sizeof(rfft_points.Ub)/sizeof(float));
                arm_cmplx_mag_f32(rfft_points.output, rfft_points.Ub, sizeof(rfft_points.Ub)/sizeof(float));
                /* 衰减补偿校正。注意 ：不能开启 RN7302_REG_DMA_WAVECON2 的硬件补偿 */
                msr_harmonic_coe(rfft_points.Ub, THD_CNT);
                msr_harmonic_thd(rfft_points.Ub, THD_CNT, &pharmonic->THD.THDVb, &pharmonic->THD.TOHDVb, &pharmonic->THD.TEHDVb, THD_U);
                msr_harmonic_ratio(rfft_points.Ub, pharmonic->HR.HRVb, THD_CNT, HR_U);
                // 0 1 -- DC   2 3 -- fundance R I
                msr_harmonic_angle(&pharmonic->Angle.VbAngle, rfft_points.output[2], rfft_points.output[3]);
            }
            else 
            {
                msr_harmonic_thd_clear(rfft_points.Ub, THD_CNT, &pharmonic->THD.THDVb, &pharmonic->THD.TOHDVb, &pharmonic->THD.TEHDVb, THD_U);
                msr_harmonic_ratio_clear(rfft_points.Ub, pharmonic->HR.HRVb, THD_CNT, HR_U);
                msr_harmonic_angle_clear(&pharmonic->Angle.VbAngle);
            }
        }
        else 
        {
            msr_harmonic_thd_clear(rfft_points.Ub, THD_CNT, &pharmonic->THD.THDVb, &pharmonic->THD.TOHDVb, &pharmonic->THD.TEHDVb, THD_U);
            msr_harmonic_ratio_clear(rfft_points.Ub, pharmonic->HR.HRVb, THD_CNT, HR_U);
            msr_harmonic_angle_clear(&pharmonic->Angle.VbAngle);
        }

        if(pMeasure->db_measure.numeric.fullwave.secondary.Vc > VoltageLimit)
        {
            msr_harmonic_rfft(rfft_points.Uc, sizeof(rfft_points.Uc)/sizeof(float), rfft_points.output, sizeof(rfft_points.Uc)/sizeof(float));
            arm_cmplx_mag_f32(rfft_points.output, rfft_points.Uc, sizeof(rfft_points.Uc)/sizeof(float));
            /* 衰减补偿校正。注意 ：不能开启 RN7302_REG_DMA_WAVECON2 的硬件补偿 */
            msr_harmonic_coe(rfft_points.Uc, THD_CNT);
            msr_harmonic_thd(rfft_points.Uc, THD_CNT, &pharmonic->THD.THDVc, &pharmonic->THD.TOHDVc, &pharmonic->THD.TEHDVc, THD_U);
            msr_harmonic_ratio(rfft_points.Uc, pharmonic->HR.HRVc, THD_CNT, HR_U);
            // 0 1 -- DC   2 3 -- fundance R I
            msr_harmonic_angle(&pharmonic->Angle.VcAngle, rfft_points.output[2], rfft_points.output[3]);
        }
        else 
        {
            msr_harmonic_thd_clear(rfft_points.Uc, THD_CNT, &pharmonic->THD.THDVc, &pharmonic->THD.TOHDVc, &pharmonic->THD.TEHDVc, THD_U);
            msr_harmonic_ratio_clear(rfft_points.Uc, pharmonic->HR.HRVc, THD_CNT, HR_U);
            msr_harmonic_angle_clear(&pharmonic->Angle.VcAngle);
        }

        if(pMeasure->db_measure.numeric.fullwave.secondary.Ia > CurrentLimit)
        {
            msr_harmonic_rfft(rfft_points.Ia, sizeof(rfft_points.Ia)/sizeof(float), rfft_points.output, sizeof(rfft_points.Ia)/sizeof(float));
            arm_cmplx_mag_f32(rfft_points.output, rfft_points.Ia, sizeof(rfft_points.Ia)/sizeof(float));
            /* 衰减补偿校正。注意 ：不能开启 RN7302_REG_DMA_WAVECON2 的硬件补偿 */
            msr_harmonic_coe(rfft_points.Ia, THD_CNT);
            msr_harmonic_thd(rfft_points.Ia, THD_CNT, &pharmonic->THD.THDIa, &pharmonic->THD.TOHDIa, &pharmonic->THD.TEHDIa, THD_I);
            msr_harmonic_ratio(rfft_points.Ia, pharmonic->HR.HRIa, THD_CNT, HR_I);
            // 0 1 -- DC   2 3 -- fundance R I
            msr_harmonic_angle(&pharmonic->Angle.IaAngle, rfft_points.output[2], rfft_points.output[3]);
        }
        else 
        {
            msr_harmonic_thd_clear(rfft_points.Ia, THD_CNT, &pharmonic->THD.THDIa, &pharmonic->THD.TOHDIa, &pharmonic->THD.TEHDIa, THD_I);
            msr_harmonic_ratio_clear(rfft_points.Ia, pharmonic->HR.HRIa, THD_CNT, HR_I);
            msr_harmonic_angle_clear(&pharmonic->Angle.IaAngle);
        }

        if(pMeasure->db_measure.numeric.fullwave.secondary.Ib > CurrentLimit)
        {
            msr_harmonic_rfft(rfft_points.Ib, sizeof(rfft_points.Ib)/sizeof(float), rfft_points.output, sizeof(rfft_points.Ib)/sizeof(float));
            arm_cmplx_mag_f32(rfft_points.output, rfft_points.Ib, sizeof(rfft_points.Ib)/sizeof(float));
            /* 衰减补偿校正。注意 ：不能开启 RN7302_REG_DMA_WAVECON2 的硬件补偿 */
            msr_harmonic_coe(rfft_points.Ib, THD_CNT);
            msr_harmonic_thd(rfft_points.Ib, THD_CNT, &pharmonic->THD.THDIb, &pharmonic->THD.TOHDIb, &pharmonic->THD.TEHDIb, THD_I);
            msr_harmonic_ratio(rfft_points.Ib, pharmonic->HR.HRIb, THD_CNT, HR_I);
            // 0 1 -- DC   2 3 -- fundance R I
            msr_harmonic_angle(&pharmonic->Angle.IbAngle, rfft_points.output[2], rfft_points.output[3]);
        }
        else 
        {
            msr_harmonic_thd_clear(rfft_points.Ib, THD_CNT, &pharmonic->THD.THDIb, &pharmonic->THD.TOHDIb, &pharmonic->THD.TEHDIb, THD_I);
            msr_harmonic_ratio_clear(rfft_points.Ib, pharmonic->HR.HRIb, THD_CNT, HR_I);
            msr_harmonic_angle_clear(&pharmonic->Angle.IbAngle);
        }

        if(pMeasure->db_measure.numeric.fullwave.secondary.Ic > CurrentLimit)
        {
            msr_harmonic_rfft(rfft_points.Ic, sizeof(rfft_points.Ic)/sizeof(float), rfft_points.output, sizeof(rfft_points.Ic)/sizeof(float));
            arm_cmplx_mag_f32(rfft_points.output, rfft_points.Ic, sizeof(rfft_points.Ic)/sizeof(float));
            /* 衰减补偿校正。注意 ：不能开启 RN7302_REG_DMA_WAVECON2 的硬件补偿 */
            msr_harmonic_coe(rfft_points.Ic, THD_CNT);
            msr_harmonic_thd(rfft_points.Ic, THD_CNT, &pharmonic->THD.THDIc, &pharmonic->THD.TOHDIc, &pharmonic->THD.TEHDIc, THD_I);
            msr_harmonic_ratio(rfft_points.Ic, pharmonic->HR.HRIc, THD_CNT, HR_I);
            // 0 1 -- DC   2 3 -- fundance R I
            msr_harmonic_angle(&pharmonic->Angle.IcAngle, rfft_points.output[2], rfft_points.output[3]);
        }
        else 
        {
            msr_harmonic_thd_clear(rfft_points.Ic, THD_CNT, &pharmonic->THD.THDIc, &pharmonic->THD.TOHDIc, &pharmonic->THD.TEHDIc, THD_I);
            msr_harmonic_ratio_clear(rfft_points.Ic, pharmonic->HR.HRIc, THD_CNT, HR_I);
            msr_harmonic_angle_clear(&pharmonic->Angle.IcAngle);
        }

        msr_harmonic_volt_rms(pMeasure);
        msr_harmonic_current_rms(pMeasure);

        /* 结束计时并打印运行时间 */
//        clock_gettime(CLOCK_MONOTONIC, &end_time);
//        double elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + 
//                              (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
//        printf("谐波计算运行时间: %.3f ms\n", elapsed_time);

        /**
         *  calculate the harmonic every 1 seconds
        */
        MSR_PORT_DELAY(1000);
    }

    return NULL;
}





/*********************************************************************************************************
** Function name:       msr_harmonic_init
** Descriptions:        
        谐波运算初始化
** input parameters:    
** output parameters:   pctx - 测量上下文
** Returned value:      0 - 成功，-1 - 失败
*********************************************************************************************************/
int32_t msr_harmonic_init(struct measure_ctx *pctx)
{
    pthread_t hWaveformThread;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (!pthread_create(&hWaveformThread, &attr, msr_harmonic_thread_proc_entry, pctx) == 0)
    {
        printf("Create Waveform Thread failed.\n");
    }
    
    return 0;
}




