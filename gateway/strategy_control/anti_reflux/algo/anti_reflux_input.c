/**
 * @file anti_reflux_input.c
 * @brief 防逆流输入处理实现：一阶滤波、逆流判定、方向滞环
 */

#include "anti_reflux_input.h"
#include <string.h>

/*********************************************************************************************************
** Function name:       anti_reflux_input_init
** Descriptions:        初始化输入处理状态（滤波、方向、滞环）
** input parameters:    state - 运行时状态
** output parameters:   state - 置零
** Returned value:      NONE
*********************************************************************************************************/
void anti_reflux_input_init(AntiRefluxInputState *state)
{
    if (!state)
        return;
    memset(state, 0, sizeof(*state));
}

/*********************************************************************************************************
** Function name:       anti_reflux_input_step
** Descriptions:        单步输入处理：一阶滤波 + 逆流判定 + 方向滞环
** input parameters:    state, raw_power_w, filter_alpha, reverse_threshold_w, direction_hysteresis_ms, dt_ms
** output parameters:   state - 更新 filtered_power_w、direction
** Returned value:      NONE
*********************************************************************************************************/
void anti_reflux_input_step(
    AntiRefluxInputState *state,
    double                raw_power_w,
    double                filter_alpha,
    int32_t               reverse_threshold_w,
    uint32_t              direction_hysteresis_ms,
    uint32_t              dt_ms)
{
    int  next_dir;
    bool in_reverse_zone;

    if (!state)
        return;

    /* 一阶低通： filtered = alpha * raw + (1-alpha) * filtered */
    if (state->filtered_power_w == 0.0 && state->hysteresis_ticks_ms == 0)
        state->filtered_power_w = raw_power_w;
    else
        state->filtered_power_w = filter_alpha * raw_power_w
            + (1.0 - filter_alpha) * state->filtered_power_w;

    in_reverse_zone = (state->filtered_power_w < (double)reverse_threshold_w);
    next_dir = in_reverse_zone ? ANTI_REFLUX_DIR_REVERSE : ANTI_REFLUX_DIR_FORWARD;

    if (next_dir != state->direction)
    {
        if (state->hysteresis_ticks_ms == 0)
            state->hysteresis_ticks_ms = direction_hysteresis_ms;
    }

    if (state->hysteresis_ticks_ms > 0)
    {
        if (dt_ms >= state->hysteresis_ticks_ms)
            state->hysteresis_ticks_ms = 0;
        else
            state->hysteresis_ticks_ms -= dt_ms;
        /* 滞环倒计时结束后再切换方向，避免下一周期再次触发计时 */
        if (state->hysteresis_ticks_ms == 0)
            state->direction = next_dir;
    }
    else
        state->direction = next_dir;
}
