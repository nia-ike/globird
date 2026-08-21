/*
 * File      : custom_cfg.c
 * Descriptions: customProto 设备点表 JSON 装载
 */

#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cJSON.h"
#include "cJSON_Utils.h"
#include "custom_cfg.h"

/*********************************************************************************************************
 ** Function name:		 jsonfile_alloc
** Descriptions:		 按文件大小申请 JSON 读缓冲
** input parameters:	 file	文件路径
                         size	输出文件大小
** output parameters:	 size
** Returned value: 	 缓冲区指针；失败 NULL
*********************************************************************************************************/
static char *jsonfile_alloc(char *file, int *size)
{
	struct stat filestat;

	if (file == NULL)
		return NULL;
	if (stat(file, &filestat) != 0) {
		printf("Json file[%s] not exist\n", file);
		return NULL;
	}
	*size = filestat.st_size;
	return malloc(filestat.st_size + 1);
}

/*********************************************************************************************************
 ** Function name:		 jsonfile_read
** Descriptions:		 读取 JSON 文件内容到缓冲并补 '\0'
** input parameters:	 file	文件路径
                         p		已申请缓冲
                         size	缓冲容量
** output parameters:	 p 内容被填充
** Returned value: 	 p 成功；NULL 失败
*********************************************************************************************************/
static char *jsonfile_read(char *file, char *p, int size)
{
	FILE *fp;
	int n;

	fp = fopen(file, "rb");
	if (fp == NULL)
		return NULL;
	n = fread(p, 1, size, fp);
	fclose(fp);
	if (n <= 0)
		return NULL;
	p[n] = '\0';
	return p;
}

/*********************************************************************************************************
 ** Function name:		 custom_parse_hex_bytes
** Descriptions:		 解析十六进制字符串为字节数组
                         支持 "AA 55 01" 或 "AA5501"；非法字符或奇数个半字节返回失败
** input parameters:	 hex		输入字符串
                         out		输出字节缓冲
                         out_max	缓冲最大长度
** output parameters:	 out
** Returned value: 	 >=0 字节数；-1 解析失败；hex 为空返回 0
*********************************************************************************************************/
static int custom_parse_hex_bytes(const char *hex, uint8_t *out, int out_max)
{
	int n = 0;
	const char *p = hex;
	int hi = -1;

	if (hex == NULL)
		return 0;

	while (*p) {
		if (isspace((unsigned char)*p)) {
			p++;
			continue;
		}
		if (!isxdigit((unsigned char)*p))
			return -1;
		{
			int v = isdigit((unsigned char)*p) ? (*p - '0')
				: (tolower((unsigned char)*p) - 'a' + 10);
			if (hi < 0) {
				hi = v;
			} else {
				if (n >= out_max)
					return -1;
				out[n++] = (uint8_t)((hi << 4) | v);
				hi = -1;
			}
		}
		p++;
	}
	/* 奇数个半字节非法 */
	if (hi >= 0)
		return -1;
	return n;
}

/*********************************************************************************************************
 ** Function name:		 streq_ci
** Descriptions:		 不区分大小写字符串比较
** input parameters:	 a, b	字符串
** output parameters:	 NONE
** Returned value: 	 1 相等；0 不等
*********************************************************************************************************/
static int streq_ci(const char *a, const char *b)
{
	if (a == NULL || b == NULL)
		return 0;
	while (*a && *b) {
		if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
			return 0;
		a++;
		b++;
	}
	return *a == *b;
}

/*********************************************************************************************************
 ** Function name:		 parse_checksum_algo
** Descriptions:		 解析校验算法字符串：none / crc16_modbus / sum8 / xor8
                         未知或空串按 none 处理（默认不匹配）
** input parameters:	 s	算法名
** output parameters:	 NONE
** Returned value: 	 custom_checksum_algo 枚举值
*********************************************************************************************************/
static int parse_checksum_algo(const char *s)
{
	if (s == NULL || s[0] == '\0')
		return CUSTOM_CHECKSUM_NONE;
	if (streq_ci(s, "none"))
		return CUSTOM_CHECKSUM_NONE;
	if (streq_ci(s, "crc16_modbus"))
		return CUSTOM_CHECKSUM_CRC16_MODBUS;
	if (streq_ci(s, "sum8"))
		return CUSTOM_CHECKSUM_SUM8;
	if (streq_ci(s, "xor8"))
		return CUSTOM_CHECKSUM_XOR8;
	return CUSTOM_CHECKSUM_NONE;
}

/*********************************************************************************************************
 ** Function name:		 frame_attr_defaults
** Descriptions:		 帧匹配属性默认值：全部关闭（直接解析）
** input parameters:	 f	帧属性结构
** output parameters:	 f 被置为默认
** Returned value: 	 NONE
*********************************************************************************************************/
static void frame_attr_defaults(struct custom_frame_attr *f)
{
	memset(f, 0, sizeof(*f));
	f->expect_len = 0;				/* 0=不校验长度 */
	f->frame_header_len = 0;		/* 0=不校验帧头 */
	f->checksum = CUSTOM_CHECKSUM_NONE;
	f->checksum_from = 0;
	f->checksum_to = -2;
	f->checksum_store = -2;
	f->checksum_endian_le = 1;
	f->addr_offset = -1;			/* <0=不校验地址 */
	f->addr_value = 0;
}

/*********************************************************************************************************
 ** Function name:		 load_frame_attr
** Descriptions:		 从 JSON 对象装载帧匹配属性（frame 或 extend_param）
                         缺省字段保持 defaults（NONE）
** input parameters:	 f		输出帧属性
                         pFrame	JSON 对象，可为 NULL
** output parameters:	 f
** Returned value: 	 0 成功；-1 帧头 hex 非法
*********************************************************************************************************/
static int load_frame_attr(struct custom_frame_attr *f, cJSON *pFrame)
{
	cJSON *pItem;
	int n;

	frame_attr_defaults(f);
	if (pFrame == NULL)
		return 0;

	pItem = cJSON_GetObjectItem(pFrame, "expect_len");
	if (pItem)
		f->expect_len = (uint16_t)pItem->valueint;

	pItem = cJSON_GetObjectItem(pFrame, "frame_header");
	if (pItem && pItem->valuestring) {
		n = custom_parse_hex_bytes(pItem->valuestring, f->frame_header, CUSTOM_HEADER_MAX_LEN);
		if (n < 0)
			return -1;
		f->frame_header_len = (uint16_t)n;
	}

	pItem = cJSON_GetObjectItem(pFrame, "checksum");
	if (pItem && pItem->valuestring)
		f->checksum = (uint8_t)parse_checksum_algo(pItem->valuestring);

	pItem = cJSON_GetObjectItem(pFrame, "checksum_from");
	if (pItem)
		f->checksum_from = (int16_t)pItem->valueint;

	pItem = cJSON_GetObjectItem(pFrame, "checksum_to");
	if (pItem)
		f->checksum_to = (int16_t)pItem->valueint;

	pItem = cJSON_GetObjectItem(pFrame, "checksum_store");
	if (pItem)
		f->checksum_store = (int16_t)pItem->valueint;

	pItem = cJSON_GetObjectItem(pFrame, "checksum_endian");
	if (pItem && pItem->valuestring) {
		if (streq_ci(pItem->valuestring, "be"))
			f->checksum_endian_le = 0;
		else
			f->checksum_endian_le = 1;
	}

	pItem = cJSON_GetObjectItem(pFrame, "addr_offset");
	if (pItem)
		f->addr_offset = (int16_t)pItem->valueint;

	pItem = cJSON_GetObjectItem(pFrame, "addr_value");
	if (pItem)
		f->addr_value = (uint8_t)pItem->valueint;

	return 0;
}

/*********************************************************************************************************
 ** Function name:		 get_frame_object
** Descriptions:		 获取帧匹配 JSON 节点
                         优先 attr.frame；否则使用上位机 Dev 属性写入的 attr.extend_param
** input parameters:	 pAttr	attr 对象
** output parameters:	 NONE
** Returned value: 	 JSON 对象指针，可能为 NULL
*********************************************************************************************************/
static cJSON *get_frame_object(cJSON *pAttr)
{
	cJSON *p;

	p = cJSON_GetObjectItem(pAttr, "frame");
	if (p)
		return p;
	return cJSON_GetObjectItem(pAttr, "extend_param");
}

/*********************************************************************************************************
 ** Function name:		 load_pkg_request
** Descriptions:		 装载请求报文列表 pkg_request.pkt_req_info
                         每条含 pkt_id 与原文十六进制 tx（直发，无占位符）
** input parameters:	 pCfg	设备属性
                         pArray	pkt_req_info 数组
** output parameters:	 pCfg->pkg_request
** Returned value: 	 0 成功；-1 失败
*********************************************************************************************************/
static int load_pkg_request(struct config_devices_attr *pCfg, cJSON *pArray)
{
	int i, size, n;
	cJSON *pObject, *pItem;

	size = cJSON_GetArraySize(pArray);
	if (size > CONFIG_MAX_REQUEST_PKT_LEN)
		size = CONFIG_MAX_REQUEST_PKT_LEN;
	pCfg->pkg_request.pkt_num = (uint16_t)size;

	for (i = 0; i < size; i++) {
		pObject = cJSON_GetArrayItem(pArray, i);
		if (!pObject)
			return -1;

		pItem = cJSON_GetObjectItem(pObject, "pkt_id");
		if (!pItem)
			return -1;
		pCfg->pkg_request.pkt_req_info[i].pkt_id = (uint16_t)pItem->valueint;

		pItem = cJSON_GetObjectItem(pObject, "tx");
		if (!pItem || !pItem->valuestring)
			return -1;
		/* 原文解析为二进制，发送时原样写出 */
		n = custom_parse_hex_bytes(pItem->valuestring,
					   pCfg->pkg_request.pkt_req_info[i].tx,
					   CUSTOM_TX_MAX_LEN);
		if (n <= 0)
			return -1;
		pCfg->pkg_request.pkt_req_info[i].tx_len = (uint16_t)n;

		printf("  pkt_id:%d tx_len:%d\n",
		       pCfg->pkg_request.pkt_req_info[i].pkt_id,
		       pCfg->pkg_request.pkt_req_info[i].tx_len);
	}
	return 0;
}

/*********************************************************************************************************
 ** Function name:		 load_nodes
** Descriptions:		 装载测点 node.node_attr
                         关键字段：pkt_id、dataoffset（帧绝对字节）、datatype（与 Modbus 一致）
** input parameters:	 pCfg	设备属性
                         num	node_num
                         pArray	node_attr 数组
** output parameters:	 pCfg->node_attr
** Returned value: 	 0 成功；-1 失败
*********************************************************************************************************/
static int load_nodes(struct config_devices_attr *pCfg, int num, cJSON *pArray)
{
	int i, size;
	cJSON *pObject, *pItem;

	size = cJSON_GetArraySize(pArray);
	if (size != num) {
		printf("customProto cfg error: node_num != node_array num!\n");
		return -1;
	}
	if (num > CFG_MAX_POINT)
		num = CFG_MAX_POINT;
	pCfg->node_num = (uint16_t)num;

	for (i = 0; i < num; i++) {
		pObject = cJSON_GetArrayItem(pArray, i);
		if (!pObject)
			return -1;

		pItem = cJSON_GetObjectItem(pObject, "id");
		if (!pItem) {
			printf("customProto cfg error: node[%d] missing id\n", i);
			return -1;
		}
		pCfg->node_attr[i].id = (uint16_t)pItem->valueint;

		pItem = cJSON_GetObjectItem(pObject, "datatype");
		if (!pItem) {
			printf("customProto cfg error: node[%d] missing datatype\n", i);
			return -1;
		}
		pCfg->node_attr[i].datatype = (uint8_t)pItem->valueint;

		/* pkt_id==0：CommStatus 等非解析点 */
		pItem = cJSON_GetObjectItem(pObject, "pkt_id");
		if (pItem)
			pCfg->node_attr[i].pkt_id = (uint8_t)pItem->valueint;
		else
			pCfg->node_attr[i].pkt_id = 0;

		pItem = cJSON_GetObjectItem(pObject, "bitfield");
		pCfg->node_attr[i].bitfield = pItem ? (uint8_t)pItem->valueint : 0;

		pItem = cJSON_GetObjectItem(pObject, "bitnum");
		pCfg->node_attr[i].bitnum = pItem ? (uint8_t)pItem->valueint : 1;

		pItem = cJSON_GetObjectItem(pObject, "datattr");
		if (!pItem) {
			printf("customProto cfg error: node[%d] missing datattr\n", i);
			return -1;
		}
		pCfg->node_attr[i].flag = (uint8_t)pItem->valueint;

		/* 帧偏移：相对整帧第 0 字节 */
		pItem = cJSON_GetObjectItem(pObject, "dataoffset");
		pCfg->node_attr[i].dataoffset = pItem ? (uint16_t)pItem->valueint : 0;

		pItem = cJSON_GetObjectItem(pObject, "catagory");
		pCfg->node_attr[i].catagory = pItem ? (uint32_t)pItem->valueint : 0;

		pItem = cJSON_GetObjectItem(pObject, "kfactor");
		if (!pItem) {
			printf("customProto cfg error: node[%d] missing kfactor\n", i);
			return -1;
		}
		pCfg->node_attr[i].kfactor = (float)pItem->valuedouble;

		pItem = cJSON_GetObjectItem(pObject, "bfactor");
		pCfg->node_attr[i].bfactor = pItem ? (float)pItem->valuedouble : 0;

		pItem = cJSON_GetObjectItem(pObject, "pt_flag1");
		pCfg->node_attr[i].pt_flag1 = pItem ? (float)pItem->valuedouble : 1;

		pItem = cJSON_GetObjectItem(pObject, "ct_flag2");
		pCfg->node_attr[i].ct_flag2 = pItem ? (float)pItem->valuedouble : 1;

		pItem = cJSON_GetObjectItem(pObject, "range_checked");
		pCfg->node_attr[i].range_checked = pItem ? (uint8_t)pItem->valueint : 0;

		pItem = cJSON_GetObjectItem(pObject, "range_min");
		pCfg->node_attr[i].range_min = pItem ? pItem->valueint : 0;

		pItem = cJSON_GetObjectItem(pObject, "range_max");
		pCfg->node_attr[i].range_max = pItem ? pItem->valueint : 0;
	}
	return 0;
}

/*********************************************************************************************************
 ** Function name:		 custom_cfg_load
** Descriptions:		 从设备点表 JSON 文件装载 customProto 配置到 RAM 结构
                         结构：attr.frame/extend_param + pkg_request + node
                         control/alarm 本驱动不强制要求（仅抄读）
** input parameters:	 pCfg	输出设备属性
                         file	JSON 路径
** output parameters:	 pCfg
** Returned value: 	 0 成功；-1 失败
*********************************************************************************************************/
int custom_cfg_load(struct config_devices_attr *pCfg, char *file)
{
	char *p;
	int size, ret;
	cJSON *pJson, *pAttr, *pGroup, *pItem;

	memset(pCfg, 0, sizeof(*pCfg));
	frame_attr_defaults(&pCfg->frame);

	p = jsonfile_alloc(file, &size);
	if (p == NULL)
		return -1;
	if (jsonfile_read(file, p, size) == NULL)
		goto __err_free;

	pJson = cJSON_Parse(p);
	if (!pJson)
		goto __err_free;

	pAttr = cJSON_GetObjectItem(pJson, "attr");
	if (!pAttr)
		goto __err_json;

	pItem = cJSON_GetObjectItem(pAttr, "checked");
	pCfg->checked = pItem ? (uint8_t)pItem->valueint : 1;

	/* 帧匹配：默认 NONE，直接解析 */
	if (load_frame_attr(&pCfg->frame, get_frame_object(pAttr)) < 0)
		goto __err_json;

	pGroup = cJSON_GetObjectItem(pAttr, "pkg_request");
	if (!pGroup)
		goto __err_json;
	pGroup = cJSON_GetObjectItem(pGroup, "pkt_req_info");
	if (!pGroup)
		goto __err_json;
	ret = load_pkg_request(pCfg, pGroup);
	if (ret < 0)
		goto __err_json;

	pGroup = cJSON_GetObjectItem(pAttr, "node");
	if (!pGroup)
		goto __err_json;
	pItem = cJSON_GetObjectItem(pGroup, "node_num");
	if (!pItem)
		goto __err_json;
	pGroup = cJSON_GetObjectItem(pGroup, "node_attr");
	if (!pGroup)
		goto __err_json;
	ret = load_nodes(pCfg, pItem->valueint, pGroup);
	if (ret < 0)
		goto __err_json;

	cJSON_Delete(pJson);
	free(p);
	return 0;

__err_json:
	cJSON_Delete(pJson);
__err_free:
	free(p);
	return -1;
}

/*********************************************************************************************************
 ** Function name:		 custom_cfg_node_attr_size
** Descriptions:		 获取该设备测点数量
** input parameters:	 pDevAttr	设备属性
** output parameters:	 NONE
** Returned value: 	 测点数
*********************************************************************************************************/
int32_t custom_cfg_node_attr_size(struct config_devices_attr *pDevAttr)
{
	return pDevAttr->node_num;
}

/*********************************************************************************************************
 ** Function name:		 custom_cfg_node_attr
** Descriptions:		 按数组下标获取测点属性指针
** input parameters:	 pDevAttr	设备属性
                         id			下标（0..node_num-1）
** output parameters:	 NONE
** Returned value: 	 测点指针；越界返回 NULL
*********************************************************************************************************/
struct config_node_attr *custom_cfg_node_attr(struct config_devices_attr *pDevAttr, uint16_t id)
{
	if (id >= CFG_MAX_POINT)
		return NULL;
	return &pDevAttr->node_attr[id];
}
