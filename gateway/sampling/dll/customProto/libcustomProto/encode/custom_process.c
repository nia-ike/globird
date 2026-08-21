/*
 * File      : custom_process.c
 * Descriptions: customProto 收包回调：可选帧匹配 + 按帧绝对偏移解析测点
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "sampling_cfg.h"
#include "db_node_fops.h"
#include "modbus-datatype.h"
#include "custom_cfg.h"
#include "custom_process.h"
#include "crc16.h"
#include "pkt.h"

/*********************************************************************************************************
 ** Function name:		 GenerateBitMask
** Descriptions:		 根据 bitnum 生成从最低位开始的连续 1 掩码（与 modbus 位域解析一致）
** input parameters:	 bitnum		位宽 1~15；越界则返回全 1
** output parameters:	 NONE
** Returned value: 	 掩码值
*********************************************************************************************************/
static uint16_t GenerateBitMask(uint8_t bitnum)
{
	if (bitnum == 0 || bitnum >= 16)
		return 0xFFFF;
	return (uint16_t)((1u << bitnum) - 1u);
}

/*********************************************************************************************************
 ** Function name:		 resolve_offset
** Descriptions:		 将配置中的偏移转为绝对下标；负值表示相对帧尾（如 -2 表示倒数第 2 字节）
** input parameters:	 off		配置偏移（可负）
                         len		当前帧长度
** output parameters:	 NONE
** Returned value: 	 绝对字节下标
*********************************************************************************************************/
static int resolve_offset(int off, int len)
{
	if (off < 0)
		return len + off;
	return off;
}

/*********************************************************************************************************
 ** Function name:		 frame_match
** Descriptions:		 应答帧可选匹配（全部默认关闭时直接通过，降低使用成本）
                         - expect_len==0：不校验长度
                         - frame_header_len==0：不校验帧头
                         - addr_offset<0：不校验地址字节
                         - checksum==none：不校验校验和
** input parameters:	 f		帧匹配属性
                         buf	接收缓冲
                         len	接收长度
** output parameters:	 NONE
** Returned value: 	 0 匹配成功（或全部跳过）；-1 匹配失败
*********************************************************************************************************/
static int frame_match(const struct custom_frame_attr *f, const uint8_t *buf, int len)
{
	int from, to, store, i;
	uint16_t crc, got16;
	uint8_t sum, xorv, got8;

	if (f == NULL || buf == NULL || len <= 0)
		return -1;

	/* expect_len: 0 = 不校验 */
	if (f->expect_len > 0 && (uint16_t)len != f->expect_len)
		return -1;

	/* frame_header: 空 = 不校验；从帧第 0 字节起比较 */
	if (f->frame_header_len > 0) {
		if (len < (int)f->frame_header_len)
			return -1;
		if (memcmp(buf, f->frame_header, f->frame_header_len) != 0)
			return -1;
	}

	/* addr: offset < 0 = 不校验 */
	if (f->addr_offset >= 0) {
		if (f->addr_offset >= len)
			return -1;
		if (buf[f->addr_offset] != f->addr_value)
			return -1;
	}

	/* checksum: none = 不校验 */
	if (f->checksum == CUSTOM_CHECKSUM_NONE)
		return 0;

	/* [from, to) 为计算区间；store 为校验存放起始偏移 */
	from = resolve_offset(f->checksum_from, len);
	to = resolve_offset(f->checksum_to, len);
	store = resolve_offset(f->checksum_store, len);
	if (from < 0 || to < from || store < 0)
		return -1;

	switch (f->checksum) {
	case CUSTOM_CHECKSUM_CRC16_MODBUS:
		if (store + 1 >= len)
			return -1;
		crc = crc16((uint8_t *)buf + from, (uint32_t)(to - from));
		if (f->checksum_endian_le)
			got16 = (uint16_t)(buf[store] | (buf[store + 1] << 8));
		else
			got16 = (uint16_t)((buf[store] << 8) | buf[store + 1]);
		if (crc != got16)
			return -1;
		break;

	case CUSTOM_CHECKSUM_SUM8:
		if (store >= len)
			return -1;
		sum = 0;
		for (i = from; i < to; i++)
			sum = (uint8_t)(sum + buf[i]);
		got8 = buf[store];
		if (sum != got8)
			return -1;
		break;

	case CUSTOM_CHECKSUM_XOR8:
		if (store >= len)
			return -1;
		xorv = 0;
		for (i = from; i < to; i++)
			xorv = (uint8_t)(xorv ^ buf[i]);
		got8 = buf[store];
		if (xorv != got8)
			return -1;
		break;

	default:
		break;
	}
	return 0;
}

/*********************************************************************************************************
 ** Function name:		 bytes_to_regs
** Descriptions:		 从整帧绝对字节偏移处，按 Modbus 寄存器大端方式打包为 uint16 数组，
                         以便复用 Modbus_RegDatatoDouble 同一套 datatype 解析
** input parameters:	 buf		接收帧
                         buflen		帧长度
                         offset		测点帧偏移（相对 frame[0]）
                         reg_num	占用寄存器个数（由 datatype 决定）
                         regs		输出寄存器缓冲
** output parameters:	 regs
** Returned value: 	 0 成功；-1 越界或参数非法
*********************************************************************************************************/
static int bytes_to_regs(const uint8_t *buf, int buflen, int offset, int reg_num, uint16_t *regs)
{
	int need = reg_num * 2;
	int i;

	if (offset < 0 || reg_num <= 0)
		return -1;
	if (offset + need > buflen)
		return -1;
	/* 每寄存器 2 字节，高字节在前（与 Modbus PDU 数据域一致） */
	for (i = 0; i < reg_num; i++) {
		regs[i] = (uint16_t)((buf[offset + i * 2] << 8) | buf[offset + i * 2 + 1]);
	}
	return 0;
}

/*********************************************************************************************************
 ** Function name:		 write_point_value
** Descriptions:		 按测点 range_checked 规则写入实时库（与 modbus 清洗逻辑一致）
                         0-关闭清洗原样赋值；1-满足范围写1否则写0；2-满足范围才更新
** input parameters:	 pIndexInfo	设备点表索引
                         idx		测点下标
                         pNodeAttr	测点属性
                         fVal		工程值
** output parameters:	 NONE
** Returned value: 	 NONE
*********************************************************************************************************/
static void write_point_value(struct db_spl_remap_info_t *pIndexInfo, int idx,
			      struct config_node_attr *pNodeAttr, double fVal)
{
	switch (pNodeAttr->range_checked) {
	case 1:
		if (((int)fVal >= pNodeAttr->range_min) && ((int)fVal <= pNodeAttr->range_max))
			fVal = 1;
		else
			fVal = 0;
		DataMgmtFops.writepoint_in1dev(pIndexInfo, idx, &fVal);
		break;
	case 2:
		if (((int)fVal >= pNodeAttr->range_min) && ((int)fVal <= pNodeAttr->range_max))
			DataMgmtFops.writepoint_in1dev(pIndexInfo, idx, &fVal);
		break;
	default:
		DataMgmtFops.writepoint_in1dev(pIndexInfo, idx, &fVal);
		break;
	}
}

/*********************************************************************************************************
 ** Function name:		 fnReceiveCallback
** Descriptions:		 解包回调函数（由 channelpoll 在收包后调用）
                         1. 可选帧匹配
                         2. 遍历归属本 pkt_id 的测点
                         3. 按 dataoffset（帧绝对字节）+ datatype 解析并写库
** input parameters:	 pDevInfoAttr	设备信息
                         prequest_list	当前请求/应答帧节点（含 precv_buff、private=请求信息）
** output parameters:	 NONE
** Returned value: 	 0 正常；-1 匹配失败或参数错误（上层计通讯异常）
*********************************************************************************************************/
int32_t fnReceiveCallback(struct config_devices_info_attr *pDevInfoAttr,
			  struct config_request_list *prequest_list)
{
	struct config_devices_attr *pDevAttr;
	struct config_pkt_request_info *pReq;
	struct config_node_attr *pNodeAttr;
	struct db_spl_remap_info_t *pIndexInfo;
	uint16_t regs[32];
	int nodeSize, i, reg_num;
	double fVal;
	uint8_t *buf;
	int len;

	pDevAttr = (struct config_devices_attr *)pDevInfoAttr->private;
	if (!pDevAttr || !prequest_list || !prequest_list->precv_buff)
		return -1;

	buf = prequest_list->precv_buff;
	len = prequest_list->recv_len;
	/* 发送时装载的本条请求私有参数 */
	pReq = (struct config_pkt_request_info *)prequest_list->private;
	if (!pReq)
		return -1;

	/* 可选匹配；默认全关则直接进入解析 */
	if (frame_match(&pDevAttr->frame, buf, len) != 0)
		return -1;

	pIndexInfo = DataMgmtFops.getIndexByPortAndDev(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id);
	if (!pIndexInfo)
		return -1;

	nodeSize = custom_cfg_node_attr_size(pDevAttr);
	for (i = 0; i < nodeSize; i++) {
		pNodeAttr = custom_cfg_node_attr(pDevAttr, i);
		if (!pNodeAttr)
			continue;
		/* pkt_id==0：CommStatus 等非解析点，由 poll 层写通讯状态 */
		if (pNodeAttr->pkt_id == 0)
			continue;
		/* 仅解析归属本请求报文的测点 */
		if (pNodeAttr->pkt_id != pReq->pkt_id)
			continue;

		reg_num = Modbus_DataTypeToRegNum(pNodeAttr->datatype);
		if (reg_num <= 0)
			reg_num = 1;
		if (reg_num > (int)(sizeof(regs) / sizeof(regs[0])))
			continue;

		/* dataoffset：相对整帧第 0 字节 */
		if (bytes_to_regs(buf, len, pNodeAttr->dataoffset, reg_num, regs) != 0)
			continue;

		if (pNodeAttr->datatype == DATATYPE_DATA_BITS) {
			if (pNodeAttr->bitnum <= 1)
				fVal = round((double)((regs[0] >> pNodeAttr->bitfield) & 0x01));
			else
				fVal = round((double)((regs[0] >> pNodeAttr->bitfield) &
						      GenerateBitMask(pNodeAttr->bitnum)));
		} else {
			/* 与 modbus 相同 datatype 表与系数公式：val*k*pt*ct+b */
			if (Modbus_RegDatatoDouble(pNodeAttr->datatype, pNodeAttr->bitfield, regs, &fVal) < 0)
				continue;
			fVal = fVal * (pNodeAttr->kfactor * pNodeAttr->pt_flag1 * pNodeAttr->ct_flag2)
			       + pNodeAttr->bfactor;
			fVal = round(fVal * 1000) / 1000.0;
		}
		write_point_value(pIndexInfo, i, pNodeAttr, fVal);
	}
	return 0;
}
