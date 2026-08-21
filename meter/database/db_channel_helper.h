/*
 * Copyright (c) 2025, 珠海科技股份有限公司
 * All rights reserved.
 * 
 * 文件名称: db_channel_helper.h
 * 文件标识: 多通道辅助宏和函数
 * 描    述: 提供优雅的多通道操作抽象，减少重复代码
 * 
 * 当前版本: V1.0
 * 作    者: AI Assistant
 * 创建日期: 2025-10-30
 */

#ifndef __DB_CHANNEL_HELPER_H__
#define __DB_CHANNEL_HELPER_H__

#include <stdint.h>
#include "db_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 多通道操作宏定义
 *============================================================================*/

/**
 * @brief 遍历所有通道执行操作
 * @param num_channels 通道总数
 * @param channel_var 循环变量名（会自动声明）
 * @param operation 要执行的操作（可以是多行语句块）
 * 
 * @example
 *   FOR_EACH_CHANNEL(num_channels, ch) {
 *       db_cfg_sys_update(&pDB->db_measurebase[ch], ch);
 *       db_msr_msg_event_cmd(ch, DB_MSR_EVENT_TYPE_SYS);
 *   }
 */
#define FOR_EACH_CHANNEL(num_channels, channel_var) \
    for(int channel_var = 0; channel_var < (num_channels); channel_var++)

/**
 * @brief 遍历所有通道执行单个函数调用
 * @param num_channels 通道总数
 * @param func 函数名
 * @param ... 函数参数（其中 CHANNEL 会被替换为通道号）
 * 
 * @example
 *   FOREACH_CHANNEL_CALL(num_channels, db_energy_destroy, 
 *                        &pDB->db_measurebase[CHANNEL], CHANNEL);
 */
#define FOREACH_CHANNEL_CALL(num_channels, func, ...) \
    do { \
        for(int _ch = 0; _ch < (num_channels); _ch++) { \
            func(__VA_ARGS__); \
        } \
    } while(0)

/**
 * @brief 从通道1开始遍历（跳过通道0）
 * @param num_channels 通道总数
 * @param channel_var 循环变量名
 * 
 * @example
 *   // 从通道0拷贝语言设置到其他通道
 *   FOR_EACH_CHANNEL_FROM_1(num_channels, ch) {
 *       pDB->db_measurebase[ch].db_cfg.user.base.Language = 
 *           pDB->db_measurebase[0].db_cfg.user.base.Language;
 *   }
 */
#define FOR_EACH_CHANNEL_FROM_1(num_channels, channel_var) \
    for(int channel_var = 1; channel_var < (num_channels); channel_var++)

/**
 * @brief 获取指定通道的 measurebase 指针
 */
#define GET_CHANNEL_BASE(pDB, channel) \
    (&((pDB)->db_measurebase[channel]))

/**
 * @brief 安全的通道号检查
 */
#define CHANNEL_VALID(channel, max_channels) \
    ((channel) >= 0 && (channel) < (max_channels))

/*============================================================================
 * 常用操作的内联辅助函数
 *============================================================================*/

/**
 * @brief 向所有通道发送相同的消息事件
 * @param num_channels 通道总数
 * @param event_type 事件类型
 */
static inline void db_broadcast_event_to_all_channels(int num_channels, int event_type)
{
    FOR_EACH_CHANNEL(num_channels, ch) {
        db_msr_msg_event_cmd(ch, event_type);
    }
}

/**
 * @brief 向所有通道广播配置更新事件（不调用更新函数）
 * @param num_channels 通道总数
 * @param event_type 事件类型
 * 
 * @note 此函数仅发送事件，不执行实际更新
 *       适用于更新函数签名不兼容的情况
 */
static inline void db_broadcast_cfg_event(int num_channels, int event_type)
{
    FOR_EACH_CHANNEL(num_channels, ch) {
        db_msr_msg_event_cmd(ch, event_type);
    }
}

/**
 * @brief 对所有通道执行初始化
 * @param pDB 数据库指针
 * @param num_channels 通道总数
 * @param init_func 初始化函数指针
 * @return 成功返回0，失败返回-1
 */
typedef int (*db_channel_init_fn)(struct db_database_measure_base_type*, uint8_t, const char*, const char*);

static inline int db_init_all_channels(
    struct db_database_type *pDB,
    int num_channels,
    db_channel_init_fn init_func,
    const char* (*get_main_path)(uint8_t),
    const char* (*get_backup_path)(uint8_t))
{
    FOR_EACH_CHANNEL(num_channels, ch) {
        if(init_func(&pDB->db_measurebase[ch], ch, 
                     get_main_path(ch), get_backup_path(ch)) < 0) {
            return -1;
        }
    }
    return 0;
}

/**
 * @brief 对所有通道执行销毁
 * @param pDB 数据库指针
 * @param num_channels 通道总数
 * @param destroy_func 销毁函数指针
 */
typedef void (*db_channel_destroy_fn)(struct db_database_measure_base_type*, uint8_t);

static inline void db_destroy_all_channels(
    struct db_database_type *pDB,
    int num_channels,
    db_channel_destroy_fn destroy_func)
{
    FOR_EACH_CHANNEL(num_channels, ch) {
        destroy_func(&pDB->db_measurebase[ch], ch);
    }
}

/*============================================================================
 * 批量操作宏（简化常见模式）
 *============================================================================*/

/**
 * @brief 批量初始化能量模块
 * @usage DB_BATCH_INIT_ENERGY(pDB, num_channels, db_energy_init, 
 *                              db_energy_get_realtime_main, 
 *                              db_energy_get_realtime_backup);
 */
#define DB_BATCH_INIT_ENERGY(pDB, num_channels, init_func, get_main, get_backup) \
    db_init_all_channels(pDB, num_channels, init_func, get_main, get_backup)

/**
 * @brief 批量销毁能量模块
 */
#define DB_BATCH_DESTROY_ENERGY(pDB, num_channels, destroy_func) \
    db_destroy_all_channels(pDB, num_channels, destroy_func)

/**
 * @brief 批量更新配置并广播事件
 * @note 对于签名为 (pDB, uint8_t channel) 的更新函数
 * 
 * @example
 *   DB_BATCH_UPDATE_AND_EVENT(pDB, num_channels, ch,
 *       db_maxdmd_default(&pDB->db_measurebase[ch], ch);
 *   , DB_MSR_EVENT_TYPE_MAXDMD_DEFAULT);
 */
#define DB_BATCH_UPDATE_AND_EVENT(pDB, num_channels, ch_var, update_code, event_type) \
    do { \
        FOR_EACH_CHANNEL(num_channels, ch_var) { \
            update_code \
            db_msr_msg_event_cmd(ch_var, event_type); \
        } \
    } while(0)

/**
 * @brief 批量广播事件
 */
#define DB_BATCH_BROADCAST_EVENT(num_channels, event_type) \
    db_broadcast_event_to_all_channels(num_channels, event_type)

#ifdef __cplusplus
}
#endif

#endif /* __DB_CHANNEL_HELPER_H__ */

