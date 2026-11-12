/**
 * @file        rn7302_read.h
 * @brief       RN7302 读取
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   RN7302 读取
 *  2026-10-29  yansh    1.0.1   声明 full/fund/half；对齐 fund 字段名
 */
#ifndef RN7302_READ_H
#define RN7302_READ_H

#include "msr_input.h"

/**
 * @brief       读取全波寄存器到 pIn->full
 * @return      0 成功；-1 失败
 */
int get_rn7302_register_msr_full(struct msr_input *pIn);

/**
 * @brief       读取基波寄存器到 pIn->fund
 * @return      0 成功；-1 失败
 */
int get_rn7302_register_msr_fund(struct msr_input *pIn);

/**
 * @brief       读取半波寄存器到 pIn->half
 * @return      0 成功；-1 失败
 */
int get_rn7302_register_msr_half(struct msr_input *pIn);

/**
 * @brief       读取全波寄存器到 pIn->full
 * @return      0 成功；-1 失败
 */
int get_rn7302_register_eng_full(struct msr_input *pIn);

/**
 * @brief       读取基波寄存器到 pIn->fund
 * @return      0 成功；-1 失败
 */
int get_rn7302_register_eng_fund(struct msr_input *pIn);

#endif
