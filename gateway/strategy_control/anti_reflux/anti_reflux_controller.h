/**
 * @file anti_reflux_controller.h
 * @brief 防逆流策略调控总控：负载估计+绝对目标值分配→写库，统一接口与周期调用
 */

#ifndef ANTI_REFLUX_CONTROLLER_H
#define ANTI_REFLUX_CONTROLLER_H

#include "anti_reflux_config.h"
#include "anti_reflux_model.h"
#include "algo/anti_reflux_input.h"
#include "algo/anti_reflux_safety.h"
#include "algo/anti_reflux_allocator.h"
#include "anti_reflux_safety_guard.h"

/** 测点读写回调：dblink_id → 读/写 double，返回 0 成功 */
typedef int (*anti_reflux_read_point_fn)(uint32_t dblink_id, double *val);
typedef int (*anti_reflux_write_point_fn)(uint32_t dblink_id, double *val);
/** 与 DataMgmtFops.write_command_op 一致：控制命令按值传 double（勿传指针） */
typedef int (*anti_reflux_write_command_op_fn)(uint32_t dblink_id, double val);
/** 控制点读/写回调（DO/AO），与 DataMgmtFops.readctrlpoint_by_id / writectrlpoint_by_id 一致 */
typedef int (*anti_reflux_read_ctrlpoint_fn)(uint32_t dblink_id, double *val);
typedef int (*anti_reflux_write_ctrlpoint_fn)(uint32_t dblink_id, double *val);

/** 控制器上下文：配置 + 模型 + 各算法状态 + 读写接口 */
typedef struct {
    const AntiRefluxConfig    *config;
    const AntiRefluxDeviceModel *model;
    AntiRefluxInputState      input_state;
    AntiRefluxSafetyState     safety_state;
    anti_reflux_read_point_fn read_point;
    anti_reflux_write_point_fn write_point;
    /** 继电器/Modbus 控制队列写点，与 write_command_op(nodeid, double) 一致 */
    anti_reflux_write_command_op_fn write_command_op;
    /** 动态抑制点与 writectrlpoint 同属控制点表，勿用 readpoint_by_id */
    anti_reflux_read_ctrlpoint_fn read_ctrlpoint;
    anti_reflux_write_ctrlpoint_fn write_ctrlpoint;
    /** 上一周期时间戳 ms（由外部设置或使用相对 dt） */
    uint32_t last_cycle_ms;
    /** 调试开关：非 0 时 controller_run 打印各步骤状态到 stderr */
    int debug;
    /** 安全防线（继电器）独立模块上下文 */
    AntiRefluxSafetyGuardCtx sg_ctx;
} AntiRefluxController;

/**
 * 初始化控制器（置零各算法状态，不修改 config/model）
 */
void anti_reflux_controller_init(
    AntiRefluxController     *ctrl,
    const AntiRefluxConfig   *config,
    const AntiRefluxDeviceModel *model,
    anti_reflux_read_point_fn  read_fn,
    anti_reflux_write_point_fn write_fn,
    anti_reflux_read_ctrlpoint_fn read_ctrlpoint_fn,
    anti_reflux_write_ctrlpoint_fn write_ctrlpoint_fn
);

/** 进程启动、库初始化后：对「持久参与且已绑定动态点」的虚拟点写 S=0，表示未抑制 */
void anti_reflux_dynamic_suppress_bootstrap_zero(
    const AntiRefluxDeviceModel *model,
    anti_reflux_write_ctrlpoint_fn write_ctrlpoint_fn);

/**
 * 执行一个控制周期：读表→负载估计→绝对目标值分配→写逆变器
 * @param ctrl  控制器上下文
 * @param now_ms 当前时间戳 ms（若为 0 则用 ctrl_interval_ms 作为 dt）
 * @return 0 成功，<0 错误（如未使能、模型无效）
 */
int anti_reflux_controller_run(AntiRefluxController *ctrl, uint32_t now_ms);

#endif /* ANTI_REFLUX_CONTROLLER_H */
