/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：Interface.c
*  文件标识：
*  描    述：SimpleGUI接口
*
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: linux
*  硬件平台：f1c100s
*  固件库  ：
*  修改记录
*  作者       时间         版本      修改描述
*  chenzy   2020       V1.00
*/

#ifndef __DB_STRUCT_H__
#define __DB_STRUCT_H__

#include <stdint.h>
#include "db_measure.h"
#include "db_cfg.h"
#include "db_measure_info.h"
#include "db_fcas_inject.h"


enum 
{
    CHANNLNO_RN7302_1,
    CHANNLNO_RN7302_2,
    CHANNLNO_RN7302_3,
};

enum
{
    VIRTUAL_MODE_GRID,
    VIRTUAL_MODE_PV,
    VIRTUAL_MODE_HYBRID,
    VIRTUAL_MODE_MAX,
};

/*
    当前硬件为3个三相通道， 第4通道暂时预留，用于后续扩展
    4个通道，分别是：
    1、RN8306： 三相通道1
    2、RN7302： 三相通道2
    3、RN7302： 三相通道3
    4、RN7302： 预留通道
*/
#define NUM   4

struct db_database_measure_base_type
{
    struct db_database_measure_type    db_measure;     //测量数据
    struct db_database_cfg_para        db_cfg;         //配置数据
    struct db_measure_runtime_info_type  info;           //测量刷新序号等信息
};

struct db_database_info_type
{
    uint16_t version;           //版本号
    uint16_t md5;               //md5 
    uint16_t size;              //结构体大小  
    uint16_t reserve[61];       //占位，尽量不影响后面的结构体
};

struct db_database_type
{
    struct db_database_info_type            db_info;        //结构体信息
    struct db_database_cfg_info_type        db_cfg_info;
    /* 多费率参数 */
    struct db_cfg_user_multirate_para       multirate;
    /* 每个回路的测量和配置参数 */
    struct db_database_measure_base_type    db_measurebase[NUM];
    /* 扩展参数 */
    struct db_database_expand_type          db_expand;
    /* 空参数 */
    struct db_null_para                     db_null;
    /* FCAS 半波测试注入控制（仅追加在末尾，避免破坏既有偏移） */
    struct db_fcas_inject_type              db_fcas_inject;
};







int db_cfg_user_expand_init(struct db_database_type *pDB);
int db_cfg_user_expand_update(struct db_database_type *pDB);
void db_cfg_user_expand_default(struct db_database_type *pDB);
int db_cfg_user_expand_load(struct db_database_type *pDB, const char *file);
struct db_database_expand_type *db_cfg_get_user_digi(struct db_database_type *pDB);


/*
    extern for modbus configure table.
    if use for update data, display or else, please use db_realtime_getmem() and db_energy_getmem()
*/

int db_cfg_sys_init(struct db_database_measure_base_type *pDB, uint16_t flg);
int db_cfg_sys_update(struct db_database_measure_base_type *pDB, uint16_t flg);
void db_cfg_sys_default(struct db_database_measure_base_type *pDB, uint16_t flg);
int db_cfg_sys_load(struct db_database_measure_base_type *pDB, const char *file, uint16_t flg);
struct db_cfg_sys_para *db_cfg_get_sys(struct db_database_measure_base_type *pDB);

int db_cfg_init(struct db_database_measure_base_type *pDB, uint16_t flg);
int db_cfg_user_init(struct db_database_measure_base_type *pDB, uint16_t flg);
int db_cfg_user_update(struct db_database_measure_base_type *pDB, uint16_t flg);
void db_cfg_user_default(struct db_database_measure_base_type *pDB, uint16_t flg);
int db_cfg_user_load(struct db_database_measure_base_type *pDB, const char *file, uint16_t flg);
struct db_cfg_user_para *db_cfg_get_user(struct db_database_measure_base_type *pDB);

int db_cfg_user_base_init(struct db_database_measure_base_type *pDB, uint16_t flg);
int db_cfg_user_base_update(struct db_database_measure_base_type *pDB, uint16_t flg);
void db_cfg_user_base_default(struct db_database_measure_base_type *pDB, uint16_t flg);

int db_energy_init(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain, const char *filebakup);
int db_energy_destroy(struct db_database_measure_base_type *pDB, uint8_t channel);
void db_energy_update(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain, const char *filebakup);
void db_energy_update_main(struct db_database_measure_base_type *pDB, uint8_t channel);  // 只更新主库
void db_energy_update_backup(struct db_database_measure_base_type *pDB, uint8_t channel); // 只更新备份库
int db_energy_default(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain, const char *filebakup);
void db_energy_cleanup_all(void);  // 清理所有通道的能量上下文


int db_history_energy_init(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain);
void db_history_energy_process(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain);
int db_history_energy_default(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain);
void db_history_energy_destroy(struct db_database_measure_base_type *pDB, uint8_t channel);
void db_history_energy_cleanup_all(void);  // 清理所有通道的历史能量上下文


int db_maxdmd_init(struct db_database_measure_base_type *pDB, uint8_t channel);
int db_maxdmd_default(struct db_database_measure_base_type *pDB, uint8_t channel);
void db_maxdmd_update(struct db_database_measure_base_type *pDB, uint8_t channel);
void db_maxdmd_destroy(struct db_database_measure_base_type *pDB, uint8_t channel);
void db_maxdmd_cleanup_all(void);  // 清理所有通道的需量最值上下文

int db_multirate_energy_init(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain, const char *filebakup);
int db_multirate_energy_destroy(struct db_database_measure_base_type *pDB, uint8_t channel);
void db_multirate_energy_update(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain, const char *filebakup);
void db_multirate_energy_update_main(struct db_database_measure_base_type *pDB, uint8_t channel);  // 只更新主库
void db_multirate_energy_update_backup(struct db_database_measure_base_type *pDB, uint8_t channel); // 只更新备份库
int db_multirate_energy_default(struct db_database_measure_base_type *pDB, uint8_t channel, const char *filemain, const char *filebakup);
void db_multirate_energy_cleanup_all(void);  // 清理所有通道的多费率能量上下文
struct db_cfg_user_multirate_para *db_cfg_get_user_multirate(struct db_database_type *pDB);


int db_cfg_user_multirate_init(struct db_database_type *pDB);
int db_cfg_user_multirate_update(struct db_database_type *pDB);
void db_cfg_user_multirate_default(struct db_database_type *pDB);


int db_loadrun_init(struct db_database_measure_base_type *pDB, uint8_t channel);
int db_loadrun_destroy(struct db_database_measure_base_type *pDB, uint8_t channel);
void db_loadrun_update(struct db_database_measure_base_type *pDB, uint8_t channel);
int db_loadrun_default(struct db_database_measure_base_type *pDB, uint8_t channel);
void db_loadrun_cleanup_all(void);  // 清理所有通道的负载运行上下文


int  db_runing_energy_init(struct db_database_measure_base_type *pDB, uint8_t channel);
int  db_runing_energy_destroy(struct db_database_measure_base_type *pDB, uint8_t channel);
int  db_runing_energy_default(struct db_database_measure_base_type *pDB, uint8_t channel);
void db_runing_energy_update(struct db_database_measure_base_type *pDB, uint8_t channel);
void db_runing_energy_cleanup_all(void);  // 清理所有通道的运行能量上下文


#endif

