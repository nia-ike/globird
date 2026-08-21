/**
 * @file anti_reflux_allocator.c
 * @brief 防逆流功率分配实现：CAPACITY_WEIGHT 按额定功率比例分配，min_running_pc 门槛
 */

#include "anti_reflux_allocator.h"
#include <string.h>
#include <math.h>

/*********************************************************************************************************
** Function name:       anti_reflux_allocator_run
** Descriptions:        按策略将总限制百分比分配到各逆变器
** input parameters:    strategy, min_running_pc, total_limit_pc, rated_w, inv_count
** output parameters:   limit_pc_out - 各机限制 %
** Returned value:      NONE
*********************************************************************************************************/
void anti_reflux_allocator_run(
    const char *strategy,
    int32_t     min_running_pc,
    double     total_limit_pc,
    const double *rated_w,
    uint32_t    inv_count,
    double     *limit_pc_out)
{
    double sum_w = 0.0;
    uint32_t i;

    if (!limit_pc_out || !rated_w || inv_count == 0)
        return;

    memset(limit_pc_out, 0, inv_count * sizeof(double));
    if (total_limit_pc <= 0.0)
        return;

    if (strategy && strcmp(strategy, ANTI_REFLUX_STRATEGY_CAPACITY_WEIGHT) == 0)
    {
        for (i = 0; i < inv_count; i++)
            sum_w += (rated_w[i] > 0.0) ? rated_w[i] : 0.0;

        if (sum_w < 1e-6)
        {
            /* 无有效容量时平均分或全 0，此处给 0 */
            return;
        }

        for (i = 0; i < inv_count; i++)
        {
            double share;
            if (rated_w[i] <= 0.0)
            {
                limit_pc_out[i] = 0.0;
                continue;
            }
            share = total_limit_pc * (rated_w[i] / sum_w);
            if (min_running_pc > 0 && share > 0.0 && share < (double)min_running_pc)
                share = 0.0;  /* 低于最小运行百分比则不投运 */
            if (share > 100.0)
                share = 100.0;
            limit_pc_out[i] = share;
        }
    }
    else
    {
        /* 未知策略：平均分配或按容量（若仅有容量信息） */
        for (i = 0; i < inv_count; i++)
            sum_w += (rated_w[i] > 0.0) ? rated_w[i] : 0.0;
        if (sum_w < 1e-6)
            sum_w = (double)inv_count;
        for (i = 0; i < inv_count; i++)
        {
            double share = total_limit_pc * ((rated_w[i] > 0.0 ? rated_w[i] : 1.0) / sum_w);
            if (min_running_pc > 0 && share > 0.0 && share < (double)min_running_pc)
                share = 0.0;
            if (share > 100.0)
                share = 100.0;
            limit_pc_out[i] = share;
        }
    }
}
