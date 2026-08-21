/**
 * @file anti_reflux_allocator.h
 * @brief 防逆流逆变器功率分配：按容量权重等策略分配总限制到各逆变器
 */

#ifndef ANTI_REFLUX_ALLOCATOR_H
#define ANTI_REFLUX_ALLOCATOR_H

#include <stdint.h>

/** 分配策略名称（与 config 中 strategy 字段一致） */
#define ANTI_REFLUX_STRATEGY_CAPACITY_WEIGHT  "CAPACITY_WEIGHT"

/**
 * 按配置策略将总限制百分比分配到各逆变器
 *
 * @param strategy        如 ANTI_REFLUX_STRATEGY_CAPACITY_WEIGHT
 * @param min_running_pc  单机最小运行百分比，低于此值则分配 0（不投运）
 * @param total_limit_pc  总功率限制 % [0, 100]
 * @param rated_w        各逆变器额定功率 W，可为 0 表示未配置（支持微型逆变器）
 * @param inv_count      逆变器数量
 * @param limit_pc_out   输出：各逆变器分配到的限制 %，长度至少 inv_count
 */
void anti_reflux_allocator_run(
    const char *strategy,
    int32_t     min_running_pc,
    double      total_limit_pc,
    const double *rated_w,
    uint32_t    inv_count,
    double     *limit_pc_out
);

#endif /* ANTI_REFLUX_ALLOCATOR_H */
