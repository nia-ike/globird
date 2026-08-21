/*
*  Copyright (c) 2019,广州碳索智控科技有限公司
*  All rights reserved.
* 
*  文件名称：
*  文件标识：
*  描    述：
* 
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: linux
*  硬件平台：
*  固件库  ：
*  修改记录 
*  作者       时间         版本      修改描述
*  chenzy          V1.00      
*/


#ifndef __OPC_UA_CFG_H__
#define __OPC_UA_CFG_H__



#include <stdint.h>
#include <stdbool.h>
#include "opc_ua_com_baseclass.h"

#include "list.h"
#include "db_struct.h"

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>

/* 串口属性  NO USE. 使用电表本身的串口参数，不使用这里的参数，仅仅预留 */
struct config_chnl_serial_attr
{
	uint32_t 					baud;			
	uint8_t						datasize;
	uint8_t						parity;
	uint8_t						stopbit;
};

/* 通道扩展属性 */
struct config_chnl_ext_attr
{
	char							whitelist1[16];		
	char							whitelist2[16];	
};

/* 点表 */
struct db_node_list_t
{
    slist_t                         list;
    uint8_t                         datattr;    /*0-AI 1-DI 2-ACC*/
    uint32_t                        db_link;
    float                           kfactor;
    
    char                            name[48];

    UA_NodeId                       UA_nodeId;
};



struct db_device_list_t
{
	slist_t						list;
	union db_node_index_id      id;
	
    char                        name[48];		//安装位置/描述
	
	struct db_node_list_t		node_list;
};



/*	工作参数	 */
struct config_chnl_working_attr
{
    bool                            debug;
	uint8_t							checked;
	uint8_t							id;
	char							dev[48];										/* /dev/ttyS1, /dev/lora.... */
	char							mode[16];										/* poll, listen */
	char							link[24];										/* serial, tcp client */
    char							protocol[24];									/* 协议类型：modbusTCP\modbusRTU\ */
    char							register_type[24];								/* serial, tcp client */
    char							register_context[64];							/* serial, tcp client */


    struct db_device_list_t         device_list;

    struct config_chnl_ext_attr     ext_attr;
	struct config_chnl_serial_attr	serial_attr;

    struct opc_ua_comm_ctx_t        opc_ua_comm_ctx;

    //---------------------------------------------------------
    UA_Server                       *ua_server;
};





int opc_ua_config_init(struct config_chnl_working_attr *pCfg_chnlattr, char *cfg_file);






#endif
