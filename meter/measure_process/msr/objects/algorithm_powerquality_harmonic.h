/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_powerquality_biased.h
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


#ifndef _ALGORITHM_POWERQUALITY_HARMONIC_H__
#define _ALGORITHM_POWERQUALITY_HARMONIC_H__

#include <stdint.h>
#include <time.h>
#include "db_struct.h"
#include "Rn7302.h"



#define THD_CNT                     64
//#define DB_MSR_HARMONIC_0_63        (64)            /* 直流分量，基波，2 - 63次谐波 */
//#define DB_MSR_HARMONIC_0_63             (62)            /* 2 - 63次谐波 */

/* 
    谐波运算的点数 
*/
#define WF_SAMPLES_CACHE_FRAME_SIZE 128
#define WF_N                        1


/* 
    rn7302设备缓存波形
    从驱动中读取的波形数据
    Ua, Ub, Uc 为电压数据
    Ia, Ib, Ic 为电流数据
*/
struct rn7302_waveform_buffer_t 
{
    int     Ua[WF_SAMPLES_CACHE_FRAME_SIZE];
    int     Ub[WF_SAMPLES_CACHE_FRAME_SIZE];
    int     Uc[WF_SAMPLES_CACHE_FRAME_SIZE];
    int     Ia[WF_SAMPLES_CACHE_FRAME_SIZE];
    int     Ib[WF_SAMPLES_CACHE_FRAME_SIZE];
    int     Ic[WF_SAMPLES_CACHE_FRAME_SIZE];
};


/*
    用于谐波运算，使用 周波的N倍进行谐波运算
    每个周波的N倍进行谐波运算
    注意：NEON SIMD 要求16字节对齐
*/
struct hm_fft_piont_buffer
{
    float Ua[WF_SAMPLES_CACHE_FRAME_SIZE * WF_N] __attribute__((aligned(16)));
    float Ub[WF_SAMPLES_CACHE_FRAME_SIZE * WF_N] __attribute__((aligned(16)));
    float Uc[WF_SAMPLES_CACHE_FRAME_SIZE * WF_N] __attribute__((aligned(16)));
    float Ia[WF_SAMPLES_CACHE_FRAME_SIZE * WF_N] __attribute__((aligned(16)));
    float Ib[WF_SAMPLES_CACHE_FRAME_SIZE * WF_N] __attribute__((aligned(16)));
    float Ic[WF_SAMPLES_CACHE_FRAME_SIZE * WF_N] __attribute__((aligned(16)));

    float output[WF_SAMPLES_CACHE_FRAME_SIZE * WF_N] __attribute__((aligned(16)));
} __attribute__((aligned(16)));


/*
    rfft 输出的结果位  r  i   r  i 的交替数据
    可用如下结构体分离 r  i
*/
struct hm_fft_ri_split 
{
    float r;
    float i;
};

union hm_fft_mix_to_split
{
    float mix[WF_SAMPLES_CACHE_FRAME_SIZE * WF_N];
    struct hm_fft_ri_split split[WF_SAMPLES_CACHE_FRAME_SIZE];
};
/* END OF rfft split */


int32_t msr_harmonic_init(struct measure_ctx *pctx);


#endif
