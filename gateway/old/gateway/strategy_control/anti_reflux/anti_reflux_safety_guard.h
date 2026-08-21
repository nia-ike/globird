/**
 * @file anti_reflux_safety_guard.h
 * @brief 防逆流安全防线（继电器可选附件）独立模块接口
 */

#ifndef ANTI_REFLUX_SAFETY_GUARD_H
#define ANTI_REFLUX_SAFETY_GUARD_H

#include <stdint.h>
#include "anti_reflux_config.h"
#include "anti_reflux_model.h"

typedef enum {
    AR_SG_DISABLED = 0,
    AR_SG_MONITOR,
    AR_SG_SUSPECT,
    AR_SG_TRIP_PENDING,
    AR_SG_TRIPPED,
    AR_SG_RECLOSE_COOLDOWN
} AntiRefluxSgState;

typedef struct {
    AntiRefluxSgState state;
    uint32_t comm_fail_cycles;
    uint32_t mismatch_cycles;
    uint32_t trip_pending_ms;
    uint32_t trip_hold_ms;
    uint32_t cooldown_ms;
    uint32_t last_relay_cmd_ms;
    uint32_t trip_count_hour;
    uint32_t trip_hour_epoch_s;
    uint8_t  allow_coord;
    uint8_t  trip_cmd_pending;
    uint8_t  close_cmd_pending;
    uint8_t  waiting_trip_feedback;
    uint8_t  waiting_close_feedback;
    uint8_t  relay_seq;       /**< 0=空闲；1=分闸序列；2=合闸序列 */
    uint8_t  relay_seq_step;  /**< 0=释放 S1/S2；1=吸合目标线圈 */
    uint32_t feedback_wait_ms;
    double   last_limit_cmd_w;
} AntiRefluxSgInvRuntime;

typedef struct {
    uint32_t inverter_count;
    AntiRefluxSgInvRuntime inv[ANTI_REFLUX_MAX_INVERTERS];
} AntiRefluxSafetyGuardCtx;

typedef struct {
    uint8_t inv_comm_ok;
    double  inv_active_w;
    double  inv_limit_readback_w;
    double  inv_limit_cmd_w;
    uint8_t relay_state_valid;
    double  relay_state_value;
    uint8_t reverse_direction;
} AntiRefluxSgInvInput;

typedef int (*anti_reflux_sg_write_ctrl_fn)(uint32_t dblink_id, double val);

void anti_reflux_sg_init(AntiRefluxSafetyGuardCtx *ctx,
                         const AntiRefluxDeviceModel *model);

int anti_reflux_sg_step(AntiRefluxSafetyGuardCtx *ctx,
                        const AntiRefluxConfig *cfg,
                        const AntiRefluxDeviceModel *model,
                        const AntiRefluxSgInvInput *inv_inputs,
                        uint32_t now_ms,
                        uint32_t dt_ms,
                        int debug);

uint8_t anti_reflux_sg_allow_coord(const AntiRefluxSafetyGuardCtx *ctx, uint32_t inv_idx);

int anti_reflux_sg_flush_commands(AntiRefluxSafetyGuardCtx *ctx,
                                  const AntiRefluxConfig *cfg,
                                  const AntiRefluxDeviceModel *model,
                                  anti_reflux_sg_write_ctrl_fn write_ctrl_fn,
                                  uint32_t now_ms,
                                  int debug);

const char *anti_reflux_sg_state_name(AntiRefluxSgState s);

const char *anti_reflux_sg_relay_seq_name(uint8_t relay_seq);

/** -o 测试：走与运行态相同的 flush_commands 继电器下发路径 */
typedef enum {
    AR_SG_TEST_RELAY_TRIP = 0,
    AR_SG_TEST_RELAY_CLOSE,
    AR_SG_TEST_RELAY_BOTH
} AntiRefluxSgTestRelayCmd;

int anti_reflux_sg_test_relay_output(AntiRefluxSafetyGuardCtx *ctx,
                                     AntiRefluxConfig *cfg,
                                     const AntiRefluxDeviceModel *model,
                                     anti_reflux_sg_write_ctrl_fn write_ctrl_fn,
                                     uint32_t inv_idx,
                                     AntiRefluxSgTestRelayCmd cmd,
                                     uint32_t now_ms);

#endif

