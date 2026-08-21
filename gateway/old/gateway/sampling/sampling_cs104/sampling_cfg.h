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

#ifndef __SAMPLING_CFG_H__
#define __SAMPLING_CFG_H__

#include <stdint.h>
#include <stdbool.h>
#include "db_struct.h"
#include "sampling_com_baseclass.h"
#include "list.h"

#include "iec104_cfg.h"

#define FRAME_RECVBUF_SIZE      1460





/* 
	data attribute setting macro
	强烈注意： 当前版本这里的宏定义必须与datamgmt文件的数量宏定义一致
*/


struct sampling_optarg_t
{
    uint32_t sn;
	uint8_t	 dump;
	uint8_t  channl_no;
    uint16_t tcp_port;
	char     cfg_file[64];
};





/* 串口属性  NO USE. 使用电表本身的串口参数，不使用这里的参数，仅仅预留 */
struct config_serial_attr
{
	 uint32_t 						baud;			
	 uint8_t						datasize;
	 uint8_t						parity;
	 uint8_t						stopbit;
};


/*	工作参数	 */
struct config_working_attr
{
	uint8_t							checked;
	uint8_t							id;
	char							name[16];										/* 通道名称		*/
	char							dev[48];										/* /dev/ttyS1, /dev/lora.... */
	char 							dst_url[32];
	int 							dst_port;
	char							mode[16];										/* poll, listen */
	char							link[24];										/* serial, tcp client */

	uint8_t 						repeat;
	uint32_t						inv_time;
	uint32_t						RecvTimeOut;
	uint32_t						framing_time;									/* 本通道接收帧间超时时间， 用于某些仪表同一帧分两段延时发 */

	struct config_serial_attr		serial_attr;
};

/*
	类型定义声明
*/
struct config_devices_info_attr;
typedef int32_t (*pfnFlowCallback_t)(struct config_devices_info_attr *, struct sampling_com_baseclass_ops_t *, struct sampling_comm_ctx_t *);




/*	设备组 参数	 */
struct config_devices_info_attr
{
	slist_t							slist;
	
	uint8_t							channel_id;
	uint8_t 						logic_id;
	uint16_t						loop_id;
	uint64_t 						phy_id;
	char 							protocol[96];
	char 							dll[96];
	char 							cfg_file[96];
	
	/*
		实时点表地址
		测点读写到 pVmem
	*/
	struct db_spl_realtime_data_tbl_t *pVmem;


	/*
		该设备私有的参数结构装载，
		放该设备的 自己定义的任意数据结构首地址，用于在该设备so的其他回调函数使用
		比如放 协议寄存器号、流号、中间变量等等任意协议需要的数据，每种协议数据结构都可以自由定义

	*/
	struct config_devices_attr	DevAttr;									/* 指向 各设备的对应协议点表属性结构 */

	/* 预留给扩展用途 */
	void 					*ext_func;
	void					*ext_args;
};




/* 通道属性 */
struct config_channel_attr
{	
	struct config_working_attr			working_attr;								/* 重发次数、断线重连、轮训间隔等 */
	struct config_devices_info_attr		devices_list_head;								/* 设备数量不定，在解析时动态生成，加入slist进行管理 */
};










#endif
