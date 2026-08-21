/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：edge_alert_engine.c
* 文件标识：
* 描    述：edge_alert 告警引擎实现
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：创建
* 完成日期：20240702
*
*/

#include "edge_alert_engine.h"
#include "edge_alert_log.h"

#include "db_node_fops.h"

#include <string.h>
#include <stdio.h>

/* 内部常量 */
#define EDGE_ALERT_MAX_CONDITIONS      8
#define EDGE_ALERT_MAX_CONTEXT_POINTS 16

/*************************************************************************/
/** Function Name:       alarm_engine_calc_threshold_with_hysteresis         **/
/** Purpose:             计算带滞后的实际阈值                               **/
/** Notice:              触发后使用带滞后的阈值防止抖动                   **/
/*************************************************************************/
static double alarm_engine_calc_threshold_with_hysteresis(enum AlarmCompareMethod method,
                                                          double base_threshold,
                                                          uint8_t hysteresis_pct,
                                                          int prev_triggered)
{
    double hysteresis;

    if (!prev_triggered || hysteresis_pct == 0)
        return base_threshold;

    /* 计算滞后量 */
    hysteresis = base_threshold * hysteresis_pct / 100.0;

    switch (method)
    {
    case ALARM_CMP_GT:
        /* 原来是 > threshold，恢复时使用 > threshold * (1 - hysteresis%) */
        return base_threshold * (1.0 - (double)hysteresis_pct / 100.0);

    case ALARM_CMP_LT:
        /* 原来是 < threshold，恢复时使用 < threshold * (1 + hysteresis%) */
        return base_threshold * (1.0 + (double)hysteresis_pct / 100.0);

    default:
        return base_threshold;
    }
}

/*************************************************************************/
/** Function Name:       alarm_engine_eval_condition                         **/
/** Purpose:             评估单个条件                                       **/
/*************************************************************************/
int alarm_engine_eval_condition(struct AlarmCondition *cond, double current_val,
                                int prev_triggered, double *threshold)
{
    double effective_threshold;
    int result = 0;

    if (!cond)
        return 0;

    /* 计算实际阈值（考虑滞后） */
    effective_threshold = alarm_engine_calc_threshold_with_hysteresis(
        cond->method, cond->threshold, cond->hysteresis_pct, prev_triggered);

    if (threshold)
        *threshold = effective_threshold;

    switch (cond->method)
    {
    case ALARM_CMP_GT:
        result = (current_val > effective_threshold) ? 1 : 0;
        break;

    case ALARM_CMP_LT:
        result = (current_val < effective_threshold) ? 1 : 0;
        break;

    case ALARM_CMP_EQ:
        result = (current_val == effective_threshold) ? 1 : 0;
        break;

    case ALARM_CMP_NEQ:
        result = (current_val != effective_threshold) ? 1 : 0;
        break;

    case ALARM_CMP_BIT_ON:
        /* 检查指定bit是否为1 */
        result = (((uint32_t)current_val) & ((uint32_t)effective_threshold)) ? 1 : 0;
        break;

    case ALARM_CMP_BIT_OFF:
        /* 检查指定bit是否为0 */
        result = ((((uint32_t)current_val) & ((uint32_t)effective_threshold)) == 0) ? 1 : 0;
        break;

    default:
        result = 0;
        break;
    }

    return result;
}

/*************************************************************************/
/** Function Name:       alarm_engine_eval_trigger                           **/
/** Purpose:             评估触发器（复合条件）                              **/
/*************************************************************************/
int alarm_engine_eval_trigger(struct AlarmTrigger *trigger)
{
    int i;
    int condition_result;
    int any_valid = 0;
    double current_val;

    if (!trigger || trigger->condition_count == 0)
        return 1; /* 无条件，视为满足 */

    for (i = 0; i < trigger->condition_count; i++)
    {
        /* 读取条件点位值 */
        if (trigger->conditions[i].point_id == 0)
        {
            edge_log(LOG_WARN, "Condition %d has point_id=0, skipping", i);
            if (trigger->logic == ALARM_LOGIC_AND)
                return 0;
            continue;
        }
        if (alarm_engine_read_point_value(trigger->conditions[i].point_id, &current_val) != 0)
        {
            /* AND：任一条件不可评估视为不满足；OR：跳过尝试其余条件 */
            if (trigger->logic == ALARM_LOGIC_AND)
                return 0;
            continue;
        }

        any_valid = 1;
        condition_result = alarm_engine_eval_condition(&trigger->conditions[i], current_val, 0, NULL);

        if (trigger->logic == ALARM_LOGIC_OR)
        {
            /* OR逻辑：任一条件满足即满足 */
            if (condition_result)
                return 1;
        }
        else
        {
            /* AND逻辑：任一条件不满足即不满足 */
            if (!condition_result)
                return 0;
        }
    }

    /* 没有有效点位可评估 */
    if (!any_valid)
        return -1;

    /* OR逻辑：全部不满足 */
    /* AND逻辑：全部满足 */
    return (trigger->logic == ALARM_LOGIC_AND) ? 1 : 0;
}

/*************************************************************************/
/** Function Name:       alarm_engine_state_machine                          **/
/** Purpose:             告警状态机处理                                    **/
/*************************************************************************/
enum AlarmState alarm_engine_state_machine(struct AlarmRule *rule,
                                           struct ActiveAlarmInstance *instance,
                                           int condition_met, uint64_t now_ms)
{
    enum AlarmState current_state;

    if (!rule)
        return ALARM_STATE_NORMAL;

    /* 根据实例推断当前状态 */
    if (!instance)
    {
        current_state = ALARM_STATE_NORMAL;
    }
    else if (instance->db_record_id > 0)
    {
        /* 已写入数据库，处于告警中 */
        current_state = ALARM_STATE_FIRED;
    }
    else if (now_ms - instance->trigger_ms < (uint64_t)rule->delay_sec * 1000U)
    {
        /* 延时确认中 */
        current_state = ALARM_STATE_PENDING;
    }
    else
    {
        /* 延时已满足，待触发 */
        current_state = ALARM_STATE_FIRED;
    }

    switch (current_state)
    {
    case ALARM_STATE_NORMAL:
        if (condition_met)
        {
            /* 条件满足，进入PENDING状态 */
            return ALARM_STATE_PENDING;
        }
        return ALARM_STATE_NORMAL;

    case ALARM_STATE_PENDING:
        if (!condition_met)
        {
            /* 条件不再满足，返回NORMAL */
            return ALARM_STATE_NORMAL;
        }

        /* 检查是否满足延时确认 */
        if (instance)
        {
            uint64_t elapsed = now_ms - instance->trigger_ms;
            uint64_t required = rule->delay_sec * 1000;
            if (elapsed >= required)
            {
                /* 延时满足，进入FIRED */
                return ALARM_STATE_FIRED;
            }
        }
        return ALARM_STATE_PENDING;

    case ALARM_STATE_FIRED:
        if (!condition_met)
        {
            /* 条件不再满足，进入RECOVERED */
            return ALARM_STATE_RECOVERED;
        }
        return ALARM_STATE_FIRED;

    case ALARM_STATE_RECOVERED:
        /* RECOVERED是终止状态，外部处理后会移除实例 */
        return ALARM_STATE_NORMAL;

    default:
        return ALARM_STATE_NORMAL;
    }
}

/*************************************************************************/
/** Function Name:       alarm_engine_eval_rule                              **/
/** Purpose:             评估单条告警规则                                   **/
/*************************************************************************/
int alarm_engine_eval_rule(struct AlarmRule *rule, struct ActiveAlarmInstance *instance,
                           uint64_t now_ms, int *should_fire, int *should_recover,
                           int *should_pending, int *should_cancel_pending)
{
    int condition_met = 0;
    int trigger_result;
    enum AlarmState new_state;
    double main_point_val;
    double effective_threshold;
    int prev_triggered;

    if (!rule || !should_fire || !should_recover || !should_pending || !should_cancel_pending)
        return -1;

    *should_fire = 0;
    *should_recover = 0;
    *should_pending = 0;
    *should_cancel_pending = 0;

    if (!rule->enabled)
        return 0; /* 规则未使能 */

    prev_triggered = (instance != NULL && instance->db_record_id > 0) ? 1 : 0;

    /* === 第一层：评估主监控点条件 === */
    if (rule->point_id == 0)
    {
        edge_log(LOG_WARN, "Rule %s has point_id=0, cannot evaluate", rule->name);
        return -1;
    }
    if (alarm_engine_read_point_value(rule->point_id, &main_point_val) != 0)
    {
        /* 主点位无法读取，无法评估 */
        return -1;
    }

    condition_met = alarm_engine_eval_condition(
        &(struct AlarmCondition){
            .point_id = rule->point_id,
            .method = rule->method,
            .threshold = rule->threshold,
            .hysteresis_pct = rule->hysteresis_pct},
        main_point_val, prev_triggered, &effective_threshold);

    /* === 第二层：评估复合触发器（仅触发前 PENDING/待触发阶段，已告警后不再参与） === */
    if (condition_met && rule->trigger.condition_count > 0 &&
        (instance == NULL || instance->db_record_id == 0))
    {
        trigger_result = alarm_engine_eval_trigger(&rule->trigger);
        if (trigger_result <= 0)
        {
            condition_met = 0; /* 触发器未满足 */
        }
    }

    /* === 第三层：状态机处理 === */
    new_state = alarm_engine_state_machine(rule, instance, condition_met, now_ms);

    /* 判断转换 */
    if (new_state == ALARM_STATE_PENDING && instance == NULL)
    {
        /* 首次越限，创建延时确认实例 */
        *should_pending = 1;
    }

    if (instance != NULL && instance->db_record_id == 0 &&
        (new_state == ALARM_STATE_NORMAL || new_state == ALARM_STATE_RECOVERED))
    {
        /* 延时确认或待触发期间条件消失，取消未入库实例 */
        *should_cancel_pending = 1;
    }

    if (new_state == ALARM_STATE_FIRED && instance != NULL && instance->db_record_id == 0)
    {
        /* 延时确认完成，正式触发告警 */
        *should_fire = 1;
    }

    if (new_state == ALARM_STATE_RECOVERED && instance != NULL && instance->db_record_id > 0)
    {
        *should_recover = 1;
    }

    return 0;
}

/*************************************************************************/
/** Function Name:       alarm_engine_update_extreme                          **/
/** Purpose:             更新活跃告警的极值统计                            **/
/*************************************************************************/
int alarm_engine_update_extreme(struct ActiveAlarmInstance *instance, double current_val, uint64_t now_ms)
{
    int result = 0;

    if (!instance)
        return -1;

    /* 更新最大值 */
    if (instance->max_value_ms == 0 || current_val > instance->max_value)
    {
        instance->max_value = current_val;
        instance->max_value_ms = now_ms;
        result |= 1;
    }

    /* 更新最小值 */
    if (instance->min_value_ms == 0 || current_val < instance->min_value)
    {
        instance->min_value = current_val;
        instance->min_value_ms = now_ms;
        result |= 2;
    }

    instance->extreme_changed = (uint8_t)result;
    return result;
}

/*************************************************************************/
/** Function Name:       alarm_engine_find_instance                          **/
/** Purpose:             在活跃告警管理器中查找实例                        **/
/*************************************************************************/
struct ActiveAlarmInstance *alarm_engine_find_instance(struct ActiveAlarmRegistry *registry,
                                                       uint16_t rule_id, uint8_t chnl_id, uint8_t dev_id)
{
    int i;
    struct ActiveAlarmInstance *inst;

    if (!registry)
        return NULL;

    for (i = 0; i < registry->count; i++)
    {
        inst = &registry->instances[i];
        if (inst->rule_id == rule_id &&
            inst->chnl_id == chnl_id &&
            inst->dev_id == dev_id)
        {
            return inst;
        }
    }

    return NULL;
}

/*************************************************************************/
/** Function Name:       alarm_engine_create_instance                        **/
/** Purpose:             创建新的活跃告警实例                              **/
/*************************************************************************/
struct ActiveAlarmInstance *alarm_engine_create_instance(struct ActiveAlarmRegistry *registry,
                                                         struct AlarmRule *rule,
                                                         double trigger_val, uint64_t now_ms)
{
    struct ActiveAlarmInstance *inst;

    if (!registry || !rule)
        return NULL;

    if (registry->count >= EDGE_ALERT_CFG_MAX_ACTIVE_INSTANCES)
    {
        edge_log(LOG_ERROR, "Active alarm registry full");
        return NULL;
    }

    inst = &registry->instances[registry->count];
    memset(inst, 0, sizeof(struct ActiveAlarmInstance));

    /* 填充基本信息 */
    inst->rule_id = rule->rule_id;
    inst->chnl_id = rule->chnl_id;
    inst->dev_id = rule->dev_id;
    inst->point_id = rule->point_id;

    /* 生成alarm_key */
    snprintf(inst->alarm_key, sizeof(inst->alarm_key), "%02X%02X%04X",
             rule->chnl_id, rule->dev_id, rule->rule_id);

    /* 触发信息 */
    inst->trigger_ms = now_ms;
    inst->trigger_value = trigger_val;

    /* 初始化极值 */
    inst->max_value = trigger_val;
    inst->min_value = trigger_val;
    inst->max_value_ms = now_ms;
    inst->min_value_ms = now_ms;
    inst->extreme_changed = 3; /* 两者都变化 */

    /* 数据库记录ID待写入后填充 */
    inst->db_record_id = 0;

    registry->count++;

    return inst;
}

/*************************************************************************/
/** Function Name:       alarm_engine_remove_instance                        **/
/** Purpose:             从活跃告警管理器中移除实例                        **/
/*************************************************************************/
int alarm_engine_remove_instance(struct ActiveAlarmRegistry *registry,
                                 struct ActiveAlarmInstance *instance)
{
    int i;
    int found_idx = -1;

    if (!registry || !instance)
        return -1;

    /* 找到实例索引 */
    for (i = 0; i < registry->count; i++)
    {
        if (&registry->instances[i] == instance)
        {
            found_idx = i;
            break;
        }
    }

    if (found_idx < 0)
        return -1; /* 未找到 */

    /* 移动后续元素填补空缺 */
    for (i = found_idx; i < registry->count - 1; i++)
    {
        registry->instances[i] = registry->instances[i + 1];
    }

    registry->count--;

    return 0;
}

/*************************************************************************/
/** Function Name:       alarm_engine_read_point_value                       **/
/** Purpose:             读取点位当前值                                   **/
/** Notice:              调用实时库接口读取点位值                            **/
/*************************************************************************/
int alarm_engine_read_point_value(uint32_t point_id, double *value)
{
    if (!value)
        return -1;

    /* 调用 DataMgmtFops 读取点位值 */
    if (DataMgmtFops.readpoint_by_id)
    {
        if (DataMgmtFops.readpoint_by_id(point_id, value) == 0)
            return 0;
    }

    /* 读取失败 */
    return -1;
}

/*************************************************************************/
/** Function Name:       alarm_engine_build_snapshot                         **/
/** Purpose:             构建告警快照JSON                                 **/
/*************************************************************************/
int alarm_engine_build_snapshot(struct AlarmRule *rule, char *snapshot_json, uint32_t max_len)
{
    int i;
    int pos = 0;
    double val;

    if (!rule || !snapshot_json || max_len == 0)
        return 0;

    pos = snprintf(snapshot_json, max_len, "{");

    /* 添加上下文点位值 */
    for (i = 0; i < rule->context_count && pos < (int)max_len - 1; i++)
    {
        if (i > 0)
            pos += snprintf(snapshot_json + pos, max_len - pos, ",");

        /* 尝试读取点位值 */
        if (rule->context_points[i].point_id == 0)
        {
            edge_log(LOG_WARN, "Context point %d (%s) has point_id=0", i, rule->context_points[i].alias);
            pos += snprintf(snapshot_json + pos, max_len - pos,
                           "\"%s\":null", rule->context_points[i].alias);
        }
        else if (alarm_engine_read_point_value(rule->context_points[i].point_id, &val) == 0)
        {
            pos += snprintf(snapshot_json + pos, max_len - pos,
                           "\"%s\":%.2f", rule->context_points[i].alias, val);
        }
        else
        {
            pos += snprintf(snapshot_json + pos, max_len - pos,
                           "\"%s\":null", rule->context_points[i].alias);
        }
    }

    if (pos < (int)max_len)
        pos += snprintf(snapshot_json + pos, max_len - pos, "}");

    return pos;
}
