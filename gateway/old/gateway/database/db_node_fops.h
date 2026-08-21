/*
* Copyright (c) 2013,珠海一构物联有限公司
* All rights reserved.
*
* 文件名称：db_cfg.h
* 文件标识：
* 描    述：
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录
*  作者       时间         版本      修改描述
*  chenzy   2013.11.08   V1.00      创建
*/

#ifndef __DB_NODE_FOPS_H__
#define __DB_NODE_FOPS_H__


#include <stdint.h>

#include "db_struct.h"





struct DataMgmt_fops
{
	/*******************************************************************************
	* Function Name  : Init
	* Description	 : 初始化函数，系统启动时调用
	*				   
	* Input 		 : None
	*				   
	* Output		 : None
	* Return		 : 0: success	-1: fail
	*******************************************************************************/
	int32_t 			(*init)(void *vmem);

    uint32_t            (*map_node_num)(void);
    uint32_t            (*map_dev_num)(void);
    uint32_t            (*map_active_node_size)(void);

	/*******************************************************************************
	* Function Name  : GetPointObj
	* Description    : 获取点属性对象
	*                  
	* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
	*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
	*                  ucnIndex: 测点序号
	*                  
	* Output         : None
	* Return         : aItem: 整个测点对象指针
	*******************************************************************************/
	struct db_spl_remap_info_t * (*getIndexByPortAndDev)(uint8_t ucnPort, uint8_t ucnDev);
    struct db_spl_remap_info_t * (*getIndexDevBySeq)(uint16_t i);
	/*******************************************************************************
	* Function Name  : ReadPoint
	* Description    : 读取指定点的实时值
	*                  
	* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
	*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
	*                  ucnIndex: 测点序号
	* Output         : f64Value: 测点的实时值
	* Return         : 0: success   -1: fail
	*******************************************************************************/
	int32_t 			(*readpoint)(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, double *f64Value);
	int32_t 			(*readpoint_by_id)(uint32_t ul32ID, double *f64Value);
	int32_t 			(*readpoint_in1dev)(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, double *f64Value);                      
	int32_t 			(*writepoint)(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, double *f64Value);
	int32_t 			(*writepoint_by_id)(uint32_t ul32ID, double *f64Value);                   
	int32_t 			(*writepoint_in1dev)(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, double *f64Value);

	int32_t 			(*readpoint_int64)(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, int64_t *i64Value);
	int32_t 			(*readpoint_int64_by_id)(uint32_t ul32ID, int64_t *i64Value);
	int32_t 			(*readpoint_int64_in1dev)(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, int64_t *i64Value);                      
	int32_t 			(*writepoint_int64)(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, int64_t *i64Value);
	int32_t 			(*writepoint_int64_by_id)(uint32_t ul32ID, int64_t *i64Value);                   
	int32_t 			(*writepoint_int64_in1dev)(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, int64_t *i64Value);

	int32_t 			(*readpoint_string)(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, char *str, uint8_t str_len);
	int32_t 			(*readpoint_string_by_id)(uint32_t ul32ID, char *str, uint8_t str_len);
	int32_t 			(*readpoint_string_in1dev)(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, char *str, uint8_t str_len);                      
	int32_t 			(*writepoint_string)(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, char *str, uint8_t str_len);
	int32_t 			(*writepoint_string_by_id)(uint32_t ul32ID, char *str, uint8_t str_len);                   
	int32_t 			(*writepoint_string_in1dev)(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, char *str, uint8_t str_len);

	int32_t 			(*write_command_op)(uint32_t nodeid, double f64val);
	int32_t 			(*flush_command_op)(uint8_t chnl_id);

    /* 新增：控制点(DO/AO)实时值接口（仅追加在末尾，保持旧版成员偏移不变） */
    int32_t             (*readctrlpoint)(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, double *f64Value);
    int32_t             (*readctrlpoint_by_id)(uint32_t ul32ID, double *f64Value);
    int32_t             (*writectrlpoint)(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, double *f64Value);
    int32_t             (*writectrlpoint_by_id)(uint32_t ul32ID, double *f64Value);
};


extern const struct DataMgmt_fops DataMgmtFops;

struct db_spl_realtime_data_tbl_t *db_DataTblObj(struct db_spl_realtime_data_tbl_t *p);

/*********************************************************************************************************
 ** Function name:       db_ctrl_queue_pending_count
 ** Descriptions:        统计指定通道主池与扩展池中尚未下发（is_active 非 0）的控制条数之和。
 ** input parameters:      chnl_id 通道号。
 ** output parameters:     NONE
 ** Returned value:         待下发条数；内部表指针无效时返回 0。
 *********************************************************************************************************/
int db_ctrl_queue_pending_count(uint8_t chnl_id);

/*********************************************************************************************************
 ** Function name:       db_ctrl_alloc_idle
 ** Descriptions:        分配空闲控制槽：先主池 ctrl[][]，满则 ctrl_extend_region 扩展池。详见 .c 实现注释。
 ** input parameters:      pDB 实时库指针；chnl_id 通道号。
 ** output parameters:     NONE
 ** Returned value:         空闲槽指针；无空槽返回 NULL。
 *********************************************************************************************************/
struct db_spl_channel_ctr_t *db_ctrl_alloc_idle(struct db_spl_realtime_data_tbl_t *pDB, uint8_t chnl_id);

/*********************************************************************************************************
 ** Function name:       db_ctrl_active_first
 ** Descriptions:        取下一条待下发控制项：先主池后扩展池。详见 .c 实现注释。
 ** input parameters:      pDB 实时库指针；chnl_id 通道号。
 ** output parameters:     NONE
 ** Returned value:         待下发项指针；无则 NULL。
 *********************************************************************************************************/
struct db_spl_channel_ctr_t *db_ctrl_active_first(struct db_spl_realtime_data_tbl_t *pDB, uint8_t chnl_id);

/** 按 device_num 选择 64/256 表布局，返回表尾 debug 域；保证 magic 已初始化。 */
struct db_spl_debug_region_t *db_debug_region(struct db_spl_realtime_data_tbl_t *pDB);

/** 返回 &debug.bypass；pDB 无效时返回 NULL。 */
struct db_spl_bypass_ctl_t *db_bypass_ctl(struct db_spl_realtime_data_tbl_t *pDB);

#endif
