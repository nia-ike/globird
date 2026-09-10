/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_currentangle.c
*  文件标识：
*  描    述：
*
*  当前版本：V1.0
*  作    者：raoy
*  运行环境：
*  操作系统: linux
*  硬件平台：f1c100s
*  固件库  ：
*  修改记录
*  作者       时间         版本      修改描述
*  raoy      2021       V1.00
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "Rn7302.h"
#include "Rn7302File.h"
#include "Rn7302_register.h"
#include "algorithm_currentangle.h"
#include "algorithm_voltageangle.h"


#define ANGLE_MULTIPLE              100
#define ANGLE_J360_MAX              (360 * ANGLE_MULTIPLE)
#define ANGLE_HALF_TURN             (180 * ANGLE_MULTIPLE)

/*********************************************************************************************************
** Function name:       current_angle_abs_u32
** Descriptions:        取电流幅值（用于相角有效性判断）
*********************************************************************************************************/
static uint32_t current_angle_abs_u32(int32_t i)
{
    return (i < 0) ? (uint32_t)(-i) : (uint32_t)i;
}

/*********************************************************************************************************
** Function name:       current_angle_norm_360
** Descriptions:        相角归一化到 [0, 36000)
*********************************************************************************************************/
static int32_t current_angle_norm_360(int32_t angle)
{
    while (angle < 0)
    {
        angle += ANGLE_J360_MAX;
    }
    while (angle >= ANGLE_J360_MAX)
    {
        angle -= ANGLE_J360_MAX;
    }
    return angle;
}

/*********************************************************************************************************
** Function name:       current_angle_by_phase_adjust
** Descriptions:        按 PhaseAdjust 从物理通道相角中选取逻辑相相角
**                      PhaseAdjust 低 3 位：1/2/3 → 物理 A/B/C
**                      bit15(0x8000)：CT 极性反，相角 +180°
*********************************************************************************************************/
static int32_t current_angle_by_phase_adjust(const int32_t yi_phys[3], uint16_t phase_adj, uint32_t i_mag)
{
    uint16_t ch;
    int32_t angle;

    ch = (uint16_t)(phase_adj & 0x07U);
    if ((ch < 1U) || (ch > 3U))
    {
        ch = 1U;
    }

    angle = yi_phys[ch - 1U];

    /* CT 反向：电流相角偏移 180° */
    if ((phase_adj & 0x8000U) != 0U)
    {
        angle = current_angle_norm_360(angle + ANGLE_HALF_TURN);
    }

    return angle_calc(i_mag, angle);
}

/*********************************************************************************************************
** Function name:       rms_algorithm_CurrentAngle
** Descriptions:        电流相角；相序调整后按 PhaseAdjust 重映射物理通道相角
**
** input parameters:
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_currentangle(struct db_database_measure_base_type  *pMeasureBase, struct db_basic_realtime_type *out,  void *input, uint32_t pt, uint32_t ct)
{
    struct rn7302_emu *pEmu = (struct rn7302_emu *)input;
    struct db_cfg_user_para *pCfgUser = &(pMeasureBase->db_cfg.user);
    /* 相角算在电流之前，幅值取上一拍基波电流做零值判断 */
    struct db_basic_realtime_type *pFund = &(pMeasureBase->db_measure.numeric.fundwave.primary);
    uint16_t wireMode = (pCfgUser->base.WiringMode.value & 0x03);
    uint16_t adj1 = pCfgUser->base.PhaseAdjust1.value;
    uint16_t adj2 = pCfgUser->base.PhaseAdjust2.value;
    uint16_t adj3 = pCfgUser->base.PhaseAdjust3.value;
    int32_t  yi_phys[3];
    int      phase_adjusted;

    (void)pt;
    (void)ct;

    /* 物理通道基波相角（芯片 YIA/YIB/YIC） */
    yi_phys[0] = angle_emu_calc(pEmu->YIA);
    yi_phys[1] = angle_emu_calc(pEmu->YIB);
    yi_phys[2] = angle_emu_calc(pEmu->YIC);

    /* 默认 1/2/3 且无极性位 → 无相序调整 */
    phase_adjusted = ((adj1 != 1U) || (adj2 != 2U) || (adj3 != 3U) ||
                      ((adj1 | adj2 | adj3) & 0x8000U) != 0U);

    switch (wireMode)
    {
    case P3W3:
    case P3W4:
        if (phase_adjusted)
        {
            out->J_Ia = current_angle_by_phase_adjust(yi_phys, adj1, current_angle_abs_u32(pFund->Ia));
            out->J_Ib = current_angle_by_phase_adjust(yi_phys, adj2, current_angle_abs_u32(pFund->Ib));
            out->J_Ic = current_angle_by_phase_adjust(yi_phys, adj3, current_angle_abs_u32(pFund->Ic));
        }
        else
        {
            out->J_Ia = angle_calc(current_angle_abs_u32(pFund->Ia), yi_phys[0]);
            out->J_Ib = angle_calc(current_angle_abs_u32(pFund->Ib), yi_phys[1]);
            out->J_Ic = angle_calc(current_angle_abs_u32(pFund->Ic), yi_phys[2]);
        }
        break;

    default:
        return -1;
    }

    return 0;
}
