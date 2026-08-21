/*
* Copyright (c) 2015,珠海科技股份有限公司
* All rights reserved.
* 
* 文件名称：db_realtime.h
* 文件标识：
* 描    述：
* 
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20190909
* 运行环境：
操作系统: rt-thread
硬件平台：STM32F030
固件库  ：V3.5
* 修改记录 
*  作者       时间         版本      修改描述
*  chenzy   2014.11.26       V1.00      创建
*/

#ifndef __DB_MEASURE_H__
#define __DB_MEASURE_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "db_measure_realtime.h"
#include "db_measure_energy.h"
#include "db_measure_statistical.h"
#include "db_measure_powerquality.h"



struct db_database_measure_type
{
    /* 
        数值类型数据：
            1、基本测量参数：   电压、电流、功率、功因、频率、角度全波值。
            2、基波测量参数：   电压、电流、中性线电压/电流、角度基波值。
    */
    struct db_measure_numeric_type          numeric;

    /*
        电能类型数据：
            1、双向电能：       有功/无功/视在的输入、输出、总电能及电能净值。
            2、四象限电能：     三相有功/无功的四象限电能。
            3、复费率电能：     基本复费率电能。
            4、历史电能：       基本历史电能、复费率历史电能。
            5、碳排放电能：     三相/总的有功/无功/视在的输入/输出电能。
            6、负载运行电能：   负载运行电能 
    */
    struct db_measure_energy_type           energy;

    /*
        统计类型数据：
            1、需量：          电流/有功功率实时需量、预测需量及需量最大值。
            2、极值：          最近31日、12月的电压、电流...的最大最小值。
            3、负载运行参数：   负载上电/运行时间和运行次数。
    */
    struct db_measure_statistical_type      statistical;

    /*
        电能质量类型数据：
            1、基本参数：       不平衡率、序分量....
            2、间谐波数据：     2~63次电压/电流含有率、有效值
            3、谐波数据：       总畸变率、2~63次电压/电流/功率角度/含有率/有效值
            4、评估数据：       电压暂升/暂降/中断/瞬态/快速、闪变严重度、电压变化...
    */
    struct db_measure_powerquality_type     powerquality;
};




#endif
