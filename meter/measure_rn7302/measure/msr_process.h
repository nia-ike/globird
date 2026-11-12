/**
 * @file        msr_process.h
 * @brief       测量处理
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   测量处理
 */
#ifndef MSR_PROCESS_H
#define MSR_PROCESS_H

#include <stdint.h>
#include "msr_config.h"
#include "msr_data.h"
#include "msr_input.h"

/**
 * @brief       测量处理
 * @param[in]    cfg     配置
 * @param[in,out] data   数据
 * @param[in]    in      输入
 * @return      0 成功；-1 失败
 */
int msr_process_fullfond(struct msr_config *cfg, struct msr_data *data, struct msr_input *in);

/**
 * @brief       半波测量处理
 * @param[in]    cfg     配置
 * @param[in,out] data   数据
 * @param[in]    in      输入
 * @return      0 成功；-1 失败
 */
int msr_process_halfwave(struct msr_config *cfg, struct msr_data *data, struct msr_input *in);

#endif
