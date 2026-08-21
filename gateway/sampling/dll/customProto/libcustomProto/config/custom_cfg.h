/*
 * File      : custom_cfg.h
 * Descriptions: customProto 点表配置结构与装载接口
 */
#ifndef __CUSTOM_CFG_H__
#define __CUSTOM_CFG_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CONFIG_MAX_REQUEST_PKT_LEN			16	/* 单设备最大请求帧数 */
#define CFG_MAX_POINT					    768
#define CUSTOM_TX_MAX_LEN					256	/* 单帧原文最大长度（字节） */
#define CUSTOM_HEADER_MAX_LEN				32

/* 校验算法：默认 NONE，不匹配直接解析 */
enum custom_checksum_algo
{
	CUSTOM_CHECKSUM_NONE = 0,
	CUSTOM_CHECKSUM_CRC16_MODBUS,
	CUSTOM_CHECKSUM_SUM8,
	CUSTOM_CHECKSUM_XOR8,
};

/* 设备级应答帧匹配属性（属性栏对应字段） */
struct custom_frame_attr
{
	uint16_t	expect_len;			/* 0 = 不校验长度 */
	uint8_t		frame_header[CUSTOM_HEADER_MAX_LEN];
	uint16_t	frame_header_len;	/* 0 = 不校验帧头 */
	uint8_t		checksum;			/* custom_checksum_algo */
	int16_t		checksum_from;		/* 计算起点，可负 */
	int16_t		checksum_to;		/* 计算终点（不含），可负；常用 -2 */
	int16_t		checksum_store;		/* 校验存放偏移，可负 */
	uint8_t		checksum_endian_le;	/* 1=le, 0=be */
	int16_t		addr_offset;		/* <0 = 不校验地址字节 */
	uint8_t		addr_value;
};

/* 单条请求报文：原文 tx 直发 */
struct config_pkt_request_info
{
	uint16_t	pkt_id;
	uint16_t	tx_len;
	uint8_t		tx[CUSTOM_TX_MAX_LEN];
};

struct config_pkg_request
{
	uint16_t						pkt_num;
	struct config_pkt_request_info	pkt_req_info[CONFIG_MAX_REQUEST_PKT_LEN];
};

/* 测点：pkt_id + 帧绝对字节偏移 + 与 Modbus 一致的 datatype */
struct config_node_attr
{
	uint16_t	id;
	uint8_t		datatype;
	uint8_t		pkt_id;				/* 0=非解析点（如 CommStatus） */
	uint8_t		bitfield;
	uint8_t		bitnum;
	uint8_t		flag;				/* datattr AI/DI/ACC */
	uint16_t	dataoffset;			/* 相对整帧第 0 字节 */
	uint32_t	catagory;
	float		kfactor;
	float		bfactor;
	float		pt_flag1;
	float		ct_flag2;
	uint8_t		range_checked;
	int			range_min;
	int			range_max;
};

struct config_devices_attr
{
	uint8_t						checked;
	struct custom_frame_attr	frame;
	struct config_pkg_request	pkg_request;
	uint16_t					node_num;
	struct config_node_attr		node_attr[CFG_MAX_POINT];
};

/*********************************************************************************************************
 ** Function name:		 custom_cfg_load
** Descriptions:		 从设备点表 JSON 装载配置
** input parameters:	 pCfg, file
** Returned value: 	 0 成功；-1 失败
*********************************************************************************************************/
int custom_cfg_load(struct config_devices_attr *pCfg, char *file);

/*********************************************************************************************************
 ** Function name:		 custom_cfg_node_attr_size
** Descriptions:		 获取测点数量
*********************************************************************************************************/
int32_t custom_cfg_node_attr_size(struct config_devices_attr *pDevAttr);

/*********************************************************************************************************
 ** Function name:		 custom_cfg_node_attr
** Descriptions:		 按数组下标取测点属性
*********************************************************************************************************/
struct config_node_attr *custom_cfg_node_attr(struct config_devices_attr *pDevAttr, uint16_t id);

#endif
