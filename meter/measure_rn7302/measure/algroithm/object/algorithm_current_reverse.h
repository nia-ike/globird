/**
 * @file        algorithm_current_reverse.h
 * @brief       CT 反向时翻转有功 / 无功 / 功因符号
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   电流反向
 */
#ifndef ALGORITHM_CURRENT_REVERSE_H
#define ALGORITHM_CURRENT_REVERSE_H

#include <stdint.h>
#include "msr_config.h"
#include "msr_data.h"

/**
 * @brief   CT 反向：按 phase_adjust bit15 翻转对应相 P/Q/PF
 *
 * 仅 WIREMODE_P3W4 生效；非三线四线直接返回 0。
 * 翻转后重算 Ptot/Qtot；若任一相反向且 Stot≠0，按 Ptot/Stot 重算 PFtot。
 * 半波相序关闭路径在功因之后调用；相序开启路径角已含极性，不再调用。
 *
 * @param[in,out] out  读写功率与功因
 * @param[in]     cfg  wiremode、phase_adjust[3]
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t alg_current_reverse(struct msr_data_realtime *out,
                            const struct msr_config *cfg);

#endif
