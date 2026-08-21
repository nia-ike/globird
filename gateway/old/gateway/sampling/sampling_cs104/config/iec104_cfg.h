#ifndef __IEC104_CFG_H__
#define __IEC104_CFG_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>

#include "cJSON.h"
#include "cJSON_Utils.h"


#define CONFIG_NODE_NAME_LEN				12			/* 测点名称长度 */
#define CONFIG_MAX_REQUEST_PKT_LEN			16

#define CFG_MAX_POINT					    2048			/* 单台智能设备最大测点数 */
#define CONFIG_MAX_CTRLNODESIZE_PER_DEVICE	8           /* 单台智能设备最大控制点数 */

enum eCommStatus
{// 设备状态描述
	eCommStatus_Unknown = 0, //未知
	eCommStatus_Fault = 1, //故障
	eCommStatus_Normal = 2, //正常
};

/*
	测点属性
*/
struct config_node_attr
{
	uint16_t			id;					/* id */
	uint8_t				datattr;			/* 0-AI，1-DI，2-ACC   */
	uint32_t			catagory;			/* 云服务类型，如A相电压 */
	double				kfactor;			/* k 系数 */
	int					inf_addr;			/* 信息体地址 */
};


/*------------------     控制结构  ------------------ */
struct config_control_attr
{
	uint8_t             id;  /*id*/
	int					inf_addr;			/* 信息体地址 */
	bool				double_ctrl;		/* 是否双点遥控（仅用于DO） */
	double				kfactor;			/* k 系数（仅用于AO） */
};

/* 设备点表属性 */
struct config_devices_attr
{
	uint8_t							checked;												/* 本设备有效 */
	int32_t							timing_inv;// 校时间隔(min)
	uint16_t						node_num;												/* 本设备节点数 ，即node_attr 有效数量 */
	uint8_t							addr_size;// ASDU地址长度，默认是1，可以调整为2
	uint8_t							cause_size;// 传送原因长度，默认是1，可以调整为2
    bool                            debug_enabled;//调试模式
	struct config_node_attr			*node_attr;
	uint8_t                         control_num;                                            /* 电表控制点个数 */
	struct config_control_attr 		control_attr[CONFIG_MAX_CTRLNODESIZE_PER_DEVICE];		/* 电表控制点设置 */
};


int iec104_cfg_load(struct config_devices_attr *pCfg, char *file);
int32_t iec104_cfg_node_attr_size(struct config_devices_attr *pDevAttr);
struct config_node_attr *iec104_cfg_node_attr(struct config_devices_attr *pDevAttr, uint8_t id);
int32_t iec104_cfg_control_attr_size(struct config_devices_attr *pDevAttr);
struct config_control_attr *iec104_cfg_control_attr(struct config_devices_attr *pDevAttr, uint8_t id);

#endif
