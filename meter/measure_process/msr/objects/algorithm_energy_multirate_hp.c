/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：
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
*/


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Rn7302.h"
#include "algorithm_energy_multirate_hp.h"
#include "db_cfg_user_multirate.h"
#include "db_struct.h"
#include "db_device.h"
#include "algorithm_energy_remain.h"

/*
    复费率的[月日]、[时分]等时间相关的段，使用整型描述（1231， 即12月31日， 或者12时31分）
    方便检索时以整型比大小
*/
#define MULTIRATE_BCD_CONVENT(x, y)         ((x*100) + y)



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int multirate_table_select(struct db_cfg_user_multirate_para *pcfg, time_t now)
{
    /*
        费率表选择
        当前时间 < 设定切换时间， 使用费率表1
        反之 则启用新费率表
    */
    if((now < pcfg->unix_switch.value) || (pcfg->unix_switch.value == 0))
        return 0;
    else  
        return 1;
}





/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static struct db_cfg_multirate_unit_type *multirate_general_unit_find(struct db_cfg_multirate_day_type *pcfg, struct db_cfg_multirate_day_table_type *pday_tbl, uint16_t bcd_now_hm, int *pindex)
{
    int i;
    struct db_cfg_multirate_unit_group_type *punit_group;

/* 日最后时段 BCD格式  2359 */
#define DAY_END              2359
#define DAY_NOTSET           0xffff
    /* 
        检索时段原理： 从0开始查看当前时间是否所属  计费季起始月日 与下一个计费季起始月日。

        第0次 |__0___|___1___|
        第1次 |__1___|___2___|
        第2次 |__2___|___3___|
        ........
        第10次|__10__|__11___|
        ----------------------------------------------
        第11次|__11__|__MD_END___|

        ==================================================
        配置文件定义： 

    */

    /* 时段索引无效 */
    if(pcfg->day.value >= MULTRATE_NUM_DAY_TABLE)
        return NULL;

    /* 根据时段索引在时段表中找到对应的时段 */
    punit_group = &pday_tbl->table_day[pcfg->day.value];

    /* 第0次 - 第2次 */
    for(i=0;i<(MULTRATE_NUM_DAY-1);i++)
    {
        /* 遍历时，下一个计费季起始时间无效，即当前计费季为所属 */
        if(punit_group->day[i+1].s_hm.value == DAY_NOTSET)
        {
            *pindex = i;
            return &punit_group->day[i];
        }

        /*  当前md  <= now < 下一个md， 计费季找到   */
        if((punit_group->day[i].s_hm.value <= bcd_now_hm) && (bcd_now_hm < punit_group->day[i+1].s_hm.value))
        {
            *pindex = i;
            return &punit_group->day[i];
        }
    }

    /* 最后一段 */
    if((punit_group->day[MULTRATE_NUM_DAY-1].s_hm.value <= bcd_now_hm) && (bcd_now_hm <= DAY_END))
    {
        *pindex = MULTRATE_NUM_DAY-1;
        return &punit_group->day[MULTRATE_NUM_DAY-1];
    }

    return NULL;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static struct db_cfg_multirate_week_type *multirate_general_week_find(struct db_cfg_multirate_general_type *pcfg, uint16_t bcd_now_md, int *pindex)
{
    int i;

/* 计费季最后日期 BCD格式  12月31日 */
#define MD_END              1231
#define MD_NOTSET           0xffff
    /* 
        检索计费季原理： 从0开始查看当前时间是否所属  计费季起始月日 与下一个计费季起始月日。

        第0次 |__0___|___1___|
        第1次 |__1___|___2___|
        第2次 |__2___|___3___|
        ........
        第10次|__10__|__11___|
        ----------------------------------------------
        第11次|__11__|__MD_END___|

        ==================================================
        配置文件定义： 从01月01日开始，计费季有效 s_md.value != 0xFFFF

    */

    /* 第一个计费季 起始时间无效，直接返回无 */
    if(pcfg->season[0].s_md.value == MD_NOTSET)
        return NULL;

    /* 第0次 - 第10次 */
    for(i=0;i<(MULTRATE_NUM_SEASON-1);i++)
    {
        /* 遍历时，下一个计费季起始时间无效，即当前计费季为所属 */
        if(pcfg->season[i+1].s_md.value == MD_NOTSET)
        {
            *pindex = i;
            return &pcfg->season[i];
        }

        /*  当前md  <= now < 下一个md， 计费季找到   */
        if((pcfg->season[i].s_md.value <= bcd_now_md) && (bcd_now_md < pcfg->season[i+1].s_md.value))
        {
            *pindex = i;
            return &pcfg->season[i];
        }
    }

    /* 第11次 */
    if((pcfg->season[MULTRATE_NUM_SEASON-1].s_md.value <= bcd_now_md) && (bcd_now_md <= MD_END))
    {
        *pindex = i;
        return &pcfg->season[MULTRATE_NUM_SEASON-1];
    }

    return NULL;
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int multirate_general_week_index_find(struct db_cfg_multirate_week_type *pcfg, struct db_cfg_multirate_week_table_type *pweek_tbl, uint16_t bcd_now_week)
{
    int i;

/* 周最后日期 BCD格式  7 */
#define WEEK_END              7
#define WEEK_NOTSET           0xffff
    /* 
        检索计费季原理： 从0开始查看当前时间是否所属  计费季起始月日 与下一个计费季起始月日。

        第0次 |__0___|___1___|
        第1次 |__1___|___2___|
        第2次 |__2___|___3___|
        ----------------------------------------------
        第3次 |__3___|__WEEK_END___|

        ==================================================
        配置文件定义： 从01月01日开始，计费季有效 s_week.value != 0xFFFF

    */

    /* 周段表中，首个周段有误 */
    if(pweek_tbl->week[0].s_week.value > WEEK_END)
        return -1;

    /* 日时段索引号有误： >=4 或者 -1（0xffff WEEK_NOTSET） */
    if(pcfg->week[0].day.value >= MULTRATE_NUM_DAY_TABLE)
        return -1;

    /* 
        根据当前周几，在周段表中查找周段索引
        根据索引返回对应计费季段的周段，
     */
    /* 第0次 - 第2次 */
    for(i=0;i<(MULTRATE_NUM_WEEK-1);i++)
    {
        /* 遍历时，下一个计费季起始时间无效，即当前计费季为所属 */
        if(pweek_tbl->week[i+1].s_week.value == WEEK_NOTSET)
        {
            return i;
        }

        /*  当前md  <= now < 下一个md， 计费季找到   */
        if((pweek_tbl->week[i].s_week.value <= bcd_now_week) && (bcd_now_week < pweek_tbl->week[i+1].s_week.value))
        {
            return i;
        }
    }

    /* 第3次 */
    if((pweek_tbl->week[MULTRATE_NUM_WEEK-1].s_week.value <= bcd_now_week) && (bcd_now_week <= WEEK_END))
    {
        return MULTRATE_NUM_WEEK-1;
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
static struct db_cfg_multirate_specialday_type *multirate_specialday_find(struct db_cfg_multirate_special_type *pcfg, uint16_t bcd_now_md, int *pindex)
{
    /*
        注意：特殊日不再使用二分法查找。
        特殊日有90个， 二分法最大需查找7次， 但二分法循环中的判断和赋值计算较多，实际在只有90条的情况下，二分法效率未必有大提升
    */

     int i;

#define MD_NOTSET           0xffff
    /* 
        检索计费季原理： 从0开始查看当前时间是否所属  计费季起始月日 与下一个计费季起始月日。

        第0次 |__0___|
        第1次 |__1___|
        第2次 |__2___|
        ----------------------------------------------
        第89次|__89___|

        ==================================================
        配置文件定义： 从01月01日开始，计费季有效 s_week.value != 0xFFFF

    */

    /* 第0次 - 第2次 */
    for(i=0;i<(MULTRATE_NUM_DAY_SPEC);i++)
    {
        // if(pcfg->cfg[i].s_md.value == MD_NOTSET)
        //     return NULL;
        /* 遍历 */
        if(pcfg->cfg[i].s_md.value == bcd_now_md)
        {
            *pindex = i;
            return &pcfg->cfg[i];
        }
    }

    return NULL;

}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static struct db_cfg_multirate_unit_type *multirate_special_unit_find(struct db_cfg_multirate_specialday_type *pcfg, struct db_cfg_multirate_day_table_type *pday_tbl, uint16_t bcd_now_hm, int *pindex)
{
    int i;
    struct db_cfg_multirate_unit_group_type *punit_group;

/* 日最后时段 BCD格式  2359 */
#define DAY_END              2359
#define DAY_NOTSET           0xffff
    /* 
        检索时段原理： 从0开始查看当前时间是否所属  计费季起始月日 与下一个计费季起始月日。

        第0次 |__0___|___1___|
        第1次 |__1___|___2___|
        第2次 |__2___|___3___|
        ........
        第10次|__10__|__11___|
        ----------------------------------------------
        第11次|__11__|__MD_END___|

        ==================================================
        配置文件定义： 

    */

    /* 时段索引无效 */
    if(pcfg->day.value >= MULTRATE_NUM_DAY_TABLE)
        return NULL;

    /* 根据时段索引在时段表中找到对应的时段 */
    punit_group = &pday_tbl->table_day[pcfg->day.value];

    /* 第0次 - 第2次 */
    for(i=0;i<(MULTRATE_NUM_DAY-1);i++)
    {
        /* 遍历时，下一个计费季起始时间无效，即当前计费季为所属 */
        if(punit_group->day[i+1].s_hm.value == DAY_NOTSET)
        {
            *pindex = i;
            return &punit_group->day[i];
        }

        /*  当前md  <= now < 下一个md， 计费季找到   */
        if((punit_group->day[i].s_hm.value <= bcd_now_hm) && (bcd_now_hm < punit_group->day[i+1].s_hm.value))
        {
            *pindex = i;
            return &punit_group->day[i];
        }
    }

    /* 最后一段 */
    if((punit_group->day[MULTRATE_NUM_DAY-1].s_hm.value <= bcd_now_hm) && (bcd_now_hm <= MD_END))
    {
        *pindex = MULTRATE_NUM_DAY-1;
        return &punit_group->day[MULTRATE_NUM_DAY-1];
    }

    return NULL;
}


/*********************************************************************************************************
** Function name:       multirate_accumulate
** Descriptions:        convert sample value data to actual data
** input parameters:    u64remain   - 电度剩余值
                        real        - 
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
static int32_t multirate_accumulate(uint64_t *u64remain, uint64_t *real)
{
    uint64_t real_tmp;

    real_tmp = *real;
    if(*u64remain > 0)
    {
        real_tmp += *u64remain;
        real_tmp %= (ENERGY_MAXNUM + 1);
        *real = real_tmp;
        return 0;
    }
    else
    {
        return -1;
    }
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
// static __inline struct db_energy_multirate_type *multirate_rate_sector_get(struct db_database_type *out, uint8_t rate_no)
// {
//     switch(rate_no)
//     {
//         case 0:
//         return &out->db_multirate.kWhImp[rate_no][0];
//         case 1:
//         return &out->db_measure.energy.multirate.peak;
//         case 2:
//         return &out->db_measure.energy.multirate.flat;
//         case 3:
//         return &out->db_measure.energy.multirate.valley;
//     }

//     return NULL;
// }



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int multirate_accumulate_group(void *in, void *pNet, uint8_t rate_no)
{
    struct db_database_measure_base_type    *pMeasureBase = (struct db_database_measure_base_type *)in;
    struct db_realtime_energy_type            *pNetEnergy = (struct db_realtime_energy_type*)pNet;

    /* 共4套费率 */
    if(rate_no >= RATES_NUM)
        return -1;

    multirate_accumulate(&pNetEnergy->kWhImp[PHASE_TOT],    &pMeasureBase->db_measure.energy.multirate_hp.rate[rate_no].kWhImp);
    multirate_accumulate(&pNetEnergy->kWhExp[PHASE_TOT],    &pMeasureBase->db_measure.energy.multirate_hp.rate[rate_no].kWhExp);
    multirate_accumulate(&pNetEnergy->kvarhImp[PHASE_TOT],  &pMeasureBase->db_measure.energy.multirate_hp.rate[rate_no].kvarhImp);
    multirate_accumulate(&pNetEnergy->kvarhExp[PHASE_TOT],  &pMeasureBase->db_measure.energy.multirate_hp.rate[rate_no].kvarhExp);

    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static __inline__ struct db_cfg_multirate_day_type *multirate_general_day_find(struct db_cfg_multirate_day_type *pday)
{
    return pday;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int32_t rms_algorithm_MultirateEnergy_hp(struct db_database_measure_base_type *pMeasureBase, struct db_energy_multirate_type *out, void *pNet, uint32_t pt, uint32_t ct)
{
    int         index;
    uint16_t    s_md, s_week, s_hm;
    time_t      now;
    struct tm   tm;
    /* 多费率配置在顶层数据库结构中（全局共享） */
    struct db_database_type *pDBTop = (struct db_database_type*)rt_db_getVmem();
    struct db_cfg_user_multirate_para *pcfg = &(pDBTop->multirate);
    struct db_cfg_multirate_schema_type         *pschema;
    struct db_cfg_multirate_specialday_type     *pspecialday;
    struct db_cfg_multirate_week_type           *pweek;
    struct db_cfg_multirate_day_type            *pday;
    struct db_cfg_multirate_unit_type           *punit;
    struct db_energy_multirate_running_status_t *prunning_status = &pMeasureBase->db_measure.energy.multirate_status;

    /* 当前时间 */
    now = time(NULL);
    localtime_r(&now, &tm);

    /* 根据当前时间确定使用的费率表 */
    if(multirate_table_select(pcfg, now) == 0)
    {
        pschema = &pcfg->schema;
        prunning_status->model_index = 1; // 第一套复费率表
    }
    else 
    {
        pschema = &pcfg->schema_new;
        prunning_status->model_index = 2; // 执行第二套复费率表
    }
        

    /* 先查特殊日. 特殊日匹配，使用当前特殊日费率 */
    s_md = MULTIRATE_BCD_CONVENT((tm.tm_mon+1), tm.tm_mday);
    pspecialday = multirate_specialday_find(&pschema->special, s_md, &index);
    if(pspecialday)
    {
        prunning_status->holiday_index = index + 1;              // 当前哪个特殊日，由于存储是从0开始的，这里转换成base1
        
        s_hm = MULTIRATE_BCD_CONVENT(tm.tm_hour, tm.tm_min);
        punit = multirate_special_unit_find(pspecialday, &pschema->table_day, s_hm, &index);
        if(punit)
        {
            multirate_accumulate_group(pMeasureBase, pNet, punit->rate.value);
            prunning_status->time_index = index + 1;                    // 当前所用时段索引号
            prunning_status->rate = punit->rate.value;                  // 当前所在尖峰平谷哪个费率
            prunning_status->day_index = pspecialday->day.value + 1;    // 当前特殊日使用那个20时段表中的哪个
            prunning_status->week_index = 0;
            prunning_status->season_index = 0;
            return 0;
        }
    }

    /* 常规日， 节假日的索引固定为0 */
    prunning_status->holiday_index = 0; 
    /* 无匹配的特殊日，使用常规日 */
    s_md = MULTIRATE_BCD_CONVENT((tm.tm_mon+1), tm.tm_mday);
    /* 根据月日查找计费季表，查找所属计费季 */
    pweek = multirate_general_week_find(&pschema->general, s_md, &index);
    if(pweek)
    {
        prunning_status->season_index = index + 1;                      // 当前哪个计费季，由于存储是从0开始的，这里转换成base1
        /* tm_wday = 0 表示星期天，这里强制改为 7，后面的逻辑就不用改 */
        if(tm.tm_wday == 0) 
        {
            tm.tm_wday = 7;
        }
        
        s_week = MULTIRATE_BCD_CONVENT(0, tm.tm_wday);
        /* 从周表中查找周段（工作日/节假日）序号  */
        index = multirate_general_week_index_find(pweek, &pschema->table_week, s_week);
        if(index == -1)
            return -1;

        prunning_status->week_index = index + 1;
        pday = multirate_general_day_find(&pweek->week[index]);
        if(pday)
        {
            s_hm = MULTIRATE_BCD_CONVENT(tm.tm_hour, tm.tm_min);
            punit = multirate_general_unit_find(pday, &pschema->table_day, s_hm, &index);
            if(punit)
            {
                multirate_accumulate_group(pMeasureBase, pNet,punit->rate.value);
                prunning_status->time_index = index + 1;        // 当前所用时段索引号
                prunning_status->rate = punit->rate.value;  // 当前所在尖峰平谷哪个费率
                prunning_status->day_index = pday->day.value + 1;

                return 0;
            }
        }
    }

    return -1;
}




