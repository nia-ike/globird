/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：edge_alert_engine.h
* 文件标识：
* 描    述：edge_alert 告警引擎头文件
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：创建
* 完成日期：20240702
*
*/

#ifndef EDGE_ALERT_ENGINE_H
#define EDGE_ALERT_ENGINE_H

#include <stdint.h>
#include "edge_alert_types.h"

/*************************************************************************/
/** Function Name:       alarm_engine_eval_condition                         **/
/** Purpose:             评估单个条件                                       **/
/** Params:            cond - 条件配置                                    **/
/**                    current_val - 当前值                               **/
/**                    prev_triggered - 之前是否已触发                     **/
/**                    threshold - 输出实际使用的阈值（考虑滞后）           **/
/** Return:            1-条件满足；0-条件不满足                             **/
/** Notice:            根据比较方法判断是否满足条件，支持滞后处理           **/
/*************************************************************************/
int alarm_engine_eval_condition(struct AlarmCondition *cond, double current_val,
                                int prev_triggered, double *threshold);

/*************************************************************************/
/** Function Name:       alarm_engine_eval_trigger                           **/
/** Purpose:             评估触发器（复合条件）                              **/
/** Params:            trigger - 触发器配置                                 **/
/** Return:            1-触发器满足；0-不满足；-1-无法评估（点位异常）      **/
/** Notice:            根据逻辑组合（OR/AND）评估所有条件                   **/
/*************************************************************************/
int alarm_engine_eval_trigger(struct AlarmTrigger *trigger);

/*************************************************************************/
/** Function Name:       alarm_engine_eval_rule                              **/
/** Purpose:             评估单条告警规则                                   **/
/** Params:            rule - 告警规则配置                                 **/
/**                    instance - 活跃告警实例（NULL表示当前未触发）       **/
/**                    now_ms - 当前时间戳（毫秒）                           **/
/**                    should_fire - 输出：是否应该触发告警                  **/
/**                    should_recover - 输出：是否应该恢复告警             **/
/**                    should_pending - 输出：是否应创建延时确认实例         **/
/**                    should_cancel_pending - 输出：是否应取消未触发实例    **/
/** Return:            0-评估完成； -1-评估出错                             **/
/** Notice:            三层评估：主条件 -> 触发器 -> 延时确认               **/
/*************************************************************************/
int alarm_engine_eval_rule(struct AlarmRule *rule, struct ActiveAlarmInstance *instance,
                           uint64_t now_ms, int *should_fire, int *should_recover,
                           int *should_pending, int *should_cancel_pending);

/*************************************************************************/
/** Function Name:       alarm_engine_update_extreme                          **/
/** Purpose:             更新活跃告警的极值统计                            **/
/** Params:            instance - 活跃告警实例                             **/
/**                    current_val - 当前值                               **/
/**                    now_ms - 当前时间戳（毫秒）                         **/
/** Return:            0-极值未变化； 1-最大值变化； 2-最小值变化；         **/
/**                    3-两者都变化                                         **/
/** Notice:            在告警持续期间跟踪最大/最小值及其发生时间           **/
/*************************************************************************/
int alarm_engine_update_extreme(struct ActiveAlarmInstance *instance, double current_val, uint64_t now_ms);

/*************************************************************************/
/** Function Name:       alarm_engine_state_machine                          **/
/** Purpose:             告警状态机处理                                    **/
/** Params:            rule - 告警规则                                   **/
/**                    instance - 活跃告警实例（可为NULL）                   **/
/**                    condition_met - 条件是否满足                        **/
/**                    now_ms - 当前时间戳                                 **/
/** Return:            新状态（AlarmState）                                **/
/** Notice:            NORMAL->PENDING->FIRED->RECOVERING->RECOVERED        **/
/*************************************************************************/
enum AlarmState alarm_engine_state_machine(struct AlarmRule *rule,
                                           struct ActiveAlarmInstance *instance,
                                           int condition_met, uint64_t now_ms);

/*************************************************************************/
/** Function Name:       alarm_engine_find_instance                          **/
/** Purpose:             在活跃告警管理器中查找实例                        **/
/** Params:            registry - 活跃告警管理器                            **/
/**                    rule_id - 规则ID                                    **/
/**                    chnl_id - 通道ID                                    **/
/**                    dev_id - 设备ID                                     **/
/** Return:            找到的实例指针； NULL-未找到                         **/
/*************************************************************************/
struct ActiveAlarmInstance *alarm_engine_find_instance(struct ActiveAlarmRegistry *registry,
                                                       uint16_t rule_id, uint8_t chnl_id, uint8_t dev_id);

/*************************************************************************/
/** Function Name:       alarm_engine_create_instance                        **/
/** Purpose:             创建新的活跃告警实例                              **/
/** Params:            registry - 活跃告警管理器                          **/
/**                    rule - 触发告警的规则                                **/
/**                    trigger_val - 触发时的值                             **/
/**                    now_ms - 触发时间戳                                  **/
/** Return:            新创建的实例指针； NULL-失败                          **/
/*************************************************************************/
struct ActiveAlarmInstance *alarm_engine_create_instance(struct ActiveAlarmRegistry *registry,
                                                         struct AlarmRule *rule,
                                                         double trigger_val, uint64_t now_ms);

/*************************************************************************/
/** Function Name:       alarm_engine_remove_instance                        **/
/** Purpose:             从活跃告警管理器中移除实例                        **/
/** Params:            registry - 活跃告警管理器                          **/
/**                    instance - 要移除的实例指针                         **/
/** Return:            0-成功； -1-失败                                    **/
/*************************************************************************/
int alarm_engine_remove_instance(struct ActiveAlarmRegistry *registry,
                                 struct ActiveAlarmInstance *instance);

/*************************************************************************/
/** Function Name:       alarm_engine_read_point_value                       **/
/** Purpose:             读取点位当前值                                   **/
/** Params:            point_id - dblink点位ID                             **/
/**                    value - 输出值                                       **/
/** Return:            0-成功；-1-失败（点位不存在或离线）                   **/
/*************************************************************************/
int alarm_engine_read_point_value(uint32_t point_id, double *value);

/*************************************************************************/
/** Function Name:       alarm_engine_build_snapshot                         **/
/** Purpose:             构建告警快照JSON                                 **/
/** Params:            rule - 告警规则                                    **/
/**                    snapshot_json - 输出JSON字符串                       **/
/**                    max_len - 缓冲区最大长度                           **/
/** Return:            实际写入的字节数                                   **/
/*************************************************************************/
int alarm_engine_build_snapshot(struct AlarmRule *rule, char *snapshot_json, uint32_t max_len);

#endif /* EDGE_ALERT_ENGINE_H */
