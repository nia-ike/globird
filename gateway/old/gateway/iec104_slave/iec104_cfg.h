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


#ifndef __IEC104_CFG_H__
#define __IEC104_CFG_H__



#include <stdint.h>
#include <stdbool.h>
#include "iec104_com_baseclass.h"

#include "list.h"

#define MB_FW_POINT_MAX         8192



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
struct config_chnl_node_attr
{
    slist_t                         list;
    uint8_t                         func_code;
    uint8_t                         datatype;
    uint8_t                         datattr;    /*0-AI 1-DI 2-ACC*/
	uint16_t                        reg_no;
    uint32_t                        db_link;
    float                           kfactor;
    float                           bfactor;
    double                          lastvalue;
    
};

/* node attr*/
struct config_node_group
{
    uint32_t                            addr;
    uint16_t                            node_num;
    struct config_chnl_node_attr        node_attr;
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

//    uint16_t                            node_num;
//    struct config_chnl_node_attr        node_attr[MB_FW_POINT_MAX];

    struct config_node_group            MMEA;
    struct config_node_group            MMEB;
    struct config_node_group            MMEC;
    struct config_node_group            CCI;
    struct config_node_group            MSP;
    struct config_node_group            MDP;
    struct config_node_group            CSC;
    struct config_node_group            CDC;
    struct config_node_group            CRC;
    struct config_node_group            CSEA;
    struct config_node_group            CSEB;
    struct config_node_group            CSEC;


    struct config_chnl_ext_attr         ext_attr;
	struct config_chnl_serial_attr		serial_attr;

    struct iec104_comm_ctx_t                iec104_comm_ctx;
};








int iec104_config_init(struct config_chnl_working_attr *pCfg_chnlattr, char *cfg_file);
int iec104_get_nodeattr_by_regno(struct config_chnl_node_attr index[], uint16_t reg_no, int low, int high);






#endif
