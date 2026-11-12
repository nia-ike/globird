/**
 * @file        port_database.h
 * @brief       measure_rn7302 ↔ database 配置对接
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   端口数据库
 */
#ifndef PORT_DATABASE_H
#define PORT_DATABASE_H

#include <stdint.h>
#include "msr_config.h"
#include "msr_data.h"
#include "db_struct.h"

/**
 * @brief   将 database 回路配置拷贝到 measure 侧 msr_config
 *
 * @param[out] cfg       measure 配置
 * @param[in]  pMeasure  database 单回路（含 user/sys 配置）
 *
 * @return 0 成功；参数为空返回 -1
 */
int port_db_copy_config_to_measure(struct msr_config *cfg,
                                   const struct db_database_measure_base_type *pMeasure);

/**
 * @brief   将 measure 电参结果写回 database 共享内存
 *
 * @param[in]  data      measure 侧 full/fund/half
 * @param[out] pMeasure  database 单回路
 *
 * @return 0 成功；参数为空返回 -1
 */
int port_db_copy_data_to_database(const struct msr_data *data,
                                  struct db_database_measure_base_type *pMeasure);

#endif
