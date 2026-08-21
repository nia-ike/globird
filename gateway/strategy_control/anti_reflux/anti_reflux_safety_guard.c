#include "anti_reflux_safety_guard.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* 继电器状态极性：1=state<=0.5 表示分闸；0=state>0.5 表示合闸 */
#ifndef AR_SG_RELAY_OPEN_IS_ZERO
#define AR_SG_RELAY_OPEN_IS_ZERO 1
#endif

#define AR_SG_RELAY_SEQ_NONE  0U
#define AR_SG_RELAY_SEQ_TRIP  1U
#define AR_SG_RELAY_SEQ_CLOSE 2U

/*********************************************************************************************************
** Function name:       ms_to_s
** Descriptions:        毫秒转秒
** input parameters:    ms - 毫秒
** output parameters:   NONE
** Returned value:      秒数
*********************************************************************************************************/
static uint32_t ms_to_s(uint32_t ms)
{
    return ms / 1000U;
}

/*********************************************************************************************************
** Function name:       relay_is_open
** Descriptions:        判断继电器反馈是否为分闸态
** input parameters:    in - 安全防线单逆变器输入
** output parameters:   NONE
** Returned value:      1-分闸；0-非分闸或无效
*********************************************************************************************************/
static int relay_is_open(const AntiRefluxSgInvInput *in)
{
    if (!in || !in->relay_state_valid)
        return 0;
#if AR_SG_RELAY_OPEN_IS_ZERO
    return (in->relay_state_value <= 0.5) ? 1 : 0;
#else
    return (in->relay_state_value > 0.5) ? 1 : 0;
#endif
}

/*********************************************************************************************************
** Function name:       relay_is_closed
** Descriptions:        判断继电器反馈是否为合闸态
** input parameters:    in - 安全防线单逆变器输入
** output parameters:   NONE
** Returned value:      1-合闸；0-非合闸或无效
*********************************************************************************************************/
static int relay_is_closed(const AntiRefluxSgInvInput *in)
{
    if (!in || !in->relay_state_valid)
        return 0;
    return relay_is_open(in) ? 0 : 1;
}

const char *anti_reflux_sg_relay_seq_name(uint8_t seq)
{
    switch (seq)
    {
        case AR_SG_RELAY_SEQ_TRIP:  return "TRIP";
        case AR_SG_RELAY_SEQ_CLOSE: return "CLOSE";
        default:                    return "NONE";
    }
}

static void sg_set_trip_cmd_pending(AntiRefluxSgInvRuntime *st, int debug, uint32_t inv_idx,
                                    const char *reason)
{
    if (!st)
        return;
    if (!st->trip_cmd_pending && debug)
    {
        fprintf(stderr, "[anti_reflux][sg] inv[%u] queue TRIP relay cmd (%s)\n",
                (unsigned)inv_idx, reason ? reason : "");
    }
    st->trip_cmd_pending = 1U;
}

static void sg_set_close_cmd_pending(AntiRefluxSgInvRuntime *st, int debug, uint32_t inv_idx,
                                     const char *reason)
{
    if (!st)
        return;
    if (!st->close_cmd_pending && debug)
    {
        fprintf(stderr, "[anti_reflux][sg] inv[%u] queue CLOSE relay cmd (%s)\n",
                (unsigned)inv_idx, reason ? reason : "");
    }
    st->close_cmd_pending = 1U;
}

/*********************************************************************************************************
** Function name:       anti_reflux_sg_state_name
** Descriptions:        安全防线状态枚举转字符串
** input parameters:    s - 状态枚举
** output parameters:   NONE
** Returned value:      状态名称字符串
*********************************************************************************************************/
const char *anti_reflux_sg_state_name(AntiRefluxSgState s)
{
    switch (s)
    {
        case AR_SG_DISABLED: return "DISABLED";
        case AR_SG_MONITOR: return "MONITOR";
        case AR_SG_SUSPECT: return "SUSPECT";
        case AR_SG_TRIP_PENDING: return "TRIP_PENDING";
        case AR_SG_TRIPPED: return "TRIPPED";
        case AR_SG_RECLOSE_COOLDOWN: return "RECLOSE_COOLDOWN";
        default: return "UNKNOWN";
    }
}

/*********************************************************************************************************
** Function name:       anti_reflux_sg_init
** Descriptions:        初始化安全防线运行时上下文
** input parameters:    ctx - 运行时上下文；model - 设备模型
** output parameters:   ctx - 初始化后的上下文
** Returned value:      NONE
*********************************************************************************************************/
void anti_reflux_sg_init(AntiRefluxSafetyGuardCtx *ctx,
                         const AntiRefluxDeviceModel *model)
{
    uint32_t i;

    if (!ctx)
        return;
    memset(ctx, 0, sizeof(*ctx));
    if (!model)
        return;
    ctx->inverter_count = model->inverter_count;
    for (i = 0; i < ctx->inverter_count && i < ANTI_REFLUX_MAX_INVERTERS; i++)
    {
        ctx->inv[i].state = AR_SG_DISABLED;
        ctx->inv[i].allow_coord = 1;
    }
}

/*********************************************************************************************************
** Function name:       inv_sg_enabled
** Descriptions:        判断指定逆变器是否启用安全防线
** input parameters:    cfg - 配置；inv - 逆变器模型
** output parameters:   NONE
** Returned value:      1-启用；0-未启用
*********************************************************************************************************/
static int inv_sg_enabled(const AntiRefluxConfig *cfg, const AntiRefluxInverterModel *inv)
{
    if (!cfg || !inv)
        return 0;
    if (!cfg->safety_guard.enable)
        return 0;
    if (!inv->persist_enabled)
        return 0;
    if (!inv->safety_relay_present || !inv->safety_relay_enabled)
        return 0;
    if (inv->safety_trip_dblink_id == ANTI_REFLUX_DBLINK_ID_INVALID ||
        inv->safety_close_dblink_id == ANTI_REFLUX_DBLINK_ID_INVALID ||
        inv->safety_state_dblink_id == ANTI_REFLUX_DBLINK_ID_INVALID)
        return 0;
    return 1;
}

/*********************************************************************************************************
** Function name:       anti_reflux_sg_step
** Descriptions:        安全防线状态机单步推进（通信/限值偏差/分合闸逻辑）
** input parameters:    ctx, cfg, model, inv_inputs, now_ms, dt_ms
** output parameters:   ctx - 更新运行时状态与待执行命令标志
** Returned value:      0-成功；-1-参数无效
*********************************************************************************************************/
int anti_reflux_sg_step(AntiRefluxSafetyGuardCtx *ctx,
                        const AntiRefluxConfig *cfg,
                        const AntiRefluxDeviceModel *model,
                        const AntiRefluxSgInvInput *inv_inputs,
                        uint32_t now_ms,
                        uint32_t dt_ms,
                        int debug)
{
    uint32_t i;

    if (!ctx || !cfg || !model || !inv_inputs)
        return -1;

    ctx->inverter_count = model->inverter_count;
    if (ctx->inverter_count > ANTI_REFLUX_MAX_INVERTERS)
        ctx->inverter_count = ANTI_REFLUX_MAX_INVERTERS;

    for (i = 0; i < ctx->inverter_count; i++)
    {
        AntiRefluxSgInvRuntime *st = &ctx->inv[i];
        const AntiRefluxInverterModel *inv = &model->inverters[i];
        const AntiRefluxSgInvInput *in = &inv_inputs[i];
        int sg_on = inv_sg_enabled(cfg, inv);
        double mismatch_abs;
        int hard_risk;
        int light_risk;

        if (!sg_on)
        {
            st->state = AR_SG_DISABLED;
            st->allow_coord = 1;
            st->comm_fail_cycles = 0;
            st->mismatch_cycles = 0;
            st->trip_pending_ms = 0;
            st->trip_hold_ms = 0;
            st->cooldown_ms = 0;
            st->trip_cmd_pending = 0;
            st->close_cmd_pending = 0;
            st->waiting_trip_feedback = 0;
            st->waiting_close_feedback = 0;
            st->relay_seq = AR_SG_RELAY_SEQ_NONE;
            st->relay_seq_step = 0;
            st->feedback_wait_ms = 0;
            st->last_limit_cmd_w = in->inv_limit_cmd_w;
            continue;
        }

        if (!in->inv_comm_ok)
            st->comm_fail_cycles++;
        else
            st->comm_fail_cycles = 0;

        mismatch_abs = in->inv_limit_cmd_w - in->inv_limit_readback_w;
        if (mismatch_abs < 0.0)
            mismatch_abs = -mismatch_abs;
        if (mismatch_abs >= (double)cfg->safety_guard.limit_mismatch_w)
            st->mismatch_cycles++;
        else
            st->mismatch_cycles = 0;

        hard_risk = ((st->comm_fail_cycles >= cfg->safety_guard.comm_fail_cycles_hard) ||
                     (st->mismatch_cycles >= cfg->safety_guard.limit_mismatch_cycles)) &&
                    in->reverse_direction;
        light_risk = (st->comm_fail_cycles >= cfg->safety_guard.comm_fail_cycles_light) ||
                     (st->mismatch_cycles > 0U);

        if (st->trip_hour_epoch_s == 0U)
            st->trip_hour_epoch_s = ms_to_s(now_ms);
        if (ms_to_s(now_ms) - st->trip_hour_epoch_s >= 3600U)
        {
            st->trip_hour_epoch_s = ms_to_s(now_ms);
            st->trip_count_hour = 0;
        }

        switch (st->state)
        {
            case AR_SG_DISABLED:
            case AR_SG_MONITOR:
                st->allow_coord = 1;
                st->waiting_trip_feedback = 0;
                if (hard_risk)
                {
                    st->state = AR_SG_TRIP_PENDING;
                    st->trip_pending_ms = 0;
                }
                else if (light_risk)
                {
                    st->state = AR_SG_SUSPECT;
                }
                else
                {
                    st->state = AR_SG_MONITOR;
                }
                /* 非分闸窗口：若读到仍是分闸态，则做闭环合闸修正 */
                if (st->state == AR_SG_MONITOR && !cfg->safety_guard.manual_reclose_only)
                {
                    if (relay_is_open(in))
                    {
                        if (!st->waiting_close_feedback)
                        {
                            sg_set_close_cmd_pending(st, debug, i, "monitor relay open");
                            st->waiting_close_feedback = 1;
                            st->feedback_wait_ms = 0;
                        }
                        else
                        {
                            st->feedback_wait_ms += dt_ms;
                            if (st->feedback_wait_ms >= cfg->safety_guard.effect_timeout_s * 1000U)
                            {
                                sg_set_close_cmd_pending(st, debug, i, "monitor close timeout retry");
                                st->feedback_wait_ms = 0;
                            }
                        }
                    }
                    else if (relay_is_closed(in))
                    {
                        st->waiting_close_feedback = 0;
                        st->feedback_wait_ms = 0;
                    }
                }
                break;
            case AR_SG_SUSPECT:
                st->allow_coord = 1;
                st->waiting_trip_feedback = 0;
                if (st->comm_fail_cycles == 0U && st->mismatch_cycles == 0U)
                    st->state = AR_SG_MONITOR;
                else if (hard_risk)
                {
                    st->state = AR_SG_TRIP_PENDING;
                    st->trip_pending_ms = 0;
                }
                break;
            case AR_SG_TRIP_PENDING:
                st->allow_coord = 1;
                if (!hard_risk)
                {
                    /* 风险条件已消除，退出分闸确认窗口，避免“恢复后延迟误分闸” */
                    st->state = AR_SG_MONITOR;
                    st->trip_pending_ms = 0;
                    st->trip_cmd_pending = 0;
                    st->waiting_trip_feedback = 0;
                    st->feedback_wait_ms = 0;
                    break;
                }
                st->trip_pending_ms += dt_ms;
                if (st->trip_pending_ms >= cfg->safety_guard.trip_confirm_s * 1000U)
                {
                    if (st->trip_count_hour < cfg->safety_guard.max_trips_per_hour)
                    {
                        if (!st->waiting_trip_feedback)
                        {
                            sg_set_trip_cmd_pending(st, debug, i, "trip confirm elapsed");
                            st->waiting_trip_feedback = 1;
                            st->feedback_wait_ms = 0;
                        }

                        if (relay_is_open(in))
                        {
                            st->waiting_trip_feedback = 0;
                            st->trip_count_hour++;
                            st->state = AR_SG_TRIPPED;
                            st->trip_hold_ms = 0;
                            st->allow_coord = 0;
                        }
                        else
                        {
                            st->allow_coord = 0; /* 已触发硬动作流程，先剔除统筹 */
                            st->feedback_wait_ms += dt_ms;
                            if (st->feedback_wait_ms >= cfg->safety_guard.effect_timeout_s * 1000U)
                            {
                                sg_set_trip_cmd_pending(st, debug, i, "trip feedback timeout retry");
                                st->feedback_wait_ms = 0;
                            }
                        }
                    }
                    else
                    {
                        /* 已达每小时分闸上限：保持监控但不再触发本轮分闸命令 */
                        st->trip_cmd_pending = 0;
                        st->waiting_trip_feedback = 0;
                        st->state = AR_SG_MONITOR;
                    }
                }
                break;
            case AR_SG_TRIPPED:
                st->allow_coord = 0;
                if (!relay_is_open(in))
                {
                    st->waiting_trip_feedback = 1;
                    st->feedback_wait_ms += dt_ms;
                    if (st->feedback_wait_ms >= cfg->safety_guard.effect_timeout_s * 1000U)
                    {
                        sg_set_trip_cmd_pending(st, debug, i, "tripped hold retry");
                        st->feedback_wait_ms = 0;
                    }
                }
                else
                {
                    st->waiting_trip_feedback = 0;
                    st->feedback_wait_ms = 0;
                }
                st->trip_hold_ms += dt_ms;
                if (st->trip_hold_ms >= cfg->safety_guard.trip_hold_min_s * 1000U)
                {
                    st->state = AR_SG_RECLOSE_COOLDOWN;
                    st->cooldown_ms = 0;
                }
                break;
            case AR_SG_RECLOSE_COOLDOWN:
                st->allow_coord = 0;
                st->cooldown_ms += dt_ms;
                if (st->cooldown_ms >= cfg->safety_guard.reclose_cooldown_s * 1000U)
                {
                    if (!cfg->safety_guard.manual_reclose_only)
                    {
                        if (!st->waiting_close_feedback)
                        {
                            sg_set_close_cmd_pending(st, debug, i, "reclose cooldown done");
                            st->waiting_close_feedback = 1;
                            st->feedback_wait_ms = 0;
                        }
                        if (relay_is_closed(in))
                        {
                            st->waiting_close_feedback = 0;
                            st->state = AR_SG_MONITOR;
                            st->allow_coord = 1;
                            st->comm_fail_cycles = 0;
                            st->mismatch_cycles = 0;
                            st->feedback_wait_ms = 0;
                        }
                        else
                        {
                            st->feedback_wait_ms += dt_ms;
                            if (st->feedback_wait_ms >= cfg->safety_guard.effect_timeout_s * 1000U)
                            {
                                sg_set_close_cmd_pending(st, debug, i, "reclose feedback timeout retry");
                                st->feedback_wait_ms = 0;
                            }
                        }
                    }
                }
                break;
            default:
                st->state = AR_SG_MONITOR;
                st->allow_coord = 1;
                break;
        }

        st->last_limit_cmd_w = in->inv_limit_cmd_w;
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       anti_reflux_sg_allow_coord
** Descriptions:        查询指定逆变器是否允许参与防逆流统筹
** input parameters:    ctx - 运行时上下文；inv_idx - 逆变器索引
** output parameters:   NONE
** Returned value:      1-允许；0-不允许
*********************************************************************************************************/
uint8_t anti_reflux_sg_allow_coord(const AntiRefluxSafetyGuardCtx *ctx, uint32_t inv_idx)
{
    if (!ctx || inv_idx >= ctx->inverter_count || inv_idx >= ANTI_REFLUX_MAX_INVERTERS)
        return 1;
    return ctx->inv[inv_idx].allow_coord ? 1 : 0;
}

/*********************************************************************************************************
** Function name:       sg_relay_interval_ms
** Descriptions:        获取继电器命令最小间隔毫秒数
** input parameters:    cfg - 配置
** output parameters:   NONE
** Returned value:      间隔毫秒，默认 200
*********************************************************************************************************/
static uint32_t sg_relay_interval_ms(const AntiRefluxConfig *cfg)
{
    uint32_t interval;

    if (!cfg)
        return 200U;
    interval = cfg->safety_guard.relay_command_interval_ms;
    return (interval > 0U) ? interval : 200U;
}

/*********************************************************************************************************
** Function name:       sg_relay_interval_elapsed
** Descriptions:        判断是否到达下一次继电器写点间隔
** input parameters:    last_ms - 上次写点时间；now_ms - 当前时间；min_interval - 最小间隔
** output parameters:   NONE
** Returned value:      1-可写；0-需等待
*********************************************************************************************************/
static int sg_relay_interval_elapsed(uint32_t last_ms, uint32_t now_ms, uint32_t min_interval)
{
    if (last_ms == 0U)
        return 1;
    if (min_interval == 0U)
        return 1;
    return (now_ms - last_ms >= min_interval) ? 1 : 0;
}

/*********************************************************************************************************
** Function name:       sg_relay_release_both
** Descriptions:        保持型双线圈：吸合目标前先释放 S1/S2
** input parameters:    inv, inv_idx, write_ctrl_fn
** output parameters:   NONE
** Returned value:      0-成功；-1-失败
*********************************************************************************************************/
static int sg_relay_release_both(const AntiRefluxInverterModel *inv,
                                 unsigned inv_idx,
                                 anti_reflux_sg_write_ctrl_fn write_ctrl_fn,
                                 int debug)
{
    double v_off = 0.0;
    int wr;

    if (!inv || !write_ctrl_fn)
        return -1;

    wr = write_ctrl_fn(inv->safety_trip_dblink_id, v_off);
    if (wr != 0)
    {
    fprintf(stderr,
            "[anti_reflux][sg] release S1(trip) failed: inv=%u dblink=0x%08X ret=%d\n",
            inv_idx, (unsigned)inv->safety_trip_dblink_id, wr);
        return -1;
    }
    if (debug)
    {
        fprintf(stderr,
                "[anti_reflux][sg] relay write: inv=%u S1(trip)=0 dblink=0x%08X (release both step)\n",
                inv_idx, (unsigned)inv->safety_trip_dblink_id);
    }

    wr = write_ctrl_fn(inv->safety_close_dblink_id, v_off);
    if (wr != 0)
    {
        fprintf(stderr,
                "[anti_reflux][sg] release S2(close) failed: inv=%u dblink=0x%08X ret=%d\n",
                inv_idx, (unsigned)inv->safety_close_dblink_id, wr);
        return -1;
    }
    if (debug)
    {
        fprintf(stderr,
                "[anti_reflux][sg] relay write: inv=%u S2(close)=0 dblink=0x%08X (release both step)\n",
                inv_idx, (unsigned)inv->safety_close_dblink_id);
    }
    return 0;
}

/*********************************************************************************************************
** Function name:       sg_relay_seq_advance
** Descriptions:        推进双继电器分/合闸序列（先释放对侧，再吸合目标）
** input parameters:    st, inv, inv_idx, write_ctrl_fn, now_ms
** output parameters:   st - 更新序列步进与时间戳
** Returned value:      0-成功或无需推进；-1-写点失败
*********************************************************************************************************/
static int sg_relay_seq_advance(AntiRefluxSgInvRuntime *st,
                              const AntiRefluxInverterModel *inv,
                              unsigned inv_idx,
                              anti_reflux_sg_write_ctrl_fn write_ctrl_fn,
                              uint32_t now_ms,
                              int debug)
{
    double v_on = 1.0;
    int wr;

    if (!st || !inv || !write_ctrl_fn || st->relay_seq == AR_SG_RELAY_SEQ_NONE)
        return 0;

    if (st->relay_seq == AR_SG_RELAY_SEQ_TRIP)
    {
        if (st->relay_seq_step == 0U)
        {
            if (sg_relay_release_both(inv, inv_idx, write_ctrl_fn, debug) != 0)
                return -1;
            st->relay_seq_step = 1U;
            st->last_relay_cmd_ms = now_ms;
            return 0;
        }
        wr = write_ctrl_fn(inv->safety_trip_dblink_id, v_on);
        if (wr != 0)
        {
            fprintf(stderr,
                    "[anti_reflux][sg] trip seq energize S1 failed: inv=%u dblink=0x%08X ret=%d\n",
                    inv_idx, (unsigned)inv->safety_trip_dblink_id, wr);
            return -1;
        }
        if (debug)
        {
            fprintf(stderr,
                    "[anti_reflux][sg] relay write: inv=%u S1(trip)=1 dblink=0x%08X (TRIP energize)\n",
                    inv_idx, (unsigned)inv->safety_trip_dblink_id);
        }
        st->relay_seq = AR_SG_RELAY_SEQ_NONE;
        st->relay_seq_step = 0U;
        st->trip_cmd_pending = 0;
        st->waiting_trip_feedback = 1;
        st->last_relay_cmd_ms = now_ms;
        return 0;
    }

    if (st->relay_seq == AR_SG_RELAY_SEQ_CLOSE)
    {
        if (st->relay_seq_step == 0U)
        {
            if (sg_relay_release_both(inv, inv_idx, write_ctrl_fn, debug) != 0)
                return -1;
            st->relay_seq_step = 1U;
            st->last_relay_cmd_ms = now_ms;
            return 0;
        }
        wr = write_ctrl_fn(inv->safety_close_dblink_id, v_on);
        if (wr != 0)
        {
            fprintf(stderr,
                    "[anti_reflux][sg] close seq energize S2 failed: inv=%u dblink=0x%08X ret=%d\n",
                    inv_idx, (unsigned)inv->safety_close_dblink_id, wr);
            return -1;
        }
        if (debug)
        {
            fprintf(stderr,
                    "[anti_reflux][sg] relay write: inv=%u S2(close)=1 dblink=0x%08X (CLOSE energize)\n",
                    inv_idx, (unsigned)inv->safety_close_dblink_id);
        }
        st->relay_seq = AR_SG_RELAY_SEQ_NONE;
        st->relay_seq_step = 0U;
        st->close_cmd_pending = 0;
        st->waiting_close_feedback = 1;
        st->last_relay_cmd_ms = now_ms;
        return 0;
    }

    st->relay_seq = AR_SG_RELAY_SEQ_NONE;
    st->relay_seq_step = 0U;
    return 0;
}

/*********************************************************************************************************
** Function name:       anti_reflux_sg_flush_commands
** Descriptions:        下发安全防线待执行的继电器分/合闸命令
** input parameters:    ctx, cfg, model, write_ctrl_fn, now_ms
** output parameters:   NONE
** Returned value:      0-成功；-1-参数无效
*********************************************************************************************************/
int anti_reflux_sg_flush_commands(AntiRefluxSafetyGuardCtx *ctx,
                                  const AntiRefluxConfig *cfg,
                                  const AntiRefluxDeviceModel *model,
                                  anti_reflux_sg_write_ctrl_fn write_ctrl_fn,
                                  uint32_t now_ms,
                                  int debug)
{
    uint32_t i;

    if (!ctx || !cfg || !model || !write_ctrl_fn)
        return -1;

    for (i = 0; i < ctx->inverter_count && i < model->inverter_count; i++)
    {
        AntiRefluxSgInvRuntime *st = &ctx->inv[i];
        const AntiRefluxInverterModel *inv = &model->inverters[i];
        uint32_t min_interval = sg_relay_interval_ms(cfg);

        if (!inv_sg_enabled(cfg, inv))
            continue;

        /* 分闸优先：合闸序列进行中若新来分闸请求，中止合闸改走分闸 */
        if (st->trip_cmd_pending && st->relay_seq == AR_SG_RELAY_SEQ_CLOSE)
        {
            if (debug)
            {
                fprintf(stderr,
                        "[anti_reflux][sg] inv[%u] flush abort CLOSE seq, TRIP takes priority\n",
                        (unsigned)i);
            }
            st->relay_seq = AR_SG_RELAY_SEQ_NONE;
            st->relay_seq_step = 0U;
            st->close_cmd_pending = 0;
        }

        if (st->relay_seq != AR_SG_RELAY_SEQ_NONE)
        {
            if (!sg_relay_interval_elapsed(st->last_relay_cmd_ms, now_ms, min_interval))
                continue;
            if (debug)
            {
                fprintf(stderr,
                        "[anti_reflux][sg] inv[%u] flush advance seq=%s step=%u now_ms=%u\n",
                        (unsigned)i, anti_reflux_sg_relay_seq_name(st->relay_seq),
                        (unsigned)st->relay_seq_step, (unsigned)now_ms);
            }
            (void)sg_relay_seq_advance(st, inv, (unsigned)i, write_ctrl_fn, now_ms, debug);
            continue;
        }

        if (!sg_relay_interval_elapsed(st->last_relay_cmd_ms, now_ms, min_interval))
            continue;

        if (st->trip_cmd_pending)
        {
            if (debug)
            {
                fprintf(stderr,
                        "[anti_reflux][sg] inv[%u] flush start TRIP seq S1=0x%08X S2=0x%08X\n",
                        (unsigned)i,
                        (unsigned)inv->safety_trip_dblink_id,
                        (unsigned)inv->safety_close_dblink_id);
            }
            st->relay_seq = AR_SG_RELAY_SEQ_TRIP;
            st->relay_seq_step = 0U;
            (void)sg_relay_seq_advance(st, inv, (unsigned)i, write_ctrl_fn, now_ms, debug);
            continue;
        }
        if (st->close_cmd_pending)
        {
            if (debug)
            {
                fprintf(stderr,
                        "[anti_reflux][sg] inv[%u] flush start CLOSE seq S1=0x%08X S2=0x%08X\n",
                        (unsigned)i,
                        (unsigned)inv->safety_trip_dblink_id,
                        (unsigned)inv->safety_close_dblink_id);
            }
            st->relay_seq = AR_SG_RELAY_SEQ_CLOSE;
            st->relay_seq_step = 0U;
            (void)sg_relay_seq_advance(st, inv, (unsigned)i, write_ctrl_fn, now_ms, debug);
            continue;
        }
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       sg_test_arm_relay_cmd
** Descriptions:        模拟 step 置位待下发的分/合闸命令（与运行态 pending 标志一致）
** input parameters:    st - 逆变器运行时；trip - 1=分闸；0=合闸
** output parameters:   st - 更新 pending 与序列状态
** Returned value:      NONE
*********************************************************************************************************/
static void sg_test_arm_relay_cmd(AntiRefluxSgInvRuntime *st, int trip)
{
    if (!st)
        return;

    st->relay_seq = AR_SG_RELAY_SEQ_NONE;
    st->relay_seq_step = 0U;
    st->last_relay_cmd_ms = 0U;
    if (trip)
    {
        st->trip_cmd_pending = 1U;
        st->close_cmd_pending = 0U;
    }
    else
    {
        st->close_cmd_pending = 1U;
        st->trip_cmd_pending = 0U;
    }
}

/*********************************************************************************************************
** Function name:       sg_relay_pending_active
** Descriptions:        判断指定逆变器是否仍有继电器命令/序列未完成
** input parameters:    st - 逆变器运行时
** output parameters:   NONE
** Returned value:      1-仍有活动；0-空闲
*********************************************************************************************************/
static int sg_relay_pending_active(const AntiRefluxSgInvRuntime *st)
{
    if (!st)
        return 0;
    return (st->relay_seq != AR_SG_RELAY_SEQ_NONE ||
            st->trip_cmd_pending || st->close_cmd_pending) ? 1 : 0;
}

/*********************************************************************************************************
** Function name:       sg_flush_relay_until_idle
** Descriptions:        循环调用 anti_reflux_sg_flush_commands 直至继电器序列完成
** input parameters:    ctx, cfg, model, write_ctrl_fn, inv_idx, now_ms
** output parameters:   now_ms - 推进后的时间戳
** Returned value:      0-成功；-1-超时或 flush 失败
*********************************************************************************************************/
static int sg_flush_relay_until_idle(AntiRefluxSafetyGuardCtx *ctx,
                                     const AntiRefluxConfig *cfg,
                                     const AntiRefluxDeviceModel *model,
                                     anti_reflux_sg_write_ctrl_fn write_ctrl_fn,
                                     uint32_t inv_idx,
                                     uint32_t *now_ms,
                                     int debug)
{
    AntiRefluxSgInvRuntime *st;
    uint32_t interval;
    unsigned loops = 0U;

    if (!ctx || !cfg || !model || !write_ctrl_fn || !now_ms ||
        inv_idx >= ctx->inverter_count || inv_idx >= ANTI_REFLUX_MAX_INVERTERS)
        return -1;

    st = &ctx->inv[inv_idx];
    interval = sg_relay_interval_ms(cfg);

    while (sg_relay_pending_active(st))
    {
        int ret;

        if (loops >= 32U)
        {
            fprintf(stderr,
                    "[anti_reflux][sg][test] flush timeout: inv=%u relay_seq=%u step=%u ptrip=%u pclose=%u\n",
                    (unsigned)inv_idx,
                    (unsigned)st->relay_seq,
                    (unsigned)st->relay_seq_step,
                    (unsigned)st->trip_cmd_pending,
                    (unsigned)st->close_cmd_pending);
            return -1;
        }

        ret = anti_reflux_sg_flush_commands(ctx, cfg, model, write_ctrl_fn, *now_ms, debug);
        if (ret != 0)
            return ret;

        if (!sg_relay_pending_active(st))
            break;

        usleep((useconds_t)interval * 1000U);
        *now_ms += interval;
        loops++;
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       sg_test_run_one_relay_cmd
** Descriptions:        通过 flush_commands 执行一次分闸或合闸（与 controller cycle_end 同路径）
** input parameters:    ctx, cfg, model, write_ctrl_fn, inv_idx, trip, now_ms
** output parameters:   now_ms - 推进后的时间戳
** Returned value:      0-成功；-1-失败
*********************************************************************************************************/
static int sg_test_run_one_relay_cmd(AntiRefluxSafetyGuardCtx *ctx,
                                     const AntiRefluxConfig *cfg,
                                     const AntiRefluxDeviceModel *model,
                                     anti_reflux_sg_write_ctrl_fn write_ctrl_fn,
                                     uint32_t inv_idx,
                                     int trip,
                                     uint32_t *now_ms)
{
    const AntiRefluxInverterModel *inv;
    AntiRefluxSgInvRuntime *st;

    if (!ctx || !cfg || !model || !write_ctrl_fn || !now_ms ||
        inv_idx >= model->inverter_count || inv_idx >= ANTI_REFLUX_MAX_INVERTERS)
        return -1;

    inv = &model->inverters[inv_idx];
    st = &ctx->inv[inv_idx];

    if (!inv_sg_enabled(cfg, inv))
    {
        fprintf(stderr,
                "[anti_reflux][sg][test] inv[%u] safety guard relay not enabled "
                "(need config.safety_guard.enable + InverterRefs.safety_relay_bind.enabled + persist enable)\n",
                (unsigned)inv_idx);
        return -1;
    }

    fprintf(stderr,
            "[anti_reflux][sg][test] arm %s via flush_commands: inv=%u S1(trip)=0x%08X S2(close)=0x%08X interval_ms=%u\n",
            trip ? "TRIP" : "CLOSE",
            (unsigned)inv_idx,
            (unsigned)inv->safety_trip_dblink_id,
            (unsigned)inv->safety_close_dblink_id,
            (unsigned)sg_relay_interval_ms(cfg));

    sg_test_arm_relay_cmd(st, trip);
    if (sg_flush_relay_until_idle(ctx, cfg, model, write_ctrl_fn, inv_idx, now_ms, 1) != 0)
        return -1;

    fprintf(stderr,
            "[anti_reflux][sg][test] %s flush done: inv=%u expect S1=%s S2=%s\n",
            trip ? "TRIP" : "CLOSE",
            (unsigned)inv_idx,
            trip ? "ON(1)" : "OFF(0)",
            trip ? "OFF(0)" : "ON(1)");
    return 0;
}

/*********************************************************************************************************
** Function name:       anti_reflux_sg_test_relay_output
** Descriptions:        -o 直测：置位 pending 后走 anti_reflux_sg_flush_commands 下发
** input parameters:    ctx, cfg, model, write_ctrl_fn, inv_idx, cmd, now_ms
** output parameters:   NONE
** Returned value:      0-成功；-1-失败
*********************************************************************************************************/
int anti_reflux_sg_test_relay_output(AntiRefluxSafetyGuardCtx *ctx,
                                     AntiRefluxConfig *cfg,
                                     const AntiRefluxDeviceModel *model,
                                     anti_reflux_sg_write_ctrl_fn write_ctrl_fn,
                                     uint32_t inv_idx,
                                     AntiRefluxSgTestRelayCmd cmd,
                                     uint32_t now_ms)
{
    uint32_t interval;
    int ret;

    if (!ctx || !cfg || !model || !write_ctrl_fn)
        return -1;
    if (inv_idx >= model->inverter_count || inv_idx >= ANTI_REFLUX_MAX_INVERTERS)
    {
        fprintf(stderr, "[anti_reflux][sg][test] invalid inv_idx=%u (count=%u)\n",
                (unsigned)inv_idx, (unsigned)model->inverter_count);
        return -1;
    }

    if (ctx->inverter_count == 0U)
        anti_reflux_sg_init(ctx, model);

    if (!inv_sg_enabled(cfg, &model->inverters[inv_idx]))
    {
        fprintf(stderr,
                "[anti_reflux][sg][test] inv[%u] not ready for safety-guard relay flush "
                "(trip=0x%08X close=0x%08X state=0x%08X sg_enable=%d relay_enable=%d persist=%d)\n",
                (unsigned)inv_idx,
                (unsigned)model->inverters[inv_idx].safety_trip_dblink_id,
                (unsigned)model->inverters[inv_idx].safety_close_dblink_id,
                (unsigned)model->inverters[inv_idx].safety_state_dblink_id,
                cfg->safety_guard.enable ? 1 : 0,
                model->inverters[inv_idx].safety_relay_enabled ? 1 : 0,
                model->inverters[inv_idx].persist_enabled ? 1 : 0);
        return -1;
    }

    interval = sg_relay_interval_ms(cfg);

    switch (cmd)
    {
        case AR_SG_TEST_RELAY_TRIP:
            return sg_test_run_one_relay_cmd(ctx, cfg, model, write_ctrl_fn, inv_idx, 1, &now_ms);
        case AR_SG_TEST_RELAY_CLOSE:
            return sg_test_run_one_relay_cmd(ctx, cfg, model, write_ctrl_fn, inv_idx, 0, &now_ms);
        case AR_SG_TEST_RELAY_BOTH:
            ret = sg_test_run_one_relay_cmd(ctx, cfg, model, write_ctrl_fn, inv_idx, 1, &now_ms);
            if (ret != 0)
                return ret;
            usleep((useconds_t)interval * 1000U);
            now_ms += interval;
            return sg_test_run_one_relay_cmd(ctx, cfg, model, write_ctrl_fn, inv_idx, 0, &now_ms);
        default:
            fprintf(stderr, "[anti_reflux][sg][test] unknown cmd=%d\n", (int)cmd);
            return -1;
    }
}

