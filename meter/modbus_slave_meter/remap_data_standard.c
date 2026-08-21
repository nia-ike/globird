/*
* Copyright (c) 2019,珠海科技股份有限公司
* All rights reserved.
* 
* 文件名称：modbusconfig.c
* 文件标识：
* 描    述：
* 
* 当前版本：V1.0
* 作    者：wangcq
* 更改说明：
* 完成日期：20190530
* 运行环境：
操作系统: RT-Thread
硬件平台：STM32F030
固件库  ：V3.1.2
* 修改记录 
*  作者       时间         版本      修改描述
*  wangcq   2019.05.30       V1.00      创建
*  chenzy   2019.09.18       V1.01      更改寄存器段配置方式
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#include "cJSON.h"
#include "cJSON_Utils.h"

#include "i2c.h"
#include "mb_reg_remap_standard.h"
#include "db_device.h"
#include "db_struct.h"
#include "db_msgevent_api.h"
#include "db_msr_msgevent_api.h"
#include "db_cfg_user_multirate.h"
#include "remap_data_standard.h"
#include "mb_cfg.h"
#include "mb_com_baseclass.h"
#include "ota_http.h"
#include "mb_reg_remap_meter1.h"
#include "mb_reg_remap_meter2.h"
#include "mb_reg_remap_meter3.h"

#define MB_ALIGN_PT_16BIT_READ(p)       (*((uint8_t *)(p)) << 8 | *((uint8_t *)(p)+1))
#define MB_ALIGN_PT_16BIT_WRITE(p,v)    do {(*((uint8_t*)p)) = v >> 8 & 0xff; (*((uint8_t*)p+1)) = v & 0xff;}while(0)
#define MB_ALIGN_PT_32BIT_WRITE(p,v)    do {(*((uint8_t*)p)) = v >> 24 & 0xff; (*((uint8_t*)p+1)) = v >> 16 & 0xff; (*((uint8_t*)p+2)) = v >> 8 & 0xff; (*((uint8_t*)p+3)) = v  & 0xff;}while(0)

#define MB_WRITE_PASSWORD   700
#define MB_WRITE_CMD        888

#define MB_READ_ENABLE (1 << 0)
#define MB_WRITE_ENABLE (1 << 1)
#define MB_PASSWORD_ENABLE (1 << 2)

enum
{
    RO = MB_READ_ENABLE,
    WO = MB_WRITE_ENABLE,
    RW_NP = MB_READ_ENABLE | MB_WRITE_ENABLE,
    RW_P = MB_READ_ENABLE | MB_WRITE_ENABLE | MB_PASSWORD_ENABLE
};

enum
{
    MB_TYPE_RAM = 0,
    MB_TYPE_ROM,
    MB_TYPE_CMD,
    MB_TYPE_LOCAL,
    MB_TYPE_CUSTOM,
};

#define MB_SECTION_GET_REG_NUM(x)                sizeof(x)/sizeof(struct remap_Modbus_node)

/*
    function declear
*/
static int mb_cmd_adjust_offset_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_clearAllCfg_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_clearSysCfg_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_clearUserCfg_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_clearMaxDmd_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cfgsys_deadzone_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_clearE_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_historyE_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cfguser_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cfgsys_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_demo_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_synctime_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cfgusermultirage_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_clearMultirateE_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_clearLoadRun_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_reboot_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_boot_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_clearAllUser_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_shadow_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);

// new
static __inline int mb_cmd_adjust_handle_meter1(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static __inline int mb_cmd_adjust_handle_meter2(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static __inline int mb_cmd_adjust_handle_meter3(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);

static int mb_cmd_clearSoe_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);

static uint16_t *mb_productTest_info_handle (struct mb_comm_ctx_t *pCtx, uint16_t regaddr, uint16_t regnum, uint8_t *p_data);
static int mb_cmd_test_envset_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_test_lcd_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_test_led_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_test_set_sn_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig,  uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_test_set_model_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig,  uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_test_set_mac_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig,  uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cfguser_new_multirate_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_block_node *pModifyConfig, uint16_t orgin_regaddr, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_clear_runing_energy_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_timezone_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);

static int mb_cmd_rn7302_demo_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);

static int mb_cmd_clearE2_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_historyE2_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_clearMultirateE2_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);


static int mb_cmd_adjustRst_handle_meter1(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_adjustRst_handle_meter2(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_adjustRst_handle_meter3(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);
static int mb_cmd_adjustRst_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);

static int mb_cmd_ota_set_url_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig,  uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value);

/* 
    modbus RTU 寄存器表
    注意，为了检索方便，请按使用的频率高低由高到低进行依次排列 
    
*/



/*
    modbus寄存器分区设计： 本段包含全功能电表的寄存器分区（克隆寄存器由  另映射寄存器进行定义）
    0-9999：         长度 10000个寄存器。 预留。 可能用于将来扩展用途
    10000-19999：    电表1寄存器段，长度 5000个寄存器。
                     offset
                     0 ~ 199:    remapInfo_meter1               电表信息段，比如电表型号ID，版本号，序列号，变比等信息 
                     200 ~ 599:  remapAggregation_meter1        电表聚合数据段（快速段）。比如 常规电表数据：电压、电流、功率、功率因数、频率、需量、电能等
                     600 ~ 999:  remapRealTimeFundwave_meter1   基波段
                     1000 ~ 7999: 电表数据段。比如 分段详细数据段
                                  1000 - 1199：remapSeg_realtime_meter1            电表详细数据
                                  1200 - 1299：remapRealTimeDmd_meter1             电表需量
                                  1300 - 1499：remapMaxDmdDay_meter1               电表最大需量日数据
                                  1500 - 1699：remapMaxDmdMonth_meter1             电表最大需量月数据
                                  1700 - 1899: remapRealTimeEnergyhp_meter1        电表电能
                                  1900 - 2099: remapEnergyMultirate_meter1         电表复费率电能
                                  2100 - 2399: remapEnergyFourquadrant_meter1      电表四象限电能
                                  2400 - 2699: remapEnergyFundwave                 基波电能
                                  2700 - 2999：remapEnergyRuning                   负载运行电能
                                  3000 - 3399：remapExtremeDataDay_meter1           日极值
                                  3400 - 3799：remapExtremeDataMonth_meter1         月极值
                                  3800 - 3849：remapPowerqualityHarmonicThd_meter1  谐波总畸变率
                                  3850 - 3899：remapPowerqualityHarmonicAngle_meter1 谐波角度
                                  3900 - 4299：remapPowerqualityHarmonicHr_meter1   谐波含有率
                                  4300 - 5099：remapPowerqualityHarmonicRms_meter1  谐波有效值
                                  
                     8000 - 9799: 配置数据
                                  8000 - 8299: remapUserBaseCfg_meter1            电表用户配置数据
                                  8300 - 8339：remapSysAdjustAdCfg_meter1         电表系统校表配置重映射 (校正寄存器)
                                  8340 - 8379：remapSysAdjustCfg_meter1           电表系统校表配置重映射 (校正寄存器)
                                  8380 - 8419：remapSysPrthCfg_meter1             电表系统参数，配置电参量的放大倍数
                                  8420 - 8459：remapSysCfgAdjustAdValue_meter1    相位角补偿系数
                                  8460 - 8499：remapSysDeadZoneCfg_meter1         电表系统死区配置重映射
                                  8500 - 8539：remapSysInterfaceCfg_meter1        电表系统额定参数配置重映射

                    9800 - 9999:  命令
                                  9800       : 校表

    20000-29999：    电表2寄存器段，长度 10000个寄存器。
    30000-39999：    电表3寄存器段，长度 10000个寄存器。
    40000-49999：    电表4寄存器段，长度 10000个寄存器。
    

    50000-64999：    预留，长度 15000个寄存器。

    65000-65535：    公共寄存器段，长度 535个寄存器。


*/

static const ST_REG_4XXXX_ATTR   reg_4x_section[] = {

    /*       存储位置基址                                                   寄存器起始位置          寄存器个数                                         类型        读写权限     read回调    write 回调*/
    /* METER 1 */
    { (struct remap_Modbus_node*)&remapInfo_meter1[0],                       10000,     MB_SECTION_GET_REG_NUM(remapInfo_meter1),                   MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapAggregation_meter1[0],                10200,     MB_SECTION_GET_REG_NUM(remapAggregation_meter1),            MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapRealTimeFundwave_meter1[0],           10600,     MB_SECTION_GET_REG_NUM(remapRealTimeFundwave_meter1),       MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapSeg_realtime_meter1[0],               11000,     MB_SECTION_GET_REG_NUM(remapSeg_realtime_meter1),           MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapRealTimeDmd_meter1[0],                11200,     MB_SECTION_GET_REG_NUM(remapRealTimeDmd_meter1),            MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
              /* 最大需量 日月 在block中，分别为 11300 11500  */     
    { (struct remap_Modbus_node*)&remapRealTimeEnergyhp_meter1[0],           11700,     MB_SECTION_GET_REG_NUM(remapRealTimeEnergyhp_meter1),       MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
              /* 最值 在block中，分别为 13000 13400 */
              /* 谐波 在block中，分别为 13800 13850 13900 14300 */
        /* 配置数据 */
    { (struct remap_Modbus_node*)&remapUserBaseCfg_meter1[0],                18000,     MB_SECTION_GET_REG_NUM(remapUserBaseCfg_meter1),            MB_TYPE_RAM,      RW_NP ,  NULL,    mb_cfguser_handle}, // 
    { (struct remap_Modbus_node*)&remapSysAdjustCfg_meter1[0],               18300,     MB_SECTION_GET_REG_NUM(remapSysAdjustCfg_meter1),           MB_TYPE_RAM,      RW_P ,   NULL,    mb_cfgsys_handle}, // 
    { (struct remap_Modbus_node*)&remapSysAdjustAdCfg_meter1[0],             18340,     MB_SECTION_GET_REG_NUM(remapSysAdjustAdCfg_meter1),         MB_TYPE_RAM,      RW_P ,   NULL,    mb_cfgsys_handle}, // system data
    { (struct remap_Modbus_node*)&remapSysPrthCfg_meter1[0],                 18380,     MB_SECTION_GET_REG_NUM(remapSysPrthCfg_meter1),             MB_TYPE_RAM,      RW_P ,   NULL,    mb_cfgsys_handle}, // system data
    { (struct remap_Modbus_node*)&remapSysCfgAdjustAdValue_meter1[0],        18420,     MB_SECTION_GET_REG_NUM(remapSysCfgAdjustAdValue_meter1),    MB_TYPE_RAM,      RW_P ,   NULL,    mb_cfgsys_handle}, // 校表,兼容多种电流互感器
    { (struct remap_Modbus_node*)&remapSysDeadZoneCfg_meter1[0],             18460,     MB_SECTION_GET_REG_NUM(remapSysDeadZoneCfg_meter1),         MB_TYPE_RAM,      RW_P ,     NULL,    mb_cfgsys_handle}, // 
    { (struct remap_Modbus_node*)&remapSysInterfaceCfg_meter1[0],            18500,     MB_SECTION_GET_REG_NUM(remapSysInterfaceCfg_meter1),        MB_TYPE_RAM,      RO ,     NULL,    mb_cfgsys_handle}, // 
    
    /* 命令 */
    { (struct remap_Modbus_node*)&remapCommandAdjust_meter1[0],              19800,     MB_SECTION_GET_REG_NUM(remapCommandAdjust_meter1),          MB_TYPE_RAM,      RW_P ,   NULL,    mb_cmd_adjust_handle_meter1}, // 校表,兼容多种电流互感器
    { NULL,                                                                  19009,     1,                                                          MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_adjustRst_handle_meter1}, // 清rn7302校表系数                                                                                                                      

    
    /* METER 2 */           
    { (struct remap_Modbus_node*)&remapInfo_meter2[0],                       20000,     MB_SECTION_GET_REG_NUM(remapInfo_meter2),                   MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapAggregation_meter2[0],                20200,     MB_SECTION_GET_REG_NUM(remapAggregation_meter2),            MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapRealTimeFundwave_meter2[0],           20600,     MB_SECTION_GET_REG_NUM(remapRealTimeFundwave_meter2),       MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapSeg_realtime_meter2[0],               21000,     MB_SECTION_GET_REG_NUM(remapSeg_realtime_meter2),           MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapRealTimeDmd_meter2[0],                21200,     MB_SECTION_GET_REG_NUM(remapRealTimeDmd_meter2),            MB_TYPE_RAM,      RO ,     NULL,    NULL}, //         
              /* 最大需量 日月 在block中，分别为 21300 21500  */     
    { (struct remap_Modbus_node*)&remapRealTimeEnergyhp_meter2[0],           21700,     MB_SECTION_GET_REG_NUM(remapRealTimeEnergyhp_meter2),       MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapEnergyMultirate_meter2[0],            21900,     MB_SECTION_GET_REG_NUM(remapEnergyMultirate_meter2),        MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
              /* 最值 在block中，分别为 23000 23400 */
              /* 谐波 在block中，分别为 23800 23850 23900 24300 */
        /* 配置数据 */
    { (struct remap_Modbus_node*)&remapUserBaseCfg_meter2[0],                28000,     MB_SECTION_GET_REG_NUM(remapUserBaseCfg_meter2),            MB_TYPE_RAM,      RW_NP ,     NULL,    mb_cfguser_handle}, // 
    { (struct remap_Modbus_node*)&remapSysAdjustCfg_meter2[0],               28300,     MB_SECTION_GET_REG_NUM(remapSysAdjustCfg_meter2),           MB_TYPE_RAM,      RO ,     NULL,    mb_cfgsys_handle}, // 
    { (struct remap_Modbus_node*)&remapSysAdjustAdCfg_meter2[0],             28340,     MB_SECTION_GET_REG_NUM(remapSysAdjustAdCfg_meter2),         MB_TYPE_RAM,      RW_P ,   NULL,    mb_cfgsys_handle}, // system data
    { (struct remap_Modbus_node*)&remapSysPrthCfg_meter2[0],                 28380,     MB_SECTION_GET_REG_NUM(remapSysPrthCfg_meter2),             MB_TYPE_RAM,      RW_P ,   NULL,    mb_cfgsys_handle}, // system data
    { (struct remap_Modbus_node*)&remapSysCfgAdjustAdValue_meter2[0],        28420,     MB_SECTION_GET_REG_NUM(remapSysCfgAdjustAdValue_meter2),    MB_TYPE_RAM,      RW_P ,   NULL,    mb_cfgsys_handle}, // 校表,兼容多种电流互感器
    { (struct remap_Modbus_node*)&remapSysDeadZoneCfg_meter2[0],             28460,     MB_SECTION_GET_REG_NUM(remapSysDeadZoneCfg_meter2),         MB_TYPE_RAM,      RW_P ,     NULL,    mb_cfgsys_handle}, // 
    { (struct remap_Modbus_node*)&remapSysInterfaceCfg_meter2[0],            28500,     MB_SECTION_GET_REG_NUM(remapSysInterfaceCfg_meter2),        MB_TYPE_RAM,      RO ,     NULL,    mb_cfgsys_handle}, // 
    /* 命令 */
    { (struct remap_Modbus_node*)&remapCommandAdjust_meter2[0],              29800,     MB_SECTION_GET_REG_NUM(remapCommandAdjust_meter2),          MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_adjust_handle_meter2}, // 校表
    { NULL,                                                                  29009,     1,                                                          MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_adjustRst_handle_meter2}, // 清rn7302校表系数                                                                                                                      
    
    /* METER 3 */           
    { (struct remap_Modbus_node*)&remapInfo_meter3[0],                       30000,     MB_SECTION_GET_REG_NUM(remapInfo_meter3),                   MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapAggregation_meter3[0],                30200,     MB_SECTION_GET_REG_NUM(remapAggregation_meter3),            MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapRealTimeFundwave_meter3[0],           30600,     MB_SECTION_GET_REG_NUM(remapRealTimeFundwave_meter3),       MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapSeg_realtime_meter3[0],               31000,     MB_SECTION_GET_REG_NUM(remapSeg_realtime_meter3),           MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapRealTimeDmd_meter3[0],                31200,     MB_SECTION_GET_REG_NUM(remapRealTimeDmd_meter3),            MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
              /* 最大需量 日月 在block中，分别为 11300 11500  */     
    { (struct remap_Modbus_node*)&remapRealTimeEnergyhp_meter3[0],           31700,     MB_SECTION_GET_REG_NUM(remapRealTimeEnergyhp_meter3),       MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_node*)&remapEnergyMultirate_meter3[0],            31900,     MB_SECTION_GET_REG_NUM(remapEnergyMultirate_meter3),        MB_TYPE_RAM,      RO ,     NULL,    NULL}, //
              /* 最值 在block中，分别为 31300 31700 */
              /* 谐波 在block中，分别为 33800 33850 33900 34300 */
        /* 配置数据 */
    { (struct remap_Modbus_node*)&remapUserBaseCfg_meter3[0],                38000,     MB_SECTION_GET_REG_NUM(remapUserBaseCfg_meter3),            MB_TYPE_RAM,      RW_NP ,     NULL,    mb_cfguser_handle}, // 
    { (struct remap_Modbus_node*)&remapSysAdjustCfg_meter3[0],               38300,     MB_SECTION_GET_REG_NUM(remapSysAdjustCfg_meter3),           MB_TYPE_RAM,      RO ,     NULL,    mb_cfgsys_handle}, // 
    { (struct remap_Modbus_node*)&remapSysAdjustAdCfg_meter3[0],             38340,     MB_SECTION_GET_REG_NUM(remapSysAdjustAdCfg_meter3),         MB_TYPE_RAM,      RW_P ,   NULL,    mb_cfgsys_handle}, // system data
    { (struct remap_Modbus_node*)&remapSysPrthCfg_meter3[0],                 38380,     MB_SECTION_GET_REG_NUM(remapSysPrthCfg_meter3),             MB_TYPE_RAM,      RW_P ,   NULL,    mb_cfgsys_handle}, // system data
    { (struct remap_Modbus_node*)&remapSysCfgAdjustAdValue_meter3[0],        38420,     MB_SECTION_GET_REG_NUM(remapSysCfgAdjustAdValue_meter3),    MB_TYPE_RAM,      RW_P ,   NULL,    mb_cfgsys_handle}, // 校表,兼容多种电流互感器
    { (struct remap_Modbus_node*)&remapSysDeadZoneCfg_meter3[0],             38460,     MB_SECTION_GET_REG_NUM(remapSysDeadZoneCfg_meter3),         MB_TYPE_RAM,      RW_P ,     NULL,    mb_cfgsys_handle}, // 
    { (struct remap_Modbus_node*)&remapSysInterfaceCfg_meter3[0],            38500,     MB_SECTION_GET_REG_NUM(remapSysInterfaceCfg_meter3),        MB_TYPE_RAM,      RO ,     NULL,    mb_cfgsys_handle}, // 
    /* 命令 */
    { (struct remap_Modbus_node*)&remapCommandAdjust_meter3[0],              39800,     MB_SECTION_GET_REG_NUM(remapCommandAdjust_meter3),          MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_adjust_handle_meter3}, // 校表
    { NULL,                                                                  39009,     1,                                                          MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_adjustRst_handle_meter3}, // 清rn7302校表系数                                                                                                                      

    /*
        公共寄存器段
        命令寄存器
    */
    { NULL,                                                      59001,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_clearE_handle}, // 清电能
    { NULL,                                                      59002,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_historyE_handle}, // 清电能
    { NULL,                                                      59003,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_clearMaxDmd_handle}, // 清最大需量
    { NULL,                                                      59004,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_clearUserCfg_handle}, // 清用户系数
    { NULL,                                                      59005,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_clearSysCfg_handle}, // 清系统系数
    { NULL,                                                      59006,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_clearAllCfg_handle}, // 清电能、用户系数、系统系数
    { NULL,                                                      59007,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    NULL}, // RN7302软件复位
    { NULL,                                                      59008,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    NULL}, // 自动相序调整
    { NULL,                                                      59009,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_adjustRst_handle}, // 清rn7302校表系数                                                                                                                      
    { NULL,                                                      59010,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_demo_handle}, // 清rn7302校表系数                                                                                                                      
    { NULL,                                                      59011,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_clearMultirateE_handle}, // 清电能
    { NULL,                                                      59012,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_clearSoe_handle}, // 清SOE事件记录
    { NULL,                                                      59014,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_clearAllUser_handle     }, // 恢复出厂设置(所有记录、电度、最值)
    { NULL,                                                      59015,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_clearLoadRun_handle     }, // 清除运行时间
    { NULL,                                                      59019,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_clear_runing_energy_handle     }, // 清除运行电能

    { NULL,                                                      59031,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_rn7302_demo_handle}, // 清rn7302校表系数                                                                                                                      

    { NULL,                                                      59050,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_reboot_handle         }, // reboot
    { NULL,                                                      59051,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    NULL         }, // boot
    { NULL,                                                      59052,     1,                                                MB_TYPE_CMD,      RW_P ,   NULL,    mb_cmd_adjust_offset_handle},

    { NULL,                                                      59067,      9,                                               MB_TYPE_CMD,      RW_P ,    NULL,    mb_cmd_test_set_mac_handle     }, // 写mac 
    { NULL,                                                      59076,      13,                                              MB_TYPE_CMD,      RW_P ,    NULL,    mb_cmd_test_set_model_handle     }, // 写型号
    { NULL,                                                      59089,      8,                                               MB_TYPE_CMD,      RW_P ,    NULL,    mb_cmd_test_set_sn_handle     }, // 写sn  
    { NULL,                                                      59097,      1,                                               MB_TYPE_CMD,      RW_P ,    NULL,    mb_cmd_test_led_handle}, // led 测试
    { NULL,                                                      59098,      1,                                               MB_TYPE_CMD,      RW_P ,    NULL,    mb_cmd_test_lcd_handle}, // lcd测试
    { NULL,                                                      59099,      1,                                               MB_TYPE_CMD,      RW_P ,    NULL,    mb_cmd_test_envset_handle     }, // 测试环境
    { NULL,                                                      59101,      1,                                               MB_TYPE_CMD,      RW_P ,    NULL,    mb_cmd_clearE2_handle}, // 清电能
    { NULL,                                                      59102,      1,                                               MB_TYPE_CMD,      RW_P ,    NULL,    mb_cmd_historyE2_handle}, // 清电能
    { NULL,                                                      59111,      1,                                               MB_TYPE_CMD,      RW_P ,    NULL,    mb_cmd_clearMultirateE2_handle}, // 清电能
    { NULL,                                                      57000,      100,                                             MB_TYPE_CMD,      RW_NP ,  NULL,    mb_cmd_ota_set_url_handle }, // 写http命令升级
    /*
        公共寄存器段
    */

    { (struct remap_Modbus_node*)&remapStatusInfo[0],           64000,      MB_SECTION_GET_REG_NUM(remapStatusInfo),         MB_TYPE_RAM,      RO ,     NULL,    NULL}, // device data
    { (struct remap_Modbus_node*)&remapTestInfo[0],             64200,      MB_SECTION_GET_REG_NUM(remapTestInfo),           MB_TYPE_RAM,      RO ,     mb_productTest_info_handle,    NULL}, // device data   
   
    { (struct remap_Modbus_node*)&remapSystemInfo[0],           64300,      MB_SECTION_GET_REG_NUM(remapSystemInfo),          MB_TYPE_LOCAL,    RW_NP ,  NULL,    mb_cmd_synctime_handle}, // 
    { (struct remap_Modbus_node*)&remapSystemTimeZone[0],       64302,      MB_SECTION_GET_REG_NUM(remapSystemTimeZone),      MB_TYPE_LOCAL,    RW_NP ,  NULL,    mb_cmd_timezone_handle}, // 
    
    { (struct remap_Modbus_node*)&remapDevInfo[0],              65000,      MB_SECTION_GET_REG_NUM(remapDevInfo),            MB_TYPE_RAM,      RO ,     NULL,    NULL}, // device data

};



static ST_REG_4XXXX_ATTR_BLOCK reg_4x_block_sector[] = {
    /*       存储位置基址                                                           寄存器起始位置   寄存器个数  类型          读写权限                 回调*/
    /* METER 1 */
    { (struct remap_Modbus_block_node *)&remapMaxDmdDay_meter1[0],                    11300,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapMaxDmdMonth_meter1[0],                  11500,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapEnergyMultirate_meter1[0],              11900,      0,        MB_TYPE_RAM,      RO ,       NULL,    NULL}, // 

    //    { (struct remap_Modbus_node*)&remapEnergyFourquadrant_meter1[0],                12100,      0,        MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
//    { (struct remap_Modbus_node*)&remapEnergyFundwave_meter1[0],                    12400,      0,        MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
//    { (struct remap_Modbus_node*)&remapEnergyRuning_meter1[0],                      12700,      0,        MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_block_node *)&remapExtremeDataDay_meter1[0],               13000,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         }, // 
    { (struct remap_Modbus_block_node *)&remapExtremeDataMonth_meter1[0],             13400,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         }, //     
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicThd_meter1[0],      13800,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicAngle_meter1[0],    13850,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicHr_meter1[0],       13900,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicRms_meter1[0],      14300,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },

    /* METER 1 */
    { (struct remap_Modbus_block_node *)&remapMaxDmdDay_meter2[0],                    21300,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapMaxDmdMonth_meter2[0],                  21500,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapEnergyMultirate_meter2[0],              21900,      0,        MB_TYPE_RAM,      RO ,       NULL,    NULL}, // 
//    { (struct remap_Modbus_node*)&remapEnergyFourquadrant_meter2[0],                22100,      0,        MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
//    { (struct remap_Modbus_node*)&remapEnergyFundwave_meter2[0],                    22400,      0,        MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
//    { (struct remap_Modbus_node*)&remapEnergyRuning_meter2[0],                      22700,      0,        MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_block_node *)&remapExtremeDataDay_meter2[0],               23000,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         }, // 
    { (struct remap_Modbus_block_node *)&remapExtremeDataMonth_meter2[0],             23400,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         }, //     
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicThd_meter2[0],      23800,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicAngle_meter2[0],    23850,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicHr_meter2[0],       23900,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicRms_meter2[0],      24300,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },

    /* METER 1 */
    { (struct remap_Modbus_block_node *)&remapMaxDmdDay_meter3[0],                    31300,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapMaxDmdMonth_meter3[0],                  31500,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapEnergyMultirate_meter3[0],              31900,      0,        MB_TYPE_RAM,      RO ,       NULL,    NULL}, // 
//    { (struct remap_Modbus_node*)&remapEnergyFourquadrant_meter3[0],                32100,      0,        MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
//    { (struct remap_Modbus_node*)&remapEnergyFundwave_meter3[0],                    32400,      0,        MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
//    { (struct remap_Modbus_node*)&remapEnergyRuning_meter3[0],                      32700,      0,        MB_TYPE_RAM,      RO ,     NULL,    NULL}, // 
    { (struct remap_Modbus_block_node *)&remapExtremeDataDay_meter3[0],               33000,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         }, // 
    { (struct remap_Modbus_block_node *)&remapExtremeDataMonth_meter3[0],             33400,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         }, //     
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicThd_meter3[0],      33800,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicAngle_meter3[0],    33850,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicHr_meter3[0],       33900,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },
    { (struct remap_Modbus_block_node *)&remapPowerqualityHarmonicRms_meter3[0],      34300,      0,        MB_TYPE_RAM,      RO ,       NULL,	    NULL         },

};


/*********************************************************************************************************
** Function name:       mb_productTest_handle
** Descriptions:        生产测试hook回调，用于生产测试时用到的
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static uint16_t *mb_productTest_info_handle (struct mb_comm_ctx_t *pCtx, uint16_t regaddr, uint16_t regnum, uint8_t *p_data)
{
    int         ret;
    uint16_t    volt;

    ret = mb_i2c_rtc_volt_get(&volt);
    if(ret != -1)
        pCtx->pvmem->db_cfg_info.product_test.rtc = volt;
    

    return NULL;
}





/*********************************************************************************************************
** Function name:       mb_cmd_test_led_handle
** Descriptions:        led test
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_test_led_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    
    return 0;
}


/*********************************************************************************************************
** Function name:       mb_mac_write_then_read
** Descriptions:        MAC write then read
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_mac_write_then_read(char *filename, char *character, uint8_t len)
{
    FILE* fp = NULL;
    char mac[18];

    fp = fopen(filename, "wb");
    if(NULL == fp)
    {
        goto __err_close;
    }
    fwrite(character, strlen(character), 1, fp);
    fclose(fp);

    fp = fopen(filename, "r");
    if(NULL == fp)
    {
        goto __err_close;
    }
    int n = fread(mac, 1, sizeof(mac) - 1, fp);
    if(n <= 0)
    {
        goto __err_close;
    }
    mac[n] = '\0';

    if( 0 == strncmp(character,mac,strlen(mac)))
    {
        /* write success */
        fclose(fp);
        return 0;
    }

__err_close:
    if(fp != NULL)
    {
        fclose(fp);
    }
    return -1;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_test_set_mac_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig,  uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    char     mac_addr[18];
    struct   db_database_type *pdb = pCtx->pvmem;
    
    /* for example: "A2:8F:5A:F3:AD:57" 最大17个字符+1个结束符, 所以寄存器需要9个.   */    
    if(reg_num == 9)  // PMAC770-H - - - 共12字节， 寄存器为6个。
    {
        if((p_value[2] != ':') || (p_value[5] != ':') || (p_value[8] != ':') || (p_value[11] != ':') || (p_value[14] != ':'))
            return -1;

        /*get mac */
        snprintf(mac_addr, sizeof(mac_addr),"%s",p_value);  

        /*mac write check */
        if(0 == mb_mac_write_then_read("/userdata/config/system/mac",mac_addr,sizeof(mac_addr))) 
        {
            /*update memory mac*/
            snprintf((char*)&pdb->db_measurebase[0].db_cfg.user.network.mac_addr,sizeof(pdb->db_measurebase[0].db_cfg.user.network.mac_addr), "%s", mac_addr);
        } 
    }
    else
    {
        return -1;
    }
    return 0;
}



/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_test_set_model_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig,  uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    int      fd;

    fd = open("/userdata/config/system/model", O_WRONLY | O_CREAT | O_TRUNC);
    if(fd < 0)
    {
        return -1;
    }
    write(fd, p_value, strlen(p_value));
    close(fd);
    
    db_msg_event_cmd(DB_EVENT_TYPE_SAVE_DEVICEINFO);
    
    return 0;
}



/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_test_set_sn_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig,  uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    int      fd;

    /* for example: "TSM2205010001" 共14个字符+1个结束符, 所以寄存器需要8个 */    
    if((p_value[0] != 'T') || (p_value[1] != 'S'))
        return -1;

    fd = open("/userdata/config/system/sn", O_WRONLY | O_CREAT | O_TRUNC);
    if(fd < 0)
    {
        return -1;
    }
    write(fd, p_value, strlen(p_value));
    close(fd);

    /* sn */
    strncpy((char*)&pCtx->pvmem->db_cfg_info.device.SNInfo[0], p_value, 15);
    db_msg_event_cmd(DB_EVENT_TYPE_SAVE_DEVICEINFO);
    
    return 0;
}

/*********************************************************************************************************
** Function name:       mb_cmd_test_envset_handle
** Descriptions:        生产测试，环境预设
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_test_lcd_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    
    return 0;
}

/*********************************************************************************************************
** Function name:       mb_cmd_test_envset_handle
** Descriptions:        生产测试，环境预设
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_test_envset_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;
    struct db_database_type *pdb = pCtx->pvmem;

    /* 生产测试时，继电器设置为远程模式，只是变量更改，不设置到配置文件 */
    pCtx->pvmem->db_cfg_info.product_test.testing = 1;

    pdb->db_expand.db_relay.relay[0].mode.value = 0;
    pdb->db_expand.db_relay.relay[1].mode.value = 0;

  
    // 客户产品需求 需要RS485-2 为他的程序使用。当前进入测试状态临时
     value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value == MB_WRITE_CMD)
        system("/etc/init.d/modbus_slave_rs485_2 start");
    else 
        system("/etc/init.d/modbus_slave_rs485_2 stop");
        
    return 0;
}


/*********************************************************************************************************
** Function name:       mb_cfguser_handle
** Descriptions:        用户数据写回调
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cfgusermultirage_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    int                                 i;
    uint16_t                            value;
    struct db_cfg_CellProperties_u16        *pCfg16;
    /*
        according the "nItem" to decide which struct to handle data, and write the data. 
        this will make us easy to change the PARAMETER STRUCT value .
    */
    for(i=0;  i<reg_num; i++)
    {
        /*
                    node 根据modbus寄存器为单元指向变量的相应位置，而不是变量的起始地址。
                    为了找到配置中的 scope ，需要先转换回变量的原始地址

                    比如 32bit   ABCD
                    可能当前node指向 CD（这样处理是因为根据modbus的16bit一个寄存器连续查找需要），但我们做变量计算以及范围约束时
                    需要找到结构的原始地址
                */
        value = MB_ALIGN_PT_16BIT_READ(p_value + i * 2);

        switch(pModifyConfig->nItem)
        {
            case 1:    
                pCfg16 = (struct db_cfg_CellProperties_u16 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));

                if((value <= pCfg16->scope.max) && (value >= pCfg16->scope.min))
                {
                    *pModifyConfig->node = value;
                    /* next node of this section */
                    pModifyConfig++;
                }
                else 
                {
                    // load the cfg
                    db_msg_event_cmd(DB_EVENT_TYPE_USER_MULTIRATE_JSON_TO_RAM);
                    return -1;
                }
            break;
        }
    }
    /* 
        POST message for update the setting of user
    */
    db_msg_event_cmd(DB_EVENT_TYPE_USER_MULTIRATE_RAM_TO_JSON);
    return 0;
}



/*********************************************************************************************************
** Function name:       mb_cfguser_handle
** Descriptions:        用户数据写回调
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cfguser_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    int                                 i;
    uint16_t                            value;
    struct db_cfg_CellProperties_u16    *pCfg16;
    struct db_cfg_CellProperties_u32    *pCfg32;
    /*
        according the "nItem" to decide which struct to handle data, and write the data. 
        this will make us easy to change the PARAMETER STRUCT value .
    */
    for(i=0;  i<reg_num; i++)
    {
        /*
                    node 根据modbus寄存器为单元指向变量的相应位置，而不是变量的起始地址。
                    为了找到配置中的 scope ，需要先转换回变量的原始地址

                    比如 32bit   ABCD
                    可能当前node指向 CD（这样处理是因为根据modbus的16bit一个寄存器连续查找需要），但我们做变量计算以及范围约束时
                    需要找到结构的原始地址
                */
        value = MB_ALIGN_PT_16BIT_READ(p_value + i * 2);

        switch(pModifyConfig->nItem)
        {
            case 1:
                pCfg16 = (struct db_cfg_CellProperties_u16 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                if((value <= pCfg16->scope.max) && (value >= pCfg16->scope.min))
                {
                    *pModifyConfig->node = value;
                    /* next node of this section */
                    pModifyConfig++;
                }
                else if(pCfg16->scope.max <= pCfg16->scope.min)     //为了解决报警功能开关没有限制值
                {
                    *pModifyConfig->node = value;
                    pModifyConfig++;
                }
                else 
                {
                    // load the cfg
                    db_msg_event_cmd(DB_EVENT_TYPE_USER_JSON_TO_RAM);
                    return -1;
                }
            break;
            case 2:
                pCfg32 = (struct db_cfg_CellProperties_u32 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                /*
                    32bit 设置时，低址字限定最小值，高字不限
                */
                if(pModifyConfig->offset == pModifyConfig->origin_offset)
                {
                    if((value >= pCfg32->scope.min))
                    {
                        *pModifyConfig->node = value;
                        /* next node of this section */
                        pModifyConfig++;
                    }
                    else
                    {
                        // load cfg
                        db_msg_event_cmd(DB_EVENT_TYPE_USER_JSON_TO_RAM);
                        return -1;
                    }
                }
                else 
                {
                    *pModifyConfig->node = value;
                    /* next node of this section */
                    pModifyConfig++;
                }
                  
            break;
        }
    }
    /* 
        POST message for update the setting of user
    */
    db_msg_event_cmd(DB_EVENT_TYPE_USER_RAM_TO_JSON);
    return 0;
}



/*********************************************************************************************************
** Function name:      mb_cmd_synctime_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_synctime_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    long int t;

    if(reg_num != 2)
        return -1;

    t = MB_ALIGN_PT_16BIT_READ(p_value) + MB_ALIGN_PT_16BIT_READ(p_value + 2) * 65536;
    {
        struct timespec ts;

        ts.tv_sec = t;
        ts.tv_nsec = 0;
        clock_settime(CLOCK_REALTIME, &ts);
    }

    system("hwclock -w -u &");

    return 0;
}

/*********************************************************************************************************
** Function name:      mb_cmd_synctime_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_shadow_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    int                                 i;
    uint16_t                            value;
    struct db_cfg_CellProperties_u16    *pCfg16;
    struct db_cfg_CellProperties_u32    *pCfg32;
    /*
        according the "nItem" to decide which struct to handle data, and write the data. 
        this will make us easy to change the PARAMETER STRUCT value .
    */
    for(i=0;  i<reg_num; i++)
    {
        /*
                    node 根据modbus寄存器为单元指向变量的相应位置，而不是变量的起始地址。
                    为了找到配置中的 scope ，需要先转换回变量的原始地址

                    比如 32bit   ABCD
                    可能当前node指向 CD（这样处理是因为根据modbus的16bit一个寄存器连续查找需要），但我们做变量计算以及范围约束时
                    需要找到结构的原始地址
                */
        value = MB_ALIGN_PT_16BIT_READ(p_value + i * 2);

        switch(pModifyConfig->nItem)
        {
            case 1:
                pCfg16 = (struct db_cfg_CellProperties_u16 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                *pModifyConfig->node = value;
                pModifyConfig++;

                
            break;
            case 2:
                pCfg32 = (struct db_cfg_CellProperties_u32 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                *pModifyConfig->node = value;
                /* next node of this section */
                pModifyConfig++;
                
                  
            break;
        }
    }



    return 0;
}





/*********************************************************************************************************
** Function name:      mb_cmd_timezone_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_timezone_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t   value;

    if(reg_addr != 8302 )
        return -1;

    value = MB_ALIGN_PT_16BIT_READ(p_value);

    /*set timezone*/
    mb_set_timezone(value);

    return 0;
}


/*********************************************************************************************************
** Function name:      mb_cfgsys_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cfgsys_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    int                                 i;
    uint16_t                            value;
    struct db_cfg_CellProperties_u16    *pCfg16;
    struct db_cfg_CellProperties_u32    *pCfg32;

    /*
        according the "nItem" to decide which struct to handle data, and write the data. 
        this will make us easy to change the PARAMETER STRUCT value .
    */
    for(i=0;  i<reg_num; i++)
    {
        /*
                    node 根据modbus寄存器为单元指向变量的相应位置，而不是变量的起始地址。
                    为了找到配置中的 scope ，需要先转换回变量的原始地址

                    比如 32bit   ABCD
                    可能当前node指向 CD（这样处理是因为根据modbus的16bit一个寄存器连续查找需要），但我们做变量计算以及范围约束时
                    需要找到结构的原始地址
                */
        value = MB_ALIGN_PT_16BIT_READ(p_value + i * 2);

        switch(pModifyConfig->nItem)
        {
            case 1:
                pCfg16 = (struct db_cfg_CellProperties_u16 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                
                if((value <= pCfg16->scope.max) && (value >= pCfg16->scope.min))
                {
                    *pModifyConfig->node = value;
                    /* next node of this section */
                    pModifyConfig++;
                }
                else 
                {
                    // load the cfg
                    db_msg_event_cmd(DB_EVENT_TYPE_SYS_JSON_TO_RAM);
                    return -1;
                }
            break;
            case 2:
                pCfg32 = (struct db_cfg_CellProperties_u32 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                /*
                    32bit 设置时，低址字限定最小值，高字不限
                */
                if(pModifyConfig->offset == pModifyConfig->origin_offset)
                {
                    if((value >= pCfg32->scope.min))
                    {
                        *pModifyConfig->node = value;
                        /* next node of this section */
                        pModifyConfig++;
                    }
                    else
                    {
                        // load the cfg
                        db_msg_event_cmd(DB_EVENT_TYPE_SYS_JSON_TO_RAM);
                        return -1;
                    }
                }
                else 
                {
                    *pModifyConfig->node = value;
                    /* next node of this section */
                    pModifyConfig++;
                }
                  
            break;
        }
    }

    /* 
        POST message for update the setting of user
    */
    db_msg_event_cmd(DB_EVENT_TYPE_SYS_RAM_TO_JSON);
    return 0;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_ota_set_url_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_node *pModifyConfig,  uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    /* 
        POST message for update the setting of user
    */
    ota_http_start((char*)p_value, pCtx->pvmem, 0);
    
    return 0;
}

/*********************************************************************************************************
** Function name:      mb_cmd_demo_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_demo_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 

    if(!((value == 1) || (value == 0)))
        return -1;
    
    // POST Message to measure_process
//    if(value == 1)
//    {
//        db_msr_msg_event_cmd(CHANNLNO_RN8306, DB_MSR_EVENT_TYPE_RN8306_DEMO);
//    }
//    else
//    {
//        db_msr_msg_event_cmd(CHANNLNO_RN8306, DB_MSR_EVENT_TYPE_RN8306_DEMO_END);
//    }
        
    return 0;
}

/*********************************************************************************************************
** Function name:      mb_cmd_demo_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_rn7302_demo_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 

    if(!((value == 1) || (value == 0)))
        return -1;
    
    // POST Message to measure_process
    if(value == 1)
//    {
//        db_msr_msg_event_cmd(CHANNLNO_RN7302, DB_MSR_EVENT_TYPE_RN7302_DEMO);
//    }
//    else
//    {
//        db_msr_msg_event_cmd(CHANNLNO_RN7302, DB_MSR_EVENT_TYPE_RN7302_DEMO_END);
//    }
        
    return 0;
}




/*********************************************************************************************************
** Function name:      mb_cmd_adjust_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_adjustRst_handle_meter1(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    // POST Message to measure_process
    db_msr_msg_event_cmd(CHANNLNO_RN7302_1, DB_MSR_EVENT_TYPE_ADJUST_RST);
    return 0;
};
/*********************************************************************************************************
** Function name:      mb_cmd_adjust_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_adjustRst_handle_meter2(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    // POST Message to measure_process
    db_msr_msg_event_cmd(CHANNLNO_RN7302_2, DB_MSR_EVENT_TYPE_ADJUST_RST);
    return 0;
};

/*********************************************************************************************************
** Function name:      mb_cmd_adjust_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_adjustRst_handle_meter3(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    // POST Message to measure_process
    db_msr_msg_event_cmd(CHANNLNO_RN7302_3, DB_MSR_EVENT_TYPE_ADJUST_RST);
    return 0;
};

/*********************************************************************************************************
** Function name:      mb_cmd_adjust_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_adjustRst_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    // POST Message to measure_process
    db_msr_msg_event_cmd(CHANNLNO_RN7302_1, DB_MSR_EVENT_TYPE_ADJUST_RST);
    db_msr_msg_event_cmd(CHANNLNO_RN7302_2, DB_MSR_EVENT_TYPE_ADJUST_RST);
    db_msr_msg_event_cmd(CHANNLNO_RN7302_3, DB_MSR_EVENT_TYPE_ADJUST_RST);
    return 0;
};


/*********************************************************************************************************
** Function name:      mb_cmd_adjust_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_adjust_handle_meter1(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    int                                 i;
    uint16_t                            value;
    struct db_cfg_CellProperties_u16    *pCfg16;
    struct db_cfg_CellProperties_u32    *pCfg32;

    /*
        according the "nItem" to decide which struct to handle data, and write the data. 
        this will make us easy to change the PARAMETER STRUCT value .
    */
    for(i=0;  i<reg_num; i++)
    {
        /*
                    node 根据modbus寄存器为单元指向变量的相应位置，而不是变量的起始地址。
                    为了找到配置中的 scope ，需要先转换回变量的原始地址

                    比如 32bit   ABCD
                    可能当前node指向 CD（这样处理是因为根据modbus的16bit一个寄存器连续查找需要），但我们做变量计算以及范围约束时
                    需要找到结构的原始地址
                */
        value = MB_ALIGN_PT_16BIT_READ(p_value + i * 2);

        switch(pModifyConfig->nItem)
        {
            case 1:
                pCfg16 = (struct db_cfg_CellProperties_u16 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                
                *pModifyConfig->node = value;
                /* next node of this section */
                pModifyConfig++;
            break;
            case 2:
                pCfg32 = (struct db_cfg_CellProperties_u32 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                /*
                    32bit 设置时，低址字限定最小值，高字不限
                */
                *pModifyConfig->node = value;
                /* next node of this section */
                pModifyConfig++;
            break;
        }
    }


    /*
        校表命令下发后执行步骤：
         1. 保存校表参数值到ROM
         2  执行 Gv  Gi 配置到计量芯片。（因为后面校表需要根据增益来计算校表参数）
         3. 执行校表命令
    */

    /* 
        POST message for update the setting of user
    */
    db_msg_event_cmd(DB_EVENT_TYPE_SYS_RAM_TO_JSON);
    // POST Message to measure_process
    db_msr_msg_event_cmd(CHANNLNO_RN7302_1, DB_MSR_EVENT_TYPE_ADJUST);
    
    return 0;
};


/*********************************************************************************************************
** Function name:      mb_cmd_adjust_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_adjust_handle_meter2(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    int                                 i;
    uint16_t                            value;
    struct db_cfg_CellProperties_u16    *pCfg16;
    struct db_cfg_CellProperties_u32    *pCfg32;

    for(i=0;  i<reg_num; i++)
    {
        /*
                    node 根据modbus寄存器为单元指向变量的相应位置，而不是变量的起始地址。
                    为了找到配置中的 scope ，需要先转换回变量的原始地址

                    比如 32bit   ABCD
                    可能当前node指向 CD（这样处理是因为根据modbus的16bit一个寄存器连续查找需要），但我们做变量计算以及范围约束时
                    需要找到结构的原始地址
                */
        value = MB_ALIGN_PT_16BIT_READ(p_value + i * 2);

        switch(pModifyConfig->nItem)
        {
            case 1:
                pCfg16 = (struct db_cfg_CellProperties_u16 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                
                *pModifyConfig->node = value;
                /* next node of this section */
                pModifyConfig++;
            break;
            case 2:
                pCfg32 = (struct db_cfg_CellProperties_u32 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                /*
                    32bit 设置时，低址字限定最小值，高字不限
                */
                *pModifyConfig->node = value;
                /* next node of this section */
                pModifyConfig++;
            break;
        }
    }
    
    // POST Message to measure_process
    db_msg_event_cmd(DB_EVENT_TYPE_SYS_RAM_TO_JSON);
//    db_msr_msg_event_cmd(CHANNLNO_RN7302_2, DB_MSR_EVENT_TYPE_REINIT);
    db_msr_msg_event_cmd(CHANNLNO_RN7302_2, DB_MSR_EVENT_TYPE_ADJUST);
    return 0;
};




/*********************************************************************************************************
** Function name:      mb_cmd_adjust_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_adjust_handle_meter3(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    int                                 i;
    uint16_t                            value;
    struct db_cfg_CellProperties_u16    *pCfg16;
    struct db_cfg_CellProperties_u32    *pCfg32;

    for(i=0;  i<reg_num; i++)
    {
        /*
                    node 根据modbus寄存器为单元指向变量的相应位置，而不是变量的起始地址。
                    为了找到配置中的 scope ，需要先转换回变量的原始地址

                    比如 32bit   ABCD
                    可能当前node指向 CD（这样处理是因为根据modbus的16bit一个寄存器连续查找需要），但我们做变量计算以及范围约束时
                    需要找到结构的原始地址
                */
        value = MB_ALIGN_PT_16BIT_READ(p_value + i * 2);

        switch(pModifyConfig->nItem)
        {
            case 1:
                pCfg16 = (struct db_cfg_CellProperties_u16 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                
                *pModifyConfig->node = value;
                /* next node of this section */
                pModifyConfig++;
            break;
            case 2:
                pCfg32 = (struct db_cfg_CellProperties_u32 *)(pModifyConfig->node - (pModifyConfig->offset - pModifyConfig->origin_offset));
                /*
                    32bit 设置时，低址字限定最小值，高字不限
                */
                *pModifyConfig->node = value;
                /* next node of this section */
                pModifyConfig++;
            break;
        }
    }
    
    // POST Message to measure_process
    db_msg_event_cmd(DB_EVENT_TYPE_SYS_RAM_TO_JSON);
//    db_msr_msg_event_cmd(CHANNLNO_RN7302_3, DB_MSR_EVENT_TYPE_REINIT);
    db_msr_msg_event_cmd(CHANNLNO_RN7302_3, DB_MSR_EVENT_TYPE_ADJUST);
    return 0;
};



/*********************************************************************************************************
** Function name:       mb_cmd_adjust_offset_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_adjust_offset_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;

    // POST Message to measure_process
    db_msr_msg_event_cmd(CHANNLNO_RN7302_1, DB_MSR_EVENT_TYPE_ADJUST_OFFSET);
    return 0;
};
/*********************************************************************************************************
** Function name:      mb_cmd_clearE_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_clearE_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    /* 
        POST message for update the setting of user
    */
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_REALTIME_ENERGY);
    
    return 0;
};


/*********************************************************************************************************
** Function name:      mb_cmd_clearE_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_historyE_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    /* 
        POST message for update the setting
    */
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_HISTORY_ENERGY);
    
    return 0;
};

/*********************************************************************************************************
** Function name:      mb_cmd_clearMultirateE_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_clearMultirateE_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    /* 
        POST message for update the setting of user
    */
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_MULTIRATE_ENERGY);
    
    return 0;
};

/*********************************************************************************************************
** Function name:      mb_cmd_clearE2_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_clearE2_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    /* 
        POST message for update the setting of user
    */
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_REALTIME_ENERGY2);
    
    return 0;
};

/*********************************************************************************************************
** Function name:      mb_cmd_clearE2_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_historyE2_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    /* 
        POST message for update the setting
    */
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_HISTORY_ENERGY2);
    
    return 0;
};

/*********************************************************************************************************
** Function name:      mb_cmd_clearMultirateE2_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_clearMultirateE2_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    /* 
        POST message for update the setting of user
    */
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_MULTIRATE_ENERGY2);
    
    return 0;
};

/*********************************************************************************************************
** Function name:      mb_cmd_clearAllUser_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_clearAllUser_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
   /* 
        POST message for update the setting 
    */
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_REALTIME_ENERGY);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_HISTORY_ENERGY);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_MAX_DEMAND);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_EXTREME);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_MULTIRATE_ENERGY);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_LOADRUN);
    db_msg_event_cmd(DB_EVENT_TYPE_BOOT);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_RUNING_ENERGY);

    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_REALTIME_ENERGY2);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_HISTORY_ENERGY2);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_MULTIRATE_ENERGY2);

     /*
        POST message to measure
    */
    // do something

    return 0;
};

/*********************************************************************************************************
** Function name:       mb_cfguser_new_multirate_handle
** Descriptions:        复费率数据写回调

** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cfguser_new_multirate_handle(struct mb_comm_ctx_t *pCtx, struct remap_Modbus_block_node *pModifyConfig, uint16_t base_regaddr, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    int                                         i;
    uint16_t                                    value;
    struct db_cfg_CellProperties_noscope_u16    *pCfg16;

    /*
        according the "nItem" to decide which struct to handle data, and write the data. 
        this will make us easy to change the PARAMETER STRUCT value .
    */
    for(i=0;  i<reg_num; i++)
    {
        /*
            node 根据modbus寄存器为单元指向变量的相应位置，而不是变量的起始地址。
            
        */
        value = MB_ALIGN_PT_16BIT_READ(p_value + i * 2);

        /*
            块映射配置的参数 不支持scope且参数是16bit对齐，所以无需判断最大最小值。 只需要根据寄存器号进行赋值即可
        */
        pCfg16 = (struct db_cfg_CellProperties_noscope_u16 *)(pModifyConfig->node + (reg_addr - base_regaddr) + i);
        pCfg16->value = value;
    }
    
    /* 
        POST message for update the setting of user
    */
    db_msg_event_cmd(DB_EVENT_TYPE_USER_MULTIRATE_RAM_TO_JSON);
    return 0;
}

/*********************************************************************************************************
** Function name:      mb_cmd_clearRunTime_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_clearLoadRun_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
   /* 
        POST message for update the setting 
    */
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_LOADRUN);

     /*
        POST message to measure
    */
    // do something

    return 0;
};






/*********************************************************************************************************
** Function name:      mb_cmd_runing_energy_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_clear_runing_energy_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
   /* 
        POST message for update the setting 
    */
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_RUNING_ENERGY);

     /*
        POST message to measure
    */

    return 0;
};



/*********************************************************************************************************
** Function name:      mb_cmd_clearSoe_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_reboot_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    /* 
        POST message for update the setting of user
    */
    db_msg_event_cmd(DB_EVENT_TYPE_REBOOT);
    
    return 0;
};

/*********************************************************************************************************
** Function name:      mb_cmd_clearSoe_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_boot_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    /* 
        POST message for update the setting of user
    */
    db_msg_event_cmd(DB_EVENT_TYPE_BOOT);
    
    return 0;
};
/*********************************************************************************************************
** Function name:      mb_cmd_clearSoe_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline int mb_cmd_clearSoe_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
    /* 
        POST message for update the setting of user
    */
    
    return 0;
};

/*********************************************************************************************************
** Function name:      mb_cmd_clearUserCfg_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_clearUserCfg_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
   /* 
        POST message for update the setting 
    */
    db_msg_event_cmd(DB_EVENT_TYPE_USER_CFG_DEFAULT);
    
    return 0;
};

/*********************************************************************************************************
** Function name:      mb_cmd_clearSysCfg_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_clearSysCfg_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
   /* 
        POST message for update the setting 
    */
    db_msg_event_cmd(DB_EVENT_TYPE_SYS_CFG_DEFAULT);

    /*
        POST message to measure
    */
    // do something

    return 0;
};


/*********************************************************************************************************
** Function name:      mb_cmd_clearSysCfg_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_clearMaxDmd_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
   /* 
        POST message for update the setting 
    */
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_MAX_DEMAND);

    return 0;
};

/*********************************************************************************************************
** Function name:      mb_cmd_clearAllCfg_handle 
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_cmd_clearAllCfg_handle(struct mb_comm_ctx_t *pCtx,  struct remap_Modbus_node *pModifyConfig, uint16_t reg_addr, uint16_t reg_num, uint8_t *p_value)
{
    uint16_t value;

    value = MB_ALIGN_PT_16BIT_READ(p_value); 
    if(value != MB_WRITE_CMD)
        return -1;
    
   /* 
        POST message for update the setting 
    */
    db_msg_event_cmd(DB_EVENT_TYPE_SYS_CFG_DEFAULT);
    db_msg_event_cmd(DB_EVENT_TYPE_USER_CFG_DEFAULT);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_REALTIME_ENERGY);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_HISTORY_ENERGY);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_MAX_DEMAND);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_EXTREME);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_MULTIRATE_ENERGY);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_LOADRUN);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_RUNING_ENERGY);

    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_REALTIME_ENERGY2);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_HISTORY_ENERGY2);
    db_msg_event_cmd(DB_EVENT_TYPE_CLEAR_MULTIRATE_ENERGY2);

     /*
        POST message to measure
    */
    // do something

    return 0;
};



/*********************************************************************************************************
** Function name:       mb_config_getSectionCnt
** Descriptions:        modbus rtu 获取寄存器段的个数
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      寄存器段的个数
*********************************************************************************************************/
static uint8_t remap_config_getSectionCnt(void)
{
    return sizeof(reg_4x_section)/sizeof(reg_4x_section[0]);
}


/*********************************************************************************************************
** Function name:       remap_config_getBlockSectionCnt
** Descriptions:        modbus rtu 获取寄存器段的个数
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      寄存器段的个数
*********************************************************************************************************/
static uint8_t remap_config_getBlockSectionCnt(void)
{
    return sizeof(reg_4x_block_sector)/sizeof(reg_4x_block_sector[0]);
}

/*********************************************************************************************************
** Function name:       mb_config_getSectionAttr
** Descriptions:        modbus rtu 获取块寄存器段的属性 (标准协议)
** input parameters:    index - 段索引
** output parameters:   NONE
** Returned value:      块寄存器段属性指针
*********************************************************************************************************/
static int remap_config_getSectionAttr(uint8_t index, const ST_REG_4XXXX_ATTR **pAttr)
{
    if (index >= sizeof(reg_4x_section)/sizeof(reg_4x_section[0]) || (pAttr == NULL))
    {
        return -1;
    }
    *pAttr = &reg_4x_section[index];
    return 0;
}

/*********************************************************************************************************
** Function name:       mb_config_getBlockSectionAttr
** Descriptions:        modbus rtu 获取块寄存器段的属性 (标准协议)
** input parameters:    index - 段索引
** output parameters:   NONE
** Returned value:      块寄存器段属性指针
*********************************************************************************************************/
static int remap_config_getBlockSectionAttr(uint8_t index, const ST_REG_4XXXX_ATTR_BLOCK **pAttr)
{
    if (index >= sizeof(reg_4x_block_sector)/sizeof(reg_4x_block_sector[0])) 
    {
        return -1;
    }
    *pAttr = &reg_4x_block_sector[index];
    return 0;
}

/*********************************************************************************************************
** Function name:       mb_config_init
** Descriptions:        modbus rtu 协议配置初始化,主要是从EEPROM中加载配置，并判断配置是否正确
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void remap_config_init(void *vmem)
{
    int i, j;
    struct system_info   *tmp_systeminfo = mb_systeminfo_pointer_get();
    struct db_database_type *pstVmem = (struct db_database_type *)vmem;

    /*
        由于使用动态获取内存作为结构存取，不能在remap结构体中初始化定义 vmem地址，
        因此在此初始化时输入vmem
        注意：
            这里是为了保持结构体类型一致，牺牲点空间和启动时间。运行时可节省时间
            同时保持接头体一致，也是为了框架方便移植到静态定义数据结构的情况  
    */

    for(i=0; i < remap_config_getSectionCnt(); i++)
    {
        const ST_REG_4XXXX_ATTR *section;
        remap_config_getSectionAttr(i, &section);
        if (section != NULL) 
        {
            for(j=0; j<section->reg_len; j++)
            {
                if(section->p_data && section->p_data[j].node == 0)
                {
                    section->p_data[j].node = (uint16_t *)(vmem) + section->p_data[j].offset;
                }
            }
        }
    }
   

    /*
        块设备 电表初始化：
        主要是计算初始地址、寄存器长度
    */
    for(i=0; i < remap_config_getBlockSectionCnt(); i++)
    {
        const ST_REG_4XXXX_ATTR_BLOCK *block_section;
        remap_config_getBlockSectionAttr(i, &block_section);
        if (block_section != NULL) 
        {
            ST_REG_4XXXX_ATTR_BLOCK *nonconst_block_section = (ST_REG_4XXXX_ATTR_BLOCK *)block_section;
            nonconst_block_section->p_data->node = ((uint16_t *)vmem) + nonconst_block_section->p_data->offset;
            nonconst_block_section->reg_len = (uint16_t)nonconst_block_section->p_data->reg_len;
        }        
    }

}

/*********************************************************************************************************
** Function name:       mb_config_standard_fops
** Descriptions:        modbus rtu 标准协议配置函数表
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
struct mb_remap_vendor_fops_t mb_config_standard_fops = 
{
    .init                   = remap_config_init,
    .getSectionCnt          = remap_config_getSectionCnt,
    .getSectionAttr         = remap_config_getSectionAttr,
    .getBlockSectionCnt     = remap_config_getBlockSectionCnt,
    .getBlockSectionAttr    = remap_config_getBlockSectionAttr,
    .setSectionSelector     = NULL,
};


