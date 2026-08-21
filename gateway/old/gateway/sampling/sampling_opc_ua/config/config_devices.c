#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include "cJSON.h"
#include "config_devices.h"
#include "sampling_cfg.h"
#include "sampling_opc_ua_process.h"
#include "opc_ua_cfg.h"

/*********************************************************************************************************
** Function name:       config_devices_jsonfile_alloc
** Descriptions:        按文件大小分配 JSON 读取缓冲区
** input parameters:    file  JSON 文件路径
** output parameters:   size  文件字节数
** Returned value:      缓冲区指针，失败返回 NULL
*********************************************************************************************************/
static char *config_devices_jsonfile_alloc(char *file, int *size)
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
** Function name:       config_devices_jsonfile_delete
** Descriptions:        释放 JSON 读取缓冲区
** input parameters:    p  缓冲区指针
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void config_devices_jsonfile_delete(char *p)
{
    if (p != NULL)
        free(p);
}

/*********************************************************************************************************
** Function name:       config_devices_jsonfile_read
** Descriptions:        将 JSON 文件内容读入缓冲区
** input parameters:    file  文件路径
**                      p     缓冲区
**                      size  缓冲区大小
** output parameters:   NONE
** Returned value:      成功返回 p，失败返回 NULL
*********************************************************************************************************/
static char *config_devices_jsonfile_read(char *file, char *p, int size)
{
    int n;
    FILE *fp;

    fp = fopen(file, "rb");
    if (fp == NULL)
        return NULL;

    n = fread(p, 1, size, fp);
    if (n <= 0)
        goto __err_close;

    fclose(fp);
    return p;

__err_close:
    fclose(fp);
    return NULL;
}

/*********************************************************************************************************
** Function name:       config_channel_workingattr
** Descriptions:        解析通道 JSON attr.common 工作参数（端点 URL、轮询间隔等）
** input parameters:    pChannelAttr  通道属性结构
**                      pJson_lay1    attr 节点
** output parameters:   NONE
** Returned value:      0 成功，-1 失败
*********************************************************************************************************/
static int config_channel_workingattr(struct config_channel_attr *pChannelAttr, cJSON *pJson_lay1)
{
    cJSON *pItem;
    cJSON *pJson_lay2, *pJson_lay3;

    pItem = cJSON_GetObjectItem(pJson_lay1, "checked");
    if (!pItem)
        return -1;
    pChannelAttr->working_attr.checked = pItem->valueint;

    pJson_lay2 = cJSON_GetObjectItem(pJson_lay1, "common");
    if (!pJson_lay2)
        return -1;

    pItem = cJSON_GetObjectItem(pJson_lay2, "id");
    if (!pItem)
        return -1;
    pChannelAttr->working_attr.id = pItem->valueint;

    memset(pChannelAttr->working_attr.name, 0, sizeof(pChannelAttr->working_attr.name));

    pItem = cJSON_GetObjectItem(pJson_lay2, "mode");
    if (!pItem)
        return -1;
    strncpy(pChannelAttr->working_attr.mode, pItem->valuestring, sizeof(pChannelAttr->working_attr.mode) - 1);

    pItem = cJSON_GetObjectItem(pJson_lay2, "link");
    if (!pItem)
        return -1;
    strncpy(pChannelAttr->working_attr.link, pItem->valuestring, sizeof(pChannelAttr->working_attr.link) - 1);

    pItem = cJSON_GetObjectItem(pJson_lay2, "dev");
    if (!pItem)
        return -1;
    strncpy(pChannelAttr->working_attr.dev, pItem->valuestring, sizeof(pChannelAttr->working_attr.dev) - 1);
    /* dev 格式 host:port，解析为 OPC UA 连接地址 */
    sscanf(pChannelAttr->working_attr.dev, "%[^:]:%d",
           (char *)&pChannelAttr->working_attr.dst_url[0],
           &pChannelAttr->working_attr.dst_port);

    pItem = cJSON_GetObjectItem(pJson_lay2, "repeat");
    pChannelAttr->working_attr.repeat = pItem ? pItem->valueint : 1;
    if (pChannelAttr->working_attr.repeat == 0)
        pChannelAttr->working_attr.repeat = 1;

    pItem = cJSON_GetObjectItem(pJson_lay2, "inv_time");
    pChannelAttr->working_attr.inv_time = pItem ? pItem->valueint : 500;

    pItem = cJSON_GetObjectItem(pJson_lay2, "RecvTimeOut");
    pChannelAttr->working_attr.RecvTimeOut = pItem ? pItem->valueint : 1000;
    if (pChannelAttr->working_attr.RecvTimeOut == 0)
        pChannelAttr->working_attr.RecvTimeOut = 1000;

    pItem = cJSON_GetObjectItem(pJson_lay2, "framing_time");
    pChannelAttr->working_attr.framing_time = pItem ? pItem->valueint : 0;

    pJson_lay3 = cJSON_GetObjectItem(pJson_lay2, "serial");
    if (pJson_lay3) {
        pItem = cJSON_GetObjectItem(pJson_lay3, "baud");
        pChannelAttr->working_attr.serial_attr.baud = pItem ? pItem->valueint : 9600;

        pItem = cJSON_GetObjectItem(pJson_lay3, "databit");
        pChannelAttr->working_attr.serial_attr.datasize = pItem ? pItem->valueint : 8;

        pItem = cJSON_GetObjectItem(pJson_lay3, "parity");
        pChannelAttr->working_attr.serial_attr.parity = (pItem && pItem->valuestring) ? pItem->valuestring[0] : 'N';

        pItem = cJSON_GetObjectItem(pJson_lay3, "stopbit");
        pChannelAttr->working_attr.serial_attr.stopbit = pItem ? pItem->valueint : 1;
    }

    printf("opc_ua channel: id=%d dev=%s url=%s port=%d\n",
           pChannelAttr->working_attr.id,
           pChannelAttr->working_attr.dev,
           pChannelAttr->working_attr.dst_url,
           pChannelAttr->working_attr.dst_port);

    return 0;
}

/*********************************************************************************************************
** Function name:       config_device_attr_init
** Descriptions:        初始化通道设备链表头
** input parameters:    pChannelAttr  通道属性
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void config_device_attr_init(struct config_channel_attr *pChannelAttr)
{
    slist_init(&pChannelAttr->devices_list_head.slist);
}

/*********************************************************************************************************
** Function name:       config_device_info_attr_new
** Descriptions:        分配设备信息节点
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      设备信息指针，失败返回 NULL
*********************************************************************************************************/
static struct config_devices_info_attr *config_device_info_attr_new(void)
{
    return calloc(1, sizeof(struct config_devices_info_attr));
}

/*********************************************************************************************************
** Function name:       config_device_info_attr_delete
** Descriptions:        释放设备信息节点
** input parameters:    pDevInfoAttr  设备信息指针
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void config_device_info_attr_delete(struct config_devices_info_attr *pDevInfoAttr)
{
    if (pDevInfoAttr != NULL)
        free(pDevInfoAttr);
}

/*********************************************************************************************************
** Function name:       config_device_info_attr_append
** Descriptions:        将设备节点挂入通道设备链表
** input parameters:    pChannelAttr  通道属性
**                      pDevInfoAttr  设备信息
** output parameters:   NONE
** Returned value:      0 成功，-1 失败
*********************************************************************************************************/
static int config_device_info_attr_append(struct config_channel_attr *pChannelAttr,
                                          struct config_devices_info_attr *pDevInfoAttr)
{
    if (pDevInfoAttr == NULL)
        return -1;

    slist_append(&pChannelAttr->devices_list_head.slist, &pDevInfoAttr->slist);
    return 0;
}

/*********************************************************************************************************
** Function name:       config_device_info_attr_parse
** Descriptions:        解析通道 JSON 中单台设备的 logic_id、协议、点表路径等
** input parameters:    pDevInfoAttr  设备信息
**                      pjsGroupDev   devices.attr[] 元素
** output parameters:   NONE
** Returned value:      0 成功，-1 失败
*********************************************************************************************************/
static int config_device_info_attr_parse(struct config_devices_info_attr *pDevInfoAttr, cJSON *pjsGroupDev)
{
    cJSON *pItem;
    char *pstr;

    pItem = cJSON_GetObjectItem(pjsGroupDev, "logic_id");
    if (!pItem)
        return -1;
    pDevInfoAttr->logic_id = (uint8_t)strtoll(pItem->valuestring, &pstr, 10);

    pItem = cJSON_GetObjectItem(pjsGroupDev, "phy_id");
    if (!pItem)
        return -1;
    pDevInfoAttr->phy_id = strtoll(pItem->valuestring, &pstr, 10);

    pItem = cJSON_GetObjectItem(pjsGroupDev, "dll");
    if (pItem)
        strncpy(pDevInfoAttr->dll, pItem->valuestring, sizeof(pDevInfoAttr->dll) - 1);

    pItem = cJSON_GetObjectItem(pjsGroupDev, "protocol");
    if (!pItem)
        return -1;
    strncpy(pDevInfoAttr->protocol, pItem->valuestring, sizeof(pDevInfoAttr->protocol) - 1);

    pItem = cJSON_GetObjectItem(pjsGroupDev, "cfg_file");
    if (!pItem)
        return -1;
    strncpy(pDevInfoAttr->cfg_file, pItem->valuestring, sizeof(pDevInfoAttr->cfg_file) - 1);

    printf("  device logic_id=%d protocol=%s cfg=%s\n",
           pDevInfoAttr->logic_id, pDevInfoAttr->protocol, pDevInfoAttr->cfg_file);

    return 0;
}

/*********************************************************************************************************
** Function name:       config_channel_devlistattr
** Descriptions:        解析通道下设备列表，并加载各设备 OPC UA 点表 JSON
** input parameters:    pChannelAttr  通道属性
**                      pjsGroup      attr 节点
** output parameters:   NONE
** Returned value:      0 成功，-1 失败
*********************************************************************************************************/
static int config_channel_devlistattr(struct config_channel_attr *pChannelAttr, cJSON *pjsGroup)
{
    uint8_t i;
    int ret;
    cJSON *pjsClassDev, *pjsGroupDev;
    struct config_devices_info_attr *pDevInfoAttr;
    uint8_t dev_num;

    pjsClassDev = cJSON_GetObjectItem(pjsGroup, "devices");
    if (!pjsClassDev)
        return -1;

    pjsClassDev = cJSON_GetObjectItem(pjsClassDev, "attr");
    if (!pjsClassDev)
        return -1;

    config_device_attr_init(pChannelAttr);

    dev_num = cJSON_GetArraySize(pjsClassDev);
    for (i = 0; i < dev_num; i++) {
        pjsGroupDev = cJSON_GetArrayItem(pjsClassDev, i);
        if (!pjsGroupDev)
            return -1;

        pDevInfoAttr = config_device_info_attr_new();
        if (!pDevInfoAttr)
            return -1;

        pDevInfoAttr->channel_id = pChannelAttr->working_attr.id;

        ret = config_device_info_attr_parse(pDevInfoAttr, pjsGroupDev);
        if (ret < 0) {
            config_device_info_attr_delete(pDevInfoAttr);
            return -1;
        }

        if (opc_ua_cfg_load(&pDevInfoAttr->DevAttr, pDevInfoAttr->cfg_file) < 0) {
            printf("opc_ua_cfg_load failed: %s\n", pDevInfoAttr->cfg_file);
            config_device_info_attr_delete(pDevInfoAttr);
            return -1;
        }

        config_device_info_attr_append(pChannelAttr, pDevInfoAttr);
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       config_channel_load
** Descriptions:        从通道 JSON 文件加载完整通道配置
** input parameters:    pChannelAttr  通道属性
**                      file          通道配置文件路径
** output parameters:   NONE
** Returned value:      0 成功，-1 失败
*********************************************************************************************************/
static int config_channel_load(struct config_channel_attr *pChannelAttr, char *file)
{
    char *p;
    int size;
    int ret;
    cJSON *pJson, *pJson_lay1;

    if (access(file, F_OK) != 0)
        return -1;

    p = config_devices_jsonfile_alloc(file, &size);
    if (p == NULL)
        return -1;

    p = config_devices_jsonfile_read(file, p, size);
    if (p == NULL)
        goto __err_free;

    pJson = cJSON_Parse(p);
    if (!pJson)
        goto __err_free;

    pJson_lay1 = cJSON_GetObjectItem(pJson, "attr");
    if (!pJson_lay1)
        goto __err_json_format;

    ret = config_channel_workingattr(pChannelAttr, pJson_lay1);
    if (ret < 0)
        goto __err_json_format;

    ret = config_channel_devlistattr(pChannelAttr, pJson_lay1);

    cJSON_Delete(pJson);
    config_devices_jsonfile_delete(p);
    return ret;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    config_devices_jsonfile_delete(p);
    return -1;
}

/*********************************************************************************************************
** Function name:       config_channel_init
** Descriptions:        初始化 OPC UA 通道管理器中的通道配置（对外接口）
** input parameters:    pMgr   通道管理器
**                      file   通道 JSON 路径
** output parameters:   NONE
** Returned value:      0 成功，-1 失败
*********************************************************************************************************/
int config_channel_init(struct opc_ua_mgr_t *pMgr, char *file)
{
    return config_channel_load(&pMgr->ChannelAttr, file);
}
