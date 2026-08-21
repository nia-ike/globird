/**
 * @file anti_reflux_safety.c
 * @brief 防逆流安全层实现：爬坡、电表故障 SHUTDOWN
 */

#include "anti_reflux_safety.h"
#include <string.h>
#include <math.h>

#define ANTI_REFLUX_ACTION_SHUTDOWN   "SHUTDOWN"

/*********************************************************************************************************
** Function name:       anti_reflux_safety_init
** Descriptions:        初始化安全层状态（当前限制、电表无效计时）
** input parameters:    state - 运行时状态
** output parameters:   state - 置零
** Returned value:      NONE
*********************************************************************************************************/
void anti_reflux_safety_init(AntiRefluxSafetyState *state)
{
    if (!state)
        return;
    memset(state, 0, sizeof(*state));
}

/*********************************************************************************************************
** Function name:       anti_reflux_safety_step
** Descriptions:        单步安全处理：爬坡限制 + 电表故障动作
** input parameters:    state, desired_limit_pc, apply_ramp, ramp_*, meter_*, max_total_limit_pc, dt_s
** output parameters:   state - 更新当前限制与电表无效计时
** Returned value:      安全后的限制 % [0, max_total_limit_pc]
*********************************************************************************************************/
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
    double                 dt_s)
{
    double max_delta;
    double out;

    if (!state)
        return 0.0;

    if (!meter_valid)
        state->meter_invalid_sec += dt_s;
    else
        state->meter_invalid_sec = 0.0;

    if (meter_fail_timeout_s > 0 && state->meter_invalid_sec >= (double)meter_fail_timeout_s)
    {
        if (meter_fail_action && strcmp(meter_fail_action, ANTI_REFLUX_ACTION_SHUTDOWN) == 0)
        {
            state->current_limit_pc = 0.0;
            return 0.0;
        }
    }

    if (max_total_limit_pc <= 0)
        max_total_limit_pc = 100;
    if (desired_limit_pc > (double)max_total_limit_pc)
        desired_limit_pc = (double)max_total_limit_pc;
    if (desired_limit_pc < 0.0)
        desired_limit_pc = 0.0;

    /* 逆流时不爬坡：立即分配并尽可能快执行 */
    if (!apply_ramp)
    {
        state->current_limit_pc = desired_limit_pc;
        return desired_limit_pc;
    }

    /* 不逆流时：爬坡限制 */
    if (desired_limit_pc >= state->current_limit_pc)
        max_delta = (double)ramp_up_pc_per_sec * dt_s;
    else
        max_delta = (double)ramp_down_pc_per_sec * dt_s;

    if (max_delta < 0.0)
        max_delta = -max_delta;
    if (fabs(desired_limit_pc - state->current_limit_pc) <= max_delta)
        out = desired_limit_pc;
    else if (desired_limit_pc > state->current_limit_pc)
        out = state->current_limit_pc + max_delta;
    else
        out = state->current_limit_pc - max_delta;

    if (out < 0.0)
        out = 0.0;
    if (out > (double)max_total_limit_pc)
        out = (double)max_total_limit_pc;

    state->current_limit_pc = out;
    return out;
}
