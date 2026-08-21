/**
 * @file anti_reflux_input.h
 * @brief 防逆流输入处理：电表功率滤波、逆流判定与方向滞环
 */

#ifndef ANTI_REFLUX_INPUT_H
#define ANTI_REFLUX_INPUT_H

#include <stdbool.h>
#include <stdint.h>

/** 方向：正向（用电/光伏向电网送电） / 逆流 */
typedef enum {
    ANTI_REFLUX_DIR_FORWARD = 0,
    ANTI_REFLUX_DIR_REVERSE = 1
} AntiRefluxDirection;

/** 输入处理运行时状态（不依赖 config，由上层按 ctrl_interval 驱动） */
typedef struct {
    double      filtered_power_w;     /**< 一阶滤波后的总有功 W */
    int         direction;            /**< 当前方向 ANTI_REFLUX_DIR_* */
    uint32_t    hysteresis_ticks_ms;  /**< 方向滞环剩余 ms（切换后保持） */
} AntiRefluxInputState;

/**
 * 初始化输入处理状态（上电或重载配置后调用）
 */
void anti_reflux_input_init(AntiRefluxInputState *state);

/**
 * 单步输入处理：滤波 + 逆流判定 + 方向滞环
 * @param state     运行时状态（输入/输出）
 * @param raw_power_w  本周期读取的电表总有功 W
 * @param filter_alpha 滤波系数 [0,1]，越大跟踪越快
 * @param reverse_threshold_w  逆流判定阈值 W（如 -200 表示 < -200 为逆流）
 * @param direction_hysteresis_ms  方向切换滞环时间 ms
 * @param dt_ms     本周期时长 ms
 */
void anti_reflux_input_step(
    AntiRefluxInputState    *state,
    double                   raw_power_w,
    double                   filter_alpha,
    int32_t                  reverse_threshold_w,
    uint32_t                 direction_hysteresis_ms,
    uint32_t                 dt_ms
);

/** 当前是否为逆流（供外部逻辑或日志） */
static inline bool anti_reflux_input_is_reverse(const AntiRefluxInputState *state)
{
    return state->direction == ANTI_REFLUX_DIR_REVERSE;
}

#endif /* ANTI_REFLUX_INPUT_H */
