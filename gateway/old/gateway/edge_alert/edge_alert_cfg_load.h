/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：edge_alert_cfg_load.h
* 文件标识：
* 描    述：edge_alert 配置加载接口
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：创建
* 完成日期：20240702
*
*/

#ifndef EDGE_ALERT_CFG_LOAD_H
#define EDGE_ALERT_CFG_LOAD_H

#include "edge_alert_types.h"

/*************************************************************************/
/** Function Name:       edge_alert_cfg_load                               **/
/** Purpose:             加载 edge_alert.json 及设备告警规则配置           **/
/** Params:            cfg - 输出配置结构体                                **/
/**                      config_base - 配置根路径（如 /userdata/config）  **/
/** Return:            0-成功（含配置文件不存在时的空配置）               **/
/**                    -1-文件存在但读/解析失败                           **/
/** Notice:            路径为 {config_base}/edge_alert/edge_alert.json      **/
/**                    设备规则从 {config_base}/sample/devices/*.json 解析  **/
/*************************************************************************/
int edge_alert_cfg_load(struct EdgeAlertConfig *cfg, const char *config_base);

/*************************************************************************/
/** Function Name:       edge_alert_cfg_parse_method                       **/
/** Purpose:             解析比较方法字符串                                **/
/** Params:            str - 方法字符串（"upper"/"lower"/"equal"/"bit_on"） **/
/** Return:            对应的 AlarmCompareMethod                            **/
/*************************************************************************/
enum AlarmCompareMethod edge_alert_cfg_parse_method(const char *str);

/*************************************************************************/
/** Function Name:       edge_alert_cfg_parse_logic                        **/
/** Purpose:             解析逻辑组合字符串                                **/
/** Params:            str - 逻辑字符串（"or"/"and"）                        **/
/** Return:            对应的 AlarmLogic                                     **/
/*************************************************************************/
enum AlarmLogic edge_alert_cfg_parse_logic(const char *str);

#endif /* EDGE_ALERT_CFG_LOAD_H */
