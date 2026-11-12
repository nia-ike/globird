/**
 * @file        algorithm_energy.h
 * @brief       电能计算（余数 / 实时 / 四象限；全波与基波共用叶子）
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   电能计算
 *  2026-10-28  yansh    1.0.1   合并四象限
 *
 * 运行电能 / translate / 复费率仍由产品侧处理。
 */
#ifndef ALGORITHM_ENERGY_H
#define ALGORITHM_ENERGY_H

#include <stdint.h>
#include "msr_config.h"
#include "msr_data.h"
#include "msr_input.h"

/**
 * @brief   电能余数处理，写出本拍净增量 eng->net
 *
 * 对各通道：脉冲×PT×CT 累加到 remain，满一格进位写入 net。
 * 对应相功率为 0、脉冲常数为 0 或脉冲异常偏大时本通道 net=0。
 * P3W4 下合相净增量由三相之和重算。
 *
 * @param[in,out] eng  全波或基波各持有一份电能状态
 * @param[in]     cfg  pt/ct/wiremode/pulse_constant
 * @param[in]     in   芯片电能增量快照（PosEP、NegEP、ES 等字段）
 * @param[in]     pwr  本拍功率，作计度门控（P/Q/S==0 不计）
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_energy_remain(struct msr_energy_realtime *eng,
                          const struct msr_config *cfg,
                          const struct msr_input_energy *in,
                          const struct msr_data_realtime *pwr);

/**
 * @brief   高精度实时电能累加（8+3 位）
 *
 * 将 eng->net 累加到 eng->realtime_hp，按 MSR_ENERGY_MAX_HP 回绕；
 * P3W4 合相由三相之和重算。
 *
 * @param[in,out] eng  读写 net / realtime_hp
 * @param[in]     cfg  接线模式
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_energy_realtime_hp(struct msr_energy_realtime *eng,
                               const struct msr_config *cfg);

/**
 * @brief   通讯实时电能（8+1 位）
 *
 * eng->realtime = eng->realtime_hp / 100，按 MSR_ENERGY_MAX_8D1 回绕；
 * P3W4 合相由三相之和重算。
 *
 * @param[in,out] eng  读 realtime_hp，写 realtime
 * @param[in]     cfg  接线模式
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_energy_realtime(struct msr_energy_realtime *eng,
                            const struct msr_config *cfg);

/**
 * @brief   四象限电能累加（8+1）
 *
 * 按本拍各相有功/无功符号划分象限，将 net 对应方向增量（÷100）累加到
 * 该相象限桶；合相按合相功率符号划分。
 *
 * 象限：Q1(+P+Q) Q2(-P+Q) Q3(-P-Q) Q4(+P-Q)
 *
 * @param[in,out] out  四象限电能
 * @param[in]     net  本拍净增量（与 realtime_hp 同精度）
 * @param[in]     pwr  本拍功率（Pa/Qa… 定象限）
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_energy_fourquadrant(struct msr_energy_fourquadrant *out,
                                const struct msr_energy_channels *net,
                                const struct msr_data_realtime *pwr);

/**
 * @brief   多费率电能累加
 *
 * @param[in,out] out  多费率电能
 * @param[in]     net  本拍净增量
 * @param[in]     data  数据
 */
int32_t algorithm_energy_multirate(struct msr_energy_multirate *out,
                                const struct msr_energy_channels *net,
                                const struct msr_data *data);
#endif
