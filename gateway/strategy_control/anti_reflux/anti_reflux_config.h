/**
 * @file anti_reflux_config.h
 * @brief 防逆流策略 config 配置对应的 C 结构体定义
 *        依据 config/strategy/strategy_control.json 的 "config" 字段
 */

#ifndef ANTI_REFLUX_CONFIG_H
#define ANTI_REFLUX_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/* 字符串字段最大长度 */
#define ANTI_REFLUX_VERSION_LEN        16
#define ANTI_REFLUX_ALGO_MODE_LEN      32
#define ANTI_REFLUX_METER_FAIL_ACTION_LEN  24
#define ANTI_REFLUX_STRATEGY_LEN       32
#define ANTI_REFLUX_MAX_DEVICES        32
#define ANTI_REFLUX_SAFETY_GUARD_NOTES_LEN 128

/** 系统配置 (config.system) */
typedef struct {
    uint32_t    ctrl_interval_ms;   /**< 控制周期，毫秒 */
    char        algo_mode[ANTI_REFLUX_ALGO_MODE_LEN];  /**< 算法模式，如 "LOAD_BASED"（负载估计+绝对目标值） */
    bool        enable;             /**< 是否启用 */
} AntiRefluxSystemConfig;

/** 输入处理配置 (config.input_processing) */
typedef struct {
    double      filter_alpha;           /**< 滤波系数 [0,1] */
    int32_t     target_power_w;         /**< 目标功率 W：理想为 0（电表接近 0 = 不逆流且光伏尽量满足负载）；可设略正(如 20~50)作安全裕度 */
    int32_t     reverse_threshold_w;    /**< 逆流判定阈值 W（如 -200） */
    uint32_t    direction_hysteresis_ms; /**< 方向滞环时间 ms */
} AntiRefluxInputProcessingConfig;

/** 输出安全配置 (config.output_safety) */
typedef struct {
    int32_t     ramp_up_pc_per_sec;     /**< 功率上升斜率 %/s */
    int32_t     ramp_down_pc_per_sec;   /**< 功率下降斜率 %/s */
    uint32_t    meter_fail_timeout_s;   /**< 电表故障超时 s */
    char        meter_fail_action[ANTI_REFLUX_METER_FAIL_ACTION_LEN]; /**< 电表故障动作，如 "SHUTDOWN" */
    int32_t     max_total_limit_pc;     /**< 总功率上限 % */
} AntiRefluxOutputSafetyConfig;

/** 逆变器组内设备引用 (config.inverter_groups.devices[] 元素) */
typedef struct {
    int32_t     ChnlId;     /**< 通道号 */
    int32_t     DevIndex;   /**< 设备号（与 chnl_N.json 中 logic_id 匹配） */
    bool        IsEmpty;    /**< 是否为空槽位 */
} AntiRefluxDeviceRef;

/** 逆变器组配置 (config.inverter_groups) */
typedef struct {
    char                    strategy[ANTI_REFLUX_STRATEGY_LEN];  /**< 组内策略，如 "CAPACITY_WEIGHT" */
    int32_t                 min_running_pc;   /**< 最小运行百分比 */
    AntiRefluxDeviceRef     devices[ANTI_REFLUX_MAX_DEVICES];    /**< 设备列表 */
    uint32_t                device_count;     /**< 实际设备数量 */
} AntiRefluxInverterGroupsConfig;

/** 安全防线配置 (config.safety_guard) */
typedef struct {
    bool        enable;
    uint32_t    comm_fail_cycles_light;
    uint32_t    comm_fail_cycles_hard;
    int32_t     limit_mismatch_w;
    uint32_t    limit_mismatch_cycles;
    uint32_t    effect_timeout_s;
    uint32_t    reverse_risk_confirm_s;
    uint32_t    trip_confirm_s;
    uint32_t    trip_hold_min_s;
    uint32_t    reclose_cooldown_s;
    uint32_t    max_trips_per_hour;
    bool        manual_reclose_only;
    uint32_t    relay_command_interval_ms;
    char        notes[ANTI_REFLUX_SAFETY_GUARD_NOTES_LEN];
} AntiRefluxSafetyGuardConfig;

/** 防逆流策略完整 config 配置 (对应 JSON config 根) */
typedef struct {
    char                            version[ANTI_REFLUX_VERSION_LEN];  /**< 配置版本，如 "2.1.0" */
    AntiRefluxSystemConfig          system;
    AntiRefluxInputProcessingConfig input_processing;
    AntiRefluxOutputSafetyConfig    output_safety;
    AntiRefluxInverterGroupsConfig  inverter_groups;
    AntiRefluxSafetyGuardConfig     safety_guard;
} AntiRefluxConfig;

#endif /* ANTI_REFLUX_CONFIG_H */
