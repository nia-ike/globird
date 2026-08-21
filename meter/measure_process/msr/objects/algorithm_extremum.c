/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_extremum.c
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
#include <time.h>
#include "db_struct.h"
#include "algorithm_extremum.h"

#define EXTREME_INITWAIT_TIME       (5)


#define OFFSET_OF_VMEM(x)                   (offsetof(struct db_database_measure_base_type, x))
#define EXTREME_OFFSET(x, y)                (OFFSET_OF_VMEM(x), OFFSET_OF_VMEM(y))


static int u32extreme_compare(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset);
static int s32extreme_compare(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset);
static int u64extreme_compare(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset);
static int s64extreme_compare(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset);
static int u32extreme_clear(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset);
static int s32extreme_clear(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset);
static int u64extreme_clear(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset);
static int s64extreme_clear(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset);

static struct extreme_offset_list_type extreme_offset_list[] = 
{
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Va  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Va  ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Vb  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Vb  ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Vc  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Vc  ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Vab ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Vab ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Vbc ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Vbc ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Vca ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Vca ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Ia  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Ia  ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Ib  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Ib  ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Ic  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Ic  ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Pa  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Pa  ), S32EXTREME, s32extreme_compare, s32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Pb  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Pb  ), S32EXTREME, s32extreme_compare, s32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Pc  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Pc  ), S32EXTREME, s32extreme_compare, s32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Ptot), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Ptot), S32EXTREME, s32extreme_compare, s32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Qa  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Qa  ), S32EXTREME, s32extreme_compare, s32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Qb  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Qb  ), S32EXTREME, s32extreme_compare, s32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Qc  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Qc  ), S32EXTREME, s32extreme_compare, s32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Qtot), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Qtot), S32EXTREME, s32extreme_compare, s32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Sa  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Sa  ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Sb  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Sb  ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Sc  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Sc  ), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Stot), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Stot), U32EXTREME, u32extreme_compare, u32extreme_clear },
    { OFFSET_OF_VMEM(db_measure.numeric.fullwave.primary.Hz  ), OFFSET_OF_VMEM(db_measure.statistical.extremum.day.Hz  ), U16EXTREME, u32extreme_compare, u32extreme_clear },
};


/*********************************************************************************************************
** Function name:       u32extreme_compare
** Descriptions:        
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
static int u32extreme_compare(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset)
{
    uint32_t u32realtime_val = 0, u32max_val = 0, u32min_val = 0;
    time_t unixtime = time(NULL);

    uint32_t *p_u32realtime_val = (uint32_t *)(pstVmem + p_extreme_offset->realtime_offset);
    struct db_extremum_group_u32_type *p_u32extreme = (struct db_extremum_group_u32_type *)(pstVmem + p_extreme_offset->extreme_offset);

    u32realtime_val = *p_u32realtime_val;
    u32max_val = p_u32extreme->max.value;
    u32min_val = p_u32extreme->min.value;

    if(u32realtime_val > u32max_val)
    {
        p_u32extreme->max.value = u32realtime_val;
        p_u32extreme->max.time  = unixtime;
    }

    if(u32realtime_val < u32min_val)
    {
        p_u32extreme->min.value = u32realtime_val;
        p_u32extreme->min.time  = unixtime;
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       u32extreme_compare
** Descriptions:        
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
static int s32extreme_compare(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset)
{
    int32_t s32realtime_val = 0, s32max_val = 0, s32min_val = 0;
    time_t unixtime = time(NULL);

    int32_t *p_s32realtime_val = (int32_t *)(pstVmem + p_extreme_offset->realtime_offset);
    struct db_extremum_group_s32_type *p_s32extreme = (struct db_extremum_group_s32_type *)(pstVmem + p_extreme_offset->extreme_offset);

    s32realtime_val = *p_s32realtime_val;
    s32max_val = p_s32extreme->max.value;
    s32min_val = p_s32extreme->min.value;

    if(labs(s32realtime_val) > labs(s32max_val))
    {
        p_s32extreme->max.value = s32realtime_val;
        p_s32extreme->max.time  = unixtime;
    }

    if(labs(s32realtime_val) < labs(s32min_val))
    {
        p_s32extreme->min.value = s32realtime_val;
        p_s32extreme->min.time  = unixtime;
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       u64extreme_compare
** Descriptions:        
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
static int u64extreme_compare(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset)
{
    uint64_t u64realtime_val = 0, u64max_val = 0, u64min_val = 0;
    time_t unixtime = time(NULL);

    uint64_t *p_u64realtime_val = (uint64_t *)(pstVmem + p_extreme_offset->realtime_offset);
    struct db_extremum_group_u64_type *p_u64extreme = (struct db_extremum_group_u64_type *)(pstVmem + p_extreme_offset->extreme_offset);

    u64realtime_val = *p_u64realtime_val;
    u64max_val = p_u64extreme->max.value;
    u64min_val = p_u64extreme->min.value;

    if(u64realtime_val > u64max_val)
    {
        p_u64extreme->max.value = u64realtime_val;
        p_u64extreme->max.time  = unixtime;
    }

    if(u64realtime_val < u64min_val)
    {
        p_u64extreme->min.value = u64realtime_val;
        p_u64extreme->min.time  = unixtime;
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       s64extreme_compare
** Descriptions:        
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
static int s64extreme_compare(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset)
{
    int64_t s64realtime_val = 0, s64max_val = 0, s64min_val = 0;
    time_t unixtime = time(NULL);

    int64_t *p_s64realtime_val = (int64_t *)(pstVmem + p_extreme_offset->realtime_offset);
    struct db_extremum_group_s64_type *p_s64extreme = (struct db_extremum_group_s64_type *)(pstVmem + p_extreme_offset->extreme_offset);

    s64realtime_val = *p_s64realtime_val;
    s64max_val = p_s64extreme->max.value;
    s64min_val = p_s64extreme->min.value;

    if(llabs(s64realtime_val) > llabs(s64max_val))
    {
        p_s64extreme->max.value = s64realtime_val;
        p_s64extreme->max.time  = unixtime;
    }

    if(llabs(s64realtime_val) < llabs(s64min_val))
    {
        p_s64extreme->min.value = s64realtime_val;
        p_s64extreme->min.time  = unixtime;
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       u16extreme_compare
** Descriptions:        
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
static int extreme_compare(uint8_t *pstVmem)
{
    uint8_t i = 0;

    for (i = 0; i < sizeof(extreme_offset_list) / sizeof(extreme_offset_list[0]); i++)
    {
        if(NULL != extreme_offset_list[i].extreme_compare)
        {
            extreme_offset_list[i].extreme_compare(pstVmem, &extreme_offset_list[i]);
        }
    }

    return 0;
}




/*********************************************************************************************************
** Function name:       u32extreme_clear
** Descriptions:        
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
static int u32extreme_clear(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset)
{
    time_t unixtime = time(NULL);
    uint32_t *p_u32realtime_val = (uint32_t *)(pstVmem + p_extreme_offset->realtime_offset);
    struct db_extremum_group_u32_type *p_u32extreme = (struct db_extremum_group_u32_type *)(pstVmem + p_extreme_offset->extreme_offset);

    uint32_t u32realtime_val = *p_u32realtime_val;

    p_u32extreme->max.value = u32realtime_val;
    p_u32extreme->min.value = u32realtime_val;
    p_u32extreme->max.time  = unixtime;
    p_u32extreme->min.time  = unixtime;

    return 0;
}

/*********************************************************************************************************
** Function name:       s32extreme_clear
** Descriptions:        
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
static int s32extreme_clear(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset)
{
    time_t unixtime = time(NULL);
    int32_t *p_s32realtime_val = (int32_t *)(pstVmem + p_extreme_offset->realtime_offset);
    struct db_extremum_group_s32_type *p_s32extreme = (struct db_extremum_group_s32_type *)(pstVmem + p_extreme_offset->extreme_offset);

    int32_t s32realtime_val = *p_s32realtime_val;

    p_s32extreme->max.value = s32realtime_val;
    p_s32extreme->min.value = s32realtime_val;
    p_s32extreme->max.time  = unixtime;
    p_s32extreme->min.time  = unixtime;

    return 0;
}

/*********************************************************************************************************
** Function name:       u64extreme_clear
** Descriptions:        
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
static int u64extreme_clear(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset)
{
    time_t unixtime = time(NULL);
    uint64_t *p_u64realtime_val = (uint64_t *)(pstVmem + p_extreme_offset->realtime_offset);
    struct db_extremum_group_u64_type *p_u64extreme = (struct db_extremum_group_u64_type *)(pstVmem + p_extreme_offset->extreme_offset);

    uint64_t u64realtime_val = *p_u64realtime_val;

    p_u64extreme->max.value = u64realtime_val;
    p_u64extreme->min.value = u64realtime_val;
    p_u64extreme->max.time  = unixtime;
    p_u64extreme->min.time  = unixtime;

    return 0;
}

/*********************************************************************************************************
** Function name:       s64extreme_clear
** Descriptions:        
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
static int s64extreme_clear(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset)
{
    time_t unixtime = time(NULL);
    int64_t *p_s64realtime_val = (int64_t *)(pstVmem + p_extreme_offset->realtime_offset);
    struct db_extremum_group_s64_type *p_s64extreme = (struct db_extremum_group_s64_type *)(pstVmem + p_extreme_offset->extreme_offset);

    int64_t s64realtime_val = *p_s64realtime_val;

    p_s64extreme->max.value = s64realtime_val;
    p_s64extreme->min.value = s64realtime_val;
    p_s64extreme->max.time  = unixtime;
    p_s64extreme->min.time  = unixtime;

    return 0;
}

/*********************************************************************************************************
** Function name:       extreme_clear
** Descriptions:        
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
int extreme_clear(uint8_t *pstVmem)
{
    uint8_t i = 0;

    for (i = 0; i < sizeof(extreme_offset_list) / sizeof(struct extreme_offset_list_type); i++)
    {
        if(NULL != extreme_offset_list[i].extreme_clear)
        {
            extreme_offset_list[i].extreme_clear(pstVmem, &extreme_offset_list[i]);
        }
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       extreme_clear_check
** Descriptions:        检查是否需要清零
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
static int32_t extreme_clear_check(void)
{
    static time_t   last;
    time_t now = time(NULL); 

    struct tm CurTime, LastTime;
    localtime_r((time_t *)&now, &CurTime); 

    if((0 == CurTime.tm_hour) && (0 == CurTime.tm_min))
    {
        localtime_r((time_t *)&last, &LastTime);

        if(((LastTime.tm_hour != 0) || (LastTime.tm_min != 0)))
        {
            last = now;
            return 1;
        }
    }

    last = now;

    return 0;  
}

/*********************************************************************************************************
** Function name:       rms_algorithm_extremum
** Descriptions:        
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/
int32_t rms_algorithm_extremum(struct db_database_measure_base_type *pMeasureBase, struct db_statistical_extremum_type *out, uint32_t pt, uint32_t ct)
{
    static uint8_t init_count = 0, extreme_init_flag = 0;

    if((0 == extreme_init_flag) && (init_count < EXTREME_INITWAIT_TIME))
    {
        init_count++;
        return 0;
    }

    if((0 == extreme_init_flag) || (1 == extreme_clear_check()))
    {
        extreme_init_flag = 1;
        return extreme_clear((uint8_t *)pMeasureBase);
    }

    return extreme_compare((uint8_t *)pMeasureBase);
}


