/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_multirate.h
*  文件标识：
*  描    述:复费率
*
*  当前版本：V1.0
*  作    者：raoy
*  运行环境：
*  操作系统: linux
*  硬件平台：f1c100s
*  固件库  ：
*  修改记录
*  作者       时间         版本      修改描述
*  raoy   2021       V1.00
*/

#include "db_struct.h"
#include "db_cfg_user_multirate.h"


#ifndef _ALGORITHM_ENERGY_MULTIRATE_HP_H_
#define _ALGORITHM_ENERGY_MULTIRATE_HP_H_




/*********************************************************************************************************
** Function name:       rms_algorithm_RealtimeEnergy
** Descriptions:        计量复费率电度
** input parameters:    in      - some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object  - 
                        out     - realtime data struct
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
int32_t rms_algorithm_MultirateEnergy_hp(struct db_database_measure_base_type *pMeasureBase, struct db_energy_multirate_type *out, void *pNet, uint32_t pt, uint32_t ct);



#endif
