/**
 * @file anti_reflux_safety.h
 * @brief 防逆流安全层：功率变化率限制（爬坡）、电表故障处理
 */

#ifndef ANTI_REFLUX_SAFETY_H
#define ANTI_REFLUX_SAFETY_H

#include <stdbool.h>
#include <stdint.h>

/** 安全层运行时状态 */
typedef struct {
    double current_limit_pc;   /**< 当前允许的功率限制 %（爬坡后） */
    double meter_invalid_sec;  /**< 电表连续无效累计时间 s */
} AntiRefluxSafetyState;

/**
 * 初始化安全状态
 */
void anti_reflux_safety_init(AntiRefluxSafetyState *state);

/**
 * 单步安全处理：爬坡限制 + 电表故障动作
 * @param state              运行时状态
 * @param desired_limit_pc   PID 输出的期望限制 %
 * @param apply_ramp         true=启用爬坡限制（仅在不逆流时）；false=逆流时立即跟随目标，不爬坡
 * @param ramp_up_pc_per_sec 上升斜率 %/s（仅当 apply_ramp 时有效）
 * @param ramp_down_pc_per_sec 下降斜率 %/s（仅当 apply_ramp 时有效）
 * @param meter_valid        本周期电表数据是否有效
 * @param meter_fail_timeout_s  电表无效多少秒触发故障动作
 * @param meter_fail_action  如 "SHUTDOWN" 表示故障时输出 0
 * @param max_total_limit_pc 总限制上限 %
 * @param dt_s               本周期时长 s
 * @return                   安全后的限制 % [0, max_total_limit_pc]
 */
double anti_reflux_safety_step(
    AntiRefluxSafetyState *state,
    double                 desired_limit_pc,
    bool                   apply_ramp,
    int32_t                ramp_up_pc_per_sec,
    int32_t                ramp_down_pc_per_sec,
    bool                   meter_valid,
    uint32_t               meter_fail_timeout_s,
    const char            *meter_fail_action,
    int32_t                max_total_limit_pc,
    double                 dt_s
);

#endif /* ANTI_REFLUX_SAFETY_H */
