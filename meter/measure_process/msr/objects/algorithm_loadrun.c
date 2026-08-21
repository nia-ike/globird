/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_loadrun.c
*  文件标识：
*  描    述：负载运行
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
#include "algorithm_loadrun.h"


#define max3(x, y, z) ({ typeof(x) _max1 = (x); typeof(y) _max2 = (y); typeof(z) _max3 = (z); (void) (&_max1 == &_max2); (void) (&_max1 == &_max3); _max1 > _max2 ? (_max1 > _max3 ? _max1 : _max3) : (_max2 > _max3 ? _max2 : _max3); })
#define min3(x, y, z) ({ typeof(x) _min1 = (x); typeof(y) _min2 = (y); typeof(z) _min3 = (z); (void) (&_min1 == &_min2); (void) (&_min1 == &_min3); _min1 < _min2 ? (_min1 < _min3 ? _min1 : _min3) : (_min2 < _min3 ? _min2 : _min3); })

#define TIME  1

static uint16_t last_obj = 0;

/*******************************************************************************
函数名称：DemandInit()
功    能：需量初始化函数
子 函 数：DemondReset();
调用限制：无
输入参数：无
输出参数：无
返 回 值：无
说    明：无
*******************************************************************************/
int32_t LoadrunInit(struct db_database_measure_base_type *pMeasureBase)
{
    struct db_cfg_user_loadrun_para      *pCfgUser  = &(pMeasureBase->db_cfg.user.loadrun);                                        /* 获取接线模式 */
    struct db_statistical_loadrun_type *pLoadRun = &(pMeasureBase->db_measure.statistical.loadrun);
    pLoadRun->doh_uptime = 0;
    //解决有负载对象情况重启后最后对象为默认0的问题
    last_obj = pCfgUser->obj.value;
    return 0;
}

/*********************************************************************************************************
** Function name:       rms_algorithm_loadrun
** Descriptions:        负载运行时间（LRH） 单位h,分辨率0.1
                        负载上电时间（LOH） 单位h,分辨率0.1
                        负载运行次数（LRC）
                        仪表运行时间（DOH） 单位h,分辨率0.1
                        负载对象是电流/总有功功率时：
                                        实时值非零值，累计负载上电时间
                                        实时值超过负载阀值，累计负载运行时间，且累计1次负载运行次数
                        负载对象是电压时：
                                        实时值非零值，累计负载上电时间
                                        电压最小阀值 < 实时值 < 电压最大阀值，累计负载运行时间，且累计1次负载运行次数
                        改变负载对象，负载运行时间、负载上电时间、负载运行次数清零
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_loadrun(struct db_database_measure_base_type *pMeasureBase, struct db_statistical_loadrun_type *out, uint32_t pt, uint32_t ct)
{
    struct db_cfg_user_loadrun_para      *pCfgLoad  = &(pMeasureBase->db_cfg.user.loadrun);                                        /* 获取接线模式 */
    struct db_basic_realtime_type       *pRealtime  = &(pMeasureBase->db_measure.numeric.fullwave.primary); /* 将实际数据更新到二次侧 */
    struct db_cfg_user_para      *pCfgUser = &(pMeasureBase->db_cfg.user);
    static uint16_t lrc_flg = 0;
    static uint64_t temp_loh = 0,temp_lrh = 0,temp_doh = 0,temp_uptime = 0;
    uint32_t over_V = 0,under_V = 0;
    uint16_t                    mode = 0;
	
    over_V = (pCfgLoad->over_V.value);
    under_V = (pCfgLoad->under_V.value);
    mode = (pCfgUser->base.WiringMode.value & 0x03);

    if(last_obj != pCfgLoad->obj.value)
    {
        last_obj = pCfgLoad->obj.value;
        out->loh = 0;
        out->lrh = 0;
        out->lrc = 0;
        temp_loh = 0;
        temp_lrh = 0;
        lrc_flg = 0;
    }

    switch (pCfgLoad->obj.value)
    {
    case OBJ_NULL:
        break;
    case OBJ_VOLTAGE:
        if(mode == P3W4)
        {
            if(min3(pRealtime->Va,pRealtime->Vb,pRealtime->Vc) != 0)
            {
                temp_loh++;
                out->loh += (temp_loh / TIME);
                temp_loh = temp_loh % TIME;
            }
            if((over_V != 0) && (((pRealtime->Va >= under_V) && (pRealtime->Va <= over_V)) || ((pRealtime->Vb >= under_V) && (pRealtime->Vb <= over_V)) || ((pRealtime->Vc >= under_V) && (pRealtime->Vc <= over_V))))
            {
                temp_lrh++;
                out->lrh += (temp_lrh / TIME);
                temp_lrh = temp_lrh % TIME;
                if(lrc_flg == 0)
                {
                    out->lrc++;
                    lrc_flg = 1;
                }
            }
            else
            {
                lrc_flg = 0;
            }
        }
        else if(mode == P3W3)
        {
            if(min3(pRealtime->Vab,pRealtime->Vbc,pRealtime->Vca) != 0)
            {
                temp_loh++;
                out->loh += (temp_loh / TIME);
                temp_loh = temp_loh % TIME;
            }
            if((over_V != 0) && (((pRealtime->Vab >= under_V) && (pRealtime->Vab <= over_V)) || ((pRealtime->Vbc >= under_V) && (pRealtime->Vbc <= over_V)) || ((pRealtime->Vca >= under_V) && (pRealtime->Vca <= over_V))))
            {

                temp_lrh++;
                out->lrh += (temp_lrh / TIME);
                temp_lrh = temp_lrh % TIME;
                if(lrc_flg == 0)
                {
                    out->lrc++;
                    lrc_flg = 1;
                }
            }
            else
            {
                lrc_flg = 0;
            }
        }
        break;
    case OBJ_CURRENT:
        if(min3(pRealtime->Ia,pRealtime->Ib,pRealtime->Ic) != 0)
        {
            temp_loh++;
            out->loh += (temp_loh / TIME);
            temp_loh = temp_loh % TIME;
        }
        if((pCfgLoad->over_I.value != 0) && (max3(pRealtime->Ia,pRealtime->Ib,pRealtime->Ic) > (pCfgLoad->over_I.value)))
        {
            temp_lrh++;
            out->lrh += (temp_lrh / TIME);
            temp_lrh = temp_lrh % TIME;
            if(lrc_flg == 0)
            {
                out->lrc++;
                lrc_flg = 1;
            }
        }
        else
        {
            lrc_flg = 0;
        }
        break;
    case OBJ_POWER:           
        //设定负荷总有功功率阀值时才有效
        if(pRealtime->Ptot != 0)
        {
            temp_loh++;
            out->loh += (temp_loh / TIME);
            temp_loh = temp_loh % TIME;
        }
        if((pCfgLoad->over_Ptot.value != 0) && (llabs(pRealtime->Ptot) > labs(pCfgLoad->over_Ptot.value)))
        {
            temp_lrh++;
            out->lrh += (temp_lrh / TIME);
            temp_lrh = temp_lrh % TIME;
            if(lrc_flg == 0)
            {
                out->lrc++;
                lrc_flg = 1;
            }
        }
        else
        {
            lrc_flg = 0;
        }
        break;        
    default:
        break;
    }

// printf("%d %d %d %d %s %d\r\n",pCfgUser->base.loadrun.obj.value,pLoadRun->loh, pLoadRun->lrh,  pLoadRun->lrc,__FILE__, __LINE__);
   
    return 0;
}
