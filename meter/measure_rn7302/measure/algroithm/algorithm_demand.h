/**
 * @file        algorithm_demand.h
 * @brief       需量计算（实时 + 日/月最值）
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   需量计算
 */
#ifndef ALGORITHM_DEMAND_H
#define ALGORITHM_DEMAND_H

#include <stdint.h>
#include <time.h>
#include "msr_config.h"
#include "msr_data.h"

/**
 * @brief   初始化需量状态与配置
 *
 * @param[out] st      需量跨拍状态（清零累加缓冲）
 * @param[in]  window  周期（分钟）；>MSR_DMD_WINDOW_MAX 时钳位
 * @param[in]  mode    需量方式
 * @param[in]  slip    滑差
 *
 * @return 0 成功；st==NULL 返回 -1
 */
int32_t algorithm_demand_init(struct msr_demand_state *st,
                        uint16_t window,
                        uint16_t mode,
                        uint16_t slip);

/**
 * @brief   清零实时需量结果
 *
 * @param[out] out  实时需量
 *
 * @return 0 成功；out==NULL 返回 -1
 */
int32_t algorithm_demand_clear(struct msr_demand *out);

/**
 * @brief   需量每秒处理
 *
 * 每秒累加全波 realtime；当分钟对齐 window 且秒==0 时输出周期平均并清缓冲。
 * now / last_* 由 st 维护，产品侧每秒调一次即可。
 *
 * @param[in,out] out  周期到时更新实时需量
 * @param[in,out] st   累加缓冲与时间状态
 * @param[in]     rt   本秒全波 realtime（电流/功率）
 * @param[in]     now  当前 Unix 时间
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_demand_process(struct msr_demand *out,
                           struct msr_demand_state *st,
                           const struct msr_data_realtime *rt,
                           time_t now);

/**
 * @brief   用实时需量刷新日/月最大需量
 *
 * 正向电流/视在取更大；有功/无功正反向分别按绝对值更大更新。
 *
 * @param[in,out] max   日/月最大需量
 * @param[in]     dmd   刚刷新的实时需量
 * @param[in]     now   时间戳（写入最值 time）
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_demand_max_update(struct msr_demand_max *max,
                              const struct msr_demand *dmd,
                              uint32_t now);

/**
 * @brief   清零日最大需量（时间戳置为 now）
 */
int32_t algorithm_demand_max_day_clear(struct msr_demand_max *max, uint32_t now);

/**
 * @brief   清零月最大需量（时间戳置为 now）
 */
int32_t algorithm_demand_max_month_clear(struct msr_demand_max *max, uint32_t now);

#endif
