#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>

#include "sampling_opc_ua_process.h"
#include "config_devices.h"
#include "opc_ua_client.h"
#include "db_node_fops.h"
#include "db_struct.h"

#define OPC_UA_TICK_US          100000
#define OPC_UA_CTRL_INTERVAL_US 10000
#define OPC_UA_RECONNECT_SEC    1
#define OPC_UA_COMM_STATUS_IDX  0

/*********************************************************************************************************
** Function name:       opc_ua_now_ms
** Descriptions:        获取单调递增的毫秒时间戳，用于轮询周期计时
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      当前毫秒时间
*********************************************************************************************************/
static uint32_t opc_ua_now_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec * 1000U + tv.tv_usec / 1000U);
}

/*********************************************************************************************************
** Function name:       opc_ua_is_comm_status_node
** Descriptions:        判断是否为通讯状态虚拟点（点表 index=0，对应 db_link index=0）
** input parameters:    pNode 点表节点
** output parameters:   NONE
** Returned value:      true 为 CommStatus 虚拟点
*********************************************************************************************************/
static bool opc_ua_is_comm_status_node(const struct config_opcua_node_attr *pNode)
{
    if (pNode == NULL)
        return false;

    return (pNode->id == OPC_UA_COMM_STATUS_IDX);
}

/*********************************************************************************************************
** Function name:       opc_ua_set_comm_status
** Descriptions:        写单设备通讯状态到实时库 index=0（1 在线 / 0 离线）
** input parameters:    pDev   设备属性
**                      value  通讯状态值
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void opc_ua_set_comm_status(struct config_devices_info_attr *pDev, double value)
{
    int ret;

    ret = DataMgmtFops.writepoint(pDev->channel_id, pDev->logic_id,
                                  OPC_UA_COMM_STATUS_IDX, &value);
    if (ret != 0) {
        printf("opc_ua comm status write fail ch=%d dev=%d val=%.0f\n",
               pDev->channel_id, pDev->logic_id, value);
    }
}

/*********************************************************************************************************
** Function name:       opc_ua_set_all_comm_status
** Descriptions:        写本通道下所有设备通讯状态（连接/断开时调用，同 IEC104 connectionHandler）
** input parameters:    pMgr   通道管理器
**                      value  通讯状态值
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void opc_ua_set_all_comm_status(struct opc_ua_mgr_t *pMgr, double value)
{
    struct config_devices_info_attr *pDev;

    slist_for_each_entry(pDev, &pMgr->ChannelAttr.devices_list_head.slist, slist)
        opc_ua_set_comm_status(pDev, value);
}

/*********************************************************************************************************
** Function name:       opc_ua_pick_auth
** Descriptions:        从设备扩展参数中选取首个非空用户名/密码，用于 OPC UA 会话认证
** input parameters:    pMgr      通道管理器
** output parameters:   password  对应密码指针
** Returned value:      用户名字符串，无认证时返回空串
*********************************************************************************************************/
static const char *opc_ua_pick_auth(struct opc_ua_mgr_t *pMgr, const char **password)
{
    struct config_devices_info_attr *pDev;

    slist_for_each_entry(pDev, &pMgr->ChannelAttr.devices_list_head.slist, slist) {
        if (pDev->DevAttr.username[0] != '\0') {
            *password = pDev->DevAttr.password;
            return pDev->DevAttr.username;
        }
    }

    *password = "";
    return "";
}

/*********************************************************************************************************
** Function name:       opc_ua_pick_poll_interval_ms
** Descriptions:        取本通道首个有效设备的轮询间隔，缺省 1000ms
** input parameters:    pMgr  通道管理器
** output parameters:   NONE
** Returned value:      轮询间隔（毫秒）
*********************************************************************************************************/
static uint16_t opc_ua_pick_poll_interval_ms(struct opc_ua_mgr_t *pMgr)
{
    struct config_devices_info_attr *pDev;
    uint16_t interval = 1000;

    slist_for_each_entry(pDev, &pMgr->ChannelAttr.devices_list_head.slist, slist) {
        if (pDev->DevAttr.poll_interval_ms > 0) {
            interval = pDev->DevAttr.poll_interval_ms;
            break;
        }
    }

    return interval;
}

/*********************************************************************************************************
** Function name:       opc_ua_poll_devices
** Descriptions:        轮询读各设备 OPC UA 测点并写入实时库；跳过 index=0 通讯状态虚拟点
** input parameters:    pMgr  通道管理器
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void opc_ua_poll_devices(struct opc_ua_mgr_t *pMgr)
{
    struct config_devices_info_attr *pDev;
    struct config_opcua_node_attr *pNode;
    UA_StatusCode status;
    double value;
    int i;

    slist_for_each_entry(pDev, &pMgr->ChannelAttr.devices_list_head.slist, slist) {
        if (!pDev->DevAttr.checked)
            continue;

        for (i = 0; i < pDev->DevAttr.node_num; i++) {
            pNode = &pDev->DevAttr.node_attr[i];

            /* CommStatus 由连接状态维护，不从 OPC UA 服务器读取 */
            if (opc_ua_is_comm_status_node(pNode))
                continue;

            if (!opc_ua_nodeid_is_valid(pNode->ns, pNode->n, pNode->s))
                continue;

            status = opc_ua_client_read_double(pMgr->client,
                                               pNode->ns, pNode->n, pNode->s,
                                               &value);
            if (status != UA_STATUSCODE_GOOD) {
                if (pMgr->poptarg->dump || pDev->DevAttr.debug_enabled) {
                    printf("opc_ua read fail ch=%d dev=%d id=%u status=0x%08x\n",
                           pDev->channel_id, pDev->logic_id, pNode->id, status);
                }
                continue;
            }

            value *= pNode->kfactor;
            DataMgmtFops.writepoint(pDev->channel_id, pDev->logic_id, pNode->id, &value);
        }
    }
}

/*********************************************************************************************************
** Function name:       opc_ua_process_control
** Descriptions:        处理单条 DO/AO 控制命令：写 OPC UA 节点并回写控制点实时值
** input parameters:    pMgr   通道管理器
**                      pctrl  控制队列条目（db_linkid 含通道/设备/类型/索引）
** output parameters:   NONE
** Returned value:      0 成功，-1 失败
*********************************************************************************************************/
static int opc_ua_process_control(struct opc_ua_mgr_t *pMgr, struct db_spl_channel_ctr_t *pctrl)
{
    struct config_devices_info_attr *pDev;
    struct config_opcua_control_attr *pCtrlAttr;
    uint32_t ctrlType;
    uint8_t ctrlIndex;
    uint8_t devNo;
    UA_StatusCode status;
    double writeVal;

    ctrlType = DB_CONVENT_ID_2_TYPE(pctrl->db_linkid);
    ctrlIndex = DB_CONVENT_ID_2_INDEX(pctrl->db_linkid);
    devNo = DB_CONVENT_ID_2_DEVNO(pctrl->db_linkid);

    slist_for_each_entry(pDev, &pMgr->ChannelAttr.devices_list_head.slist, slist) {
        if (pDev->logic_id != devNo)
            continue;

        pCtrlAttr = opc_ua_cfg_control_attr(&pDev->DevAttr, ctrlIndex);
        if (pCtrlAttr == NULL) {
            printf("opc_ua_process_control: control attr not found id=%u\n", ctrlIndex);
            return -1;
        }

        if (ctrlType == DB_TYPE_DO) {
            writeVal = (pctrl->value != 0.0) ? 1.0 : 0.0;
            status = opc_ua_client_write_value(pMgr->client,
                                               pCtrlAttr->ns, pCtrlAttr->n, pCtrlAttr->s,
                                               3, writeVal);
        } else if (ctrlType == DB_TYPE_AO) {
            writeVal = pctrl->value / pCtrlAttr->kfactor;
            status = opc_ua_client_write_value(pMgr->client,
                                               pCtrlAttr->ns, pCtrlAttr->n, pCtrlAttr->s,
                                               4, writeVal);
        } else {
            printf("opc_ua_process_control: unsupported type 0x%03x\n", ctrlType);
            return -1;
        }

        if (status != UA_STATUSCODE_GOOD) {
            printf("opc_ua write fail dev=%d ctrl=%u status=0x%08x\n",
                   devNo, ctrlIndex, status);
            return -1;
        }

        DataMgmtFops.writectrlpoint(pDev->channel_id, pDev->logic_id, ctrlIndex, &pctrl->value);

        if (pMgr->poptarg->dump || pDev->DevAttr.debug_enabled) {
            printf("opc_ua control ok dev=%d ctrl=%u value=%.3f\n",
                   devNo, ctrlIndex, pctrl->value);
        }

        return 0;
    }

    printf("opc_ua_process_control: device %u not found\n", devNo);
    return -1;
}

/*********************************************************************************************************
** Function name:       opc_ua_drain_ctrl_queue
** Descriptions:        消费本通道控制队列中待下发的 DO/AO 命令
** input parameters:    pMgr  通道管理器
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void opc_ua_drain_ctrl_queue(struct opc_ua_mgr_t *pMgr)
{
    struct db_spl_realtime_data_tbl_t *pDB = pMgr->pVmem;
    struct db_spl_channel_ctr_t *pctrl;
    uint8_t channelno;

    channelno = pMgr->ChannelNo & 0x3f;

    while (1) {
        pctrl = db_ctrl_active_first(pDB, channelno);
        if (pctrl == NULL || pctrl->is_active != 1)
            break;

        opc_ua_process_control(pMgr, pctrl);
        pctrl->is_active = 0;
        usleep(OPC_UA_CTRL_INTERVAL_US);
    }
}

/*********************************************************************************************************
** Function name:       opc_ua_channel_start
** Descriptions:        初始化 OPC UA 客户端并绑定实时库
** input parameters:    pRTDb  实时库 mmap 指针
**                      pMgr   通道管理器
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void opc_ua_channel_start(struct db_spl_realtime_data_tbl_t *pRTDb, struct opc_ua_mgr_t *pMgr)
{
    pMgr->pVmem = pRTDb;
    pMgr->client = opc_ua_client_create();
    pMgr->connected = false;
}

/*********************************************************************************************************
** Function name:       opc_ua_try_connect
** Descriptions:        建立 OPC UA 会话；成功时写通讯状态在线（index=0）
** input parameters:    pMgr  通道管理器
** output parameters:   NONE
** Returned value:      true 已连接，false 连接失败
*********************************************************************************************************/
static bool opc_ua_try_connect(struct opc_ua_mgr_t *pMgr)
{
    char endpoint[128];
    const char *username;
    const char *password;
    UA_StatusCode status;

    if (pMgr->connected)
        return true;

    snprintf(endpoint, sizeof(endpoint), "opc.tcp://%s:%d",
             pMgr->ChannelAttr.working_attr.dst_url,
             pMgr->ChannelAttr.working_attr.dst_port);

    username = opc_ua_pick_auth(pMgr, &password);

    status = opc_ua_client_connect(pMgr->client, endpoint, username, password);
    if (status != UA_STATUSCODE_GOOD) {
        printf("opc_ua connect fail: %s status=0x%08x\n", endpoint, status);
        pMgr->connected = false;
        return false;
    }

    printf("opc_ua connected: %s\n", endpoint);
    opc_ua_set_all_comm_status(pMgr, 1.0);
    pMgr->connected = true;
    return true;
}

/*********************************************************************************************************
** Function name:       opc_ua_channel_process
** Descriptions:        通道主循环：连接/重连、iterate、控制队列、周期轮询采集
** input parameters:    pMgr  通道管理器
** output parameters:   NONE
** Returned value:      0 正常（实际不返回），-1 客户端未创建
*********************************************************************************************************/
int opc_ua_channel_process(struct opc_ua_mgr_t *pMgr)
{
    uint32_t lastPollMs = 0;
    uint16_t pollIntervalMs;
    UA_StatusCode runStatus;

    if (pMgr->client == NULL)
        return -1;

    pollIntervalMs = opc_ua_pick_poll_interval_ms(pMgr);

    while (1) {
        if (!opc_ua_try_connect(pMgr)) {
            sleep(OPC_UA_RECONNECT_SEC);
            continue;
        }

        lastPollMs = opc_ua_now_ms();

        while (pMgr->connected) {
            runStatus = UA_Client_run_iterate(pMgr->client, 0);
            if (runStatus != UA_STATUSCODE_GOOD) {
                printf("opc_ua iterate fail status=0x%08x\n", runStatus);
                opc_ua_client_disconnect(pMgr->client);
                opc_ua_set_all_comm_status(pMgr, 0.0);
                pMgr->connected = false;
                break;
            }

            opc_ua_drain_ctrl_queue(pMgr);

            {
                uint32_t nowMs = opc_ua_now_ms();
                if ((uint32_t)(nowMs - lastPollMs) >= pollIntervalMs) {
                    opc_ua_poll_devices(pMgr);
                    lastPollMs = nowMs;
                }
            }

            usleep(OPC_UA_TICK_US);
        }
    }

    return 0;
}
