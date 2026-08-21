
/*
* Copyright (c) 2019,珠海科技股份有限公司
* All rights reserved.
* 
* 文件名称：algorithm_phasecheck.h
* 文件标识：
* 描    述：
* 
* 当前版本：V1.0
* 作    者：raoy
* 更改说明：
* 完成日期：20190425
* 运行环境：
操作系统: 
硬件平台：
固件库  ：
* 修改记录 
*  作者       时间         版本      修改描述
*  raoy   2019.04.25       V1.00      创建
*/


#ifndef _ALGORITHM_PHASECHECK_H_
#define _ALGORITHM_PHASECHECK_H_

#include <stdint.h>

#define PHASE_FLAG_MASK                         (1 << 15)
#define PHASE_CHNL_MASK                         ((1 << 0) | (1 << 1))


//#define P3W4        (0x00)
//#define P3W3        (0x01)
#define VOL_LOSS_VALUE      3000    //电压缺相判断值
#define CURR_LOSS_VALUE    100      //电流缺相判断值
#define ANGLE_VALUE_MULT   100 //角度值扩大倍数
//以上为应用参数，可根据需要修改
//--------------------end---------------------
#define NO_CHK_WIRING_ERR   -1   //未找到对应查线状态标志
#define PHASE_LOSS_NO       0              //没有缺相
#define WIRING_OK           0              //接线正确

#define SINGLE_PHASE_MODE   0XFFFD   //单相模式
#define PHASE_LOSS_ERR      0XFFFE      //缺相错误标志
#define PARA_ERR            0XFFFF      //参数传入NULL标志

#define ALARM_VOLTAGE_LOST_BIT      0x0004
/********************************相序调整定义 *****************************************/
typedef enum 
{
    AD_PHASE_PHY_I1 =1 ,                                   /*当前电流通道I映射到的I1*/
    AD_PHASE_PHY_I2 ,                                       /*当前电流通道I映射到的I2*/
    AD_PHASE_PHY_I3                                         /*当前电流通道I映射到的I3*/
}AD_PhaseCmdTypedef;

typedef struct
{
    AD_PhaseCmdTypedef Ia;
    AD_PhaseCmdTypedef Ib;
    AD_PhaseCmdTypedef Ic;      
}AD_PhaseSetTypedef;

struct TypeCheckWiringInputPara
{
    uint16_t mode;       //接线模式
    int16_t  VaAngle;    //Va基波角度
    int16_t  VbAngle;    //Vb基波角度
    int16_t  VcAngle;    //Vc基波角度
    int16_t  IaAngle;    //Ia基波角度
    int16_t  IbAngle;    //Ib基波角度
    int16_t  IcAngle;    //Ic基波角度
};


int32_t rms_algorithm_AutoPhaseAdjust(struct db_database_measure_base_type *pMeasureBase, int num);
void rms_algorithm_HandPhaseAdjust(struct db_database_measure_base_type *pMeasureBase);
int32_t power_adjust(struct db_database_measure_base_type *pMeasureBase);


#endif
