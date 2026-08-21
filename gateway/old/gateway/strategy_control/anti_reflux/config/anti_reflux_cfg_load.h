/**
 * @file anti_reflux_cfg_load.h
 * @brief 防逆流策略 JSON 配置加载接口
 *
 * 从 config_base（设备上一般为 /userdata/config）下的 strategy、sample/channels、
 * sample/devices 等路径解析 JSON，将结果写入调用方传入的结构体指针，实现解耦。
 */

#ifndef ANTI_REFLUX_CFG_LOAD_H
#define ANTI_REFLUX_CFG_LOAD_H

#include "../anti_reflux_config.h"
#include "../anti_reflux_model.h"

/** 配置根路径最大长度（如 /userdata/config） */
#define ANTI_REFLUX_CFG_BASE_PATH_LEN   128

/*********************************************************************************************************
** Function name:       anti_reflux_config_load
** Descriptions:        加载防逆流工作参数到 pCfg
** input parameters:   pCfg        - 输出，防逆流 config 结构体指针
**                      config_base - 配置根路径（如 "/userdata/config"），结尾勿加 '/'
** Returned value:     0  - 成功
**                     -1 - 失败（文件不存在、非 anti_reflux 策略或解析错误）
*********************************************************************************************************/
int anti_reflux_config_load(AntiRefluxConfig *pCfg, const char *config_base);

/*********************************************************************************************************
** Function name:       anti_reflux_model_load
** Descriptions:        加载防逆流设备模型（电表 + 逆变器）及测点 dblink_id 到 pModel
** input parameters:   pModel      - 输出，防逆流设备模型结构体指针
**                      config_base - 配置根路径（如 "/userdata/config"）
** Returned value:     0  - 成功
**                     -1 - 失败
*********************************************************************************************************/
int anti_reflux_model_load(AntiRefluxDeviceModel *pModel, const char *config_base);

#endif /* ANTI_REFLUX_CFG_LOAD_H */
