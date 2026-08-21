/*
 * File      : custom_port.c
 * Descriptions: customProto 动态库入口（仅抄读一问一答）
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "custom_cfg.h"
#include "custom_process.h"
#include "pkt.h"
#include "db_node_fops.h"
/* list.h 已由 sampling_cfg.h 引入；勿再 include（slist_* 在 list.h 的 #endif 之后，会重定义） */

/*********************************************************************************************************
 ** Function name:		 dump_tx_bytes
** Descriptions:		 打印即将入队的原文发送帧（便于确认是否挂到 Poll 队列）
** input parameters:	 pkt_id, buf, len
** Returned value: 	 NONE
*********************************************************************************************************/
static void dump_tx_bytes(uint16_t pkt_id, const uint8_t *buf, uint16_t len)
{
	uint16_t i;

	printf("customProto: queue pkt_id=%u len=%u tx:", pkt_id, len);
	for (i = 0; i < len && i < 64; i++)
		printf(" %02X", buf[i]);
	if (len > 64)
		printf(" ...");
	printf("\n");
	fflush(stdout);
}

/*********************************************************************************************************
 ** Function name:		 custom_pkt_request_list_create
** Descriptions:		 本设备请求帧装载
                         所有问答式的协议驱动，均需要调用pkt.c 中的接口，把协议生成的发送帧，append到发送链表队列中pkt_request_list_static_append
                         每调用一次pkt_request_list_static_append，即在本设备中增加发送帧链表节点
                         customProto：直接使用点表中的原文十六进制 tx，不做占位符展开
** input parameters:	 pDevInfoAttr	设备信息（通道/逻辑号等）
                         pDevAttr		本驱动私有点表属性
** output parameters:	 NONE
** Returned value: 	 >=0 成功入队帧数；-1 全部失败或无可发帧
*********************************************************************************************************/
static int custom_pkt_request_list_create(struct config_devices_info_attr *pDevInfoAttr,
					  struct config_devices_attr *pDevAttr)
{
	int i;
	int queued = 0;
	uint16_t npkt;

	/* 显式初始化发送链表头（calloc 虽已清零，与主程序设备头初始化保持一致） */
	slist_init(&pDevInfoAttr->pkt_request_list_head.slist);

	/* 请求帧数量上限保护，与框架 CONFIG_MAX_REQUEST_PKT_LEN 一致 */
	npkt = pDevAttr->pkg_request.pkt_num;
	if (npkt > CONFIG_MAX_REQUEST_PKT_LEN)
		npkt = CONFIG_MAX_REQUEST_PKT_LEN;

	printf("customProto: create request list, pkt_num=%u\n", npkt);
	fflush(stdout);

	for (i = 0; i < npkt; i++) {
		struct config_pkt_request_info *pinfo = &pDevAttr->pkg_request.pkt_req_info[i];
		int ret;

		if (pinfo->tx_len == 0) {
			printf("customProto: skip pkt_id=%u tx_len=0\n", pinfo->pkt_id);
			continue;
		}

		dump_tx_bytes(pinfo->pkt_id, pinfo->tx, pinfo->tx_len);

		/* private 指向本条请求信息，收包回调用 pkt_id 匹配测点 */
		ret = pkt_request_list_static_append(pDevInfoAttr,
						    pinfo->tx,
						    pinfo->tx_len,
						    fnReceiveCallback,
						    NULL,
						    pinfo);
		if (ret < 0) {
			printf("customProto: append FAIL pkt_id=%u (malloc?)\n", pinfo->pkt_id);
			fflush(stdout);
			continue;
		}
		queued++;
	}

	printf("customProto: queued %d frame(s), list_len=%u\n",
	       queued,
	       slist_len(&pDevInfoAttr->pkt_request_list_head.slist));
	fflush(stdout);

	return (queued > 0) ? queued : -1;
}

/*********************************************************************************************************
 ** Function name:		 lib_create
** Descriptions:		 本驱动初始化（动态库导出符号，由 sampling_channelpoll dlsym 调用）
                         1. 绑定实时库
                         2. 申请设备私有点表结构
                         3. 从 cfg_file 加载 JSON
                         4. 装载发送帧队列
** input parameters:	 pDevInfoAttr	通道设备信息（含 cfg_file / pVmem）
** output parameters:	 NONE
** Returned value: 	 0 成功；-1 失败
*********************************************************************************************************/
int32_t lib_create(struct config_devices_info_attr *pDevInfoAttr)
{
	struct config_devices_attr *pDevAttr;

	/* 0. 实时数据库绑定 */
	DataMgmtFops.init(pDevInfoAttr->pVmem);

	/*
		1. 申请当前设备点表属性管理空间
		   注意：驱动内禁止用全局/静态变量保存设备状态，多设备共用一个 so
	*/
	pDevAttr = calloc(1, sizeof(struct config_devices_attr));
	if (!pDevAttr) {
		printf("customProto: calloc devices_attr fail, sizeof=%zu\n",
		       sizeof(struct config_devices_attr));
		fflush(stdout);
		return -1;
	}

	/* 2. 保存当前点表到设备信息管理中 */
	pDevInfoAttr->private = pDevAttr;
	/* 仅一问一答，不使用 Flow 回调 */
	pDevInfoAttr->pfnFlowCallback = NULL;

	/* 3. 从 json 加载当前设备的配置 */
	if (custom_cfg_load(pDevAttr, pDevInfoAttr->cfg_file) < 0) {
		free(pDevAttr);
		pDevInfoAttr->private = NULL;
		printf("customProto: load cfg fail: %s\n", pDevInfoAttr->cfg_file);
		fflush(stdout);
		return -1;
	}

	/* 4. 根据配置把原文发送帧压入本设备发送队列 */
	if (custom_pkt_request_list_create(pDevInfoAttr, pDevAttr) < 0) {
		printf("customProto: NO frame queued — Poll 将不发 485（空队列会把 CommStatus 置 1）\n");
		fflush(stdout);
		return -1;
	}

	printf("customProto: lib_create OK\n");
	fflush(stdout);
	return 0;
}

/*********************************************************************************************************
 ** Function name:		 lib_destroy
** Descriptions:		 本驱动销毁，释放请求帧链表与私有点表内存
** input parameters:	 pDevInfoAttr	设备信息
** Returned value: 	 NONE
*********************************************************************************************************/
void lib_destroy(struct config_devices_info_attr *pDevInfoAttr)
{
	/* 释放请求报文申请的内存 */
	pkt_request_list_all_destory(pDevInfoAttr);

	/* 释放点表申请的内存 */
	if (pDevInfoAttr->private) {
		free(pDevInfoAttr->private);
		pDevInfoAttr->private = NULL;
	}
}

/*********************************************************************************************************
 ** Function name:		 lib_control
** Descriptions:		 下发控制组帧（动态库导出符号）
                         customProto V1 仅支持抄读，不支持遥控/遥调；返回 0 表示无可发送控制帧
** Returned value: 	 <=0 表示不发送；>0 为组好的控制帧长度
*********************************************************************************************************/
int32_t lib_control(struct config_devices_info_attr *pDevInfoAttr, void *pargs, void *pbuffer, uint32_t len)
{
	(void)pDevInfoAttr;
	(void)pargs;
	(void)pbuffer;
	(void)len;
	return 0;
}
