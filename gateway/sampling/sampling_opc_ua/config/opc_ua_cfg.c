#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "opc_ua_cfg.h"

/*********************************************************************************************************
** Function name:       config_jsonfile_alloc
** Descriptions:        按文件大小分配 JSON 读取缓冲区
** input parameters:    file  JSON 文件路径
**                      size  输出文件字节数
** output parameters:   size  文件字节数
** Returned value:      缓冲区指针，失败返回 NULL
*********************************************************************************************************/
static char *config_jsonfile_alloc(char *file, int *size)
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
** Function name:       config_jsonfile_delete
** Descriptions:        释放 JSON 读取缓冲区
** input parameters:    p  缓冲区指针
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void config_jsonfile_delete(char *p)
{
    if (p != NULL)
        free(p);
}

/*********************************************************************************************************
** Function name:       config_jsonfile_read
** Descriptions:        将 JSON 文件内容读入缓冲区
** input parameters:    file  文件路径
**                      p     缓冲区
**                      size  缓冲区大小
** output parameters:   NONE
** Returned value:      成功返回 p，失败返回 NULL
*********************************************************************************************************/
static char *config_jsonfile_read(char *file, char *p, int size)
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
** Function name:       opc_ua_cfg_read_nodeid_fields
** Descriptions:        从 JSON 对象解析 OPC UA NodeId 字段（ns / n / s）
** input parameters:    pObject  JSON 对象
**                      ns       命名空间索引
**                      n        数值型 NodeId
**                      s        字符串型 NodeId 缓冲区
**                      s_len    字符串缓冲区长度
** output parameters:   ns, n, s
** Returned value:      NONE
*********************************************************************************************************/
static void opc_ua_cfg_read_nodeid_fields(cJSON *pObject, uint16_t *ns, int32_t *n, char *s, size_t s_len)
{
    cJSON *pItem;

    pItem = cJSON_GetObjectItem(pObject, "ns");
    *ns = (pItem != NULL) ? (uint16_t)pItem->valueint : 0;

    pItem = cJSON_GetObjectItem(pObject, "n");
    *n = (pItem != NULL) ? (int32_t)pItem->valueint : 0;

    pItem = cJSON_GetObjectItem(pObject, "s");
    if (pItem != NULL && pItem->valuestring != NULL)
        strncpy(s, pItem->valuestring, s_len - 1);
    else
        s[0] = '\0';
}

/*********************************************************************************************************
** Function name:       opc_ua_cfg_node_attr_size
** Descriptions:        获取设备点表节点数量
** input parameters:    pDevAttr  设备点表配置
** output parameters:   NONE
** Returned value:      节点数量
*********************************************************************************************************/
int32_t opc_ua_cfg_node_attr_size(struct config_opcua_devices_attr *pDevAttr)
{
    return pDevAttr->node_num;
}

/*********************************************************************************************************
** Function name:       opc_ua_cfg_node_attr
** Descriptions:        按索引获取设备点表节点属性
** input parameters:    pDevAttr  设备点表配置
**                      id        节点索引
** output parameters:   NONE
** Returned value:      节点属性指针，越界返回 NULL
*********************************************************************************************************/
struct config_opcua_node_attr *opc_ua_cfg_node_attr(struct config_opcua_devices_attr *pDevAttr, uint8_t id)
{
    if (id >= OPCUA_CFG_MAX_POINT)
        return NULL;

    return &pDevAttr->node_attr[id];
}

/*********************************************************************************************************
** Function name:       opc_ua_cfg_control_attr_size
** Descriptions:        获取设备控制点数量
** input parameters:    pDevAttr  设备点表配置
** output parameters:   NONE
** Returned value:      控制点数量
*********************************************************************************************************/
int32_t opc_ua_cfg_control_attr_size(struct config_opcua_devices_attr *pDevAttr)
{
    return pDevAttr->control_num;
}

/*********************************************************************************************************
** Function name:       opc_ua_cfg_control_attr
** Descriptions:        按控制点 id 查找控制点属性
** input parameters:    pDevAttr  设备点表配置
**                      id        控制点 id
** output parameters:   NONE
** Returned value:      控制点属性指针，未找到返回 NULL
*********************************************************************************************************/
struct config_opcua_control_attr *opc_ua_cfg_control_attr(struct config_opcua_devices_attr *pDevAttr, uint8_t id)
{
    int i;

    for (i = 0; i < pDevAttr->control_num; i++) {
        if (pDevAttr->control_attr[i].id == id)
            return &pDevAttr->control_attr[i];
    }

    return NULL;
}

/*********************************************************************************************************
** Function name:       config_json_group_node_read
** Descriptions:        解析设备 JSON 中 node_attr 数组
** input parameters:    pCfg        设备点表配置
**                      num         期望节点数量
**                      pArrayItem  node_attr 数组
** output parameters:   NONE
** Returned value:      0 成功，-1 失败
*********************************************************************************************************/
static int config_json_group_node_read(struct config_opcua_devices_attr *pCfg, int num, cJSON *pArrayItem)
{
    int i;
    int size;
    cJSON *pObject, *pItem;

    size = cJSON_GetArraySize(pArrayItem);
    if (size != num) {
        printf("opc_ua cfg warn: node_num=%d but node_attr size=%d, use array size\n", num, size);
        num = size;
        pCfg->node_num = (uint16_t)size;
    }

    for (i = 0; i < num; i++) {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (pObject == NULL) {
            printf("opc_ua cfg error: node_attr[%d] is null\n", i);
            return -1;
        }

        pItem = cJSON_GetObjectItem(pObject, "id");
        if (pItem == NULL) {
            printf("opc_ua cfg error: node_attr[%d] missing id\n", i);
            return -1;
        }
        pCfg->node_attr[i].id = (uint16_t)pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "datattr");
        if (pItem == NULL) {
            printf("opc_ua cfg error: node_attr[%d] missing datattr\n", i);
            return -1;
        }
        pCfg->node_attr[i].datattr = (uint8_t)pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "catagory");
        pCfg->node_attr[i].catagory = (pItem != NULL) ? (uint32_t)pItem->valueint : 0;

        pItem = cJSON_GetObjectItem(pObject, "kfactor");
        pCfg->node_attr[i].kfactor = (pItem != NULL) ? pItem->valuedouble : 1.0;

        opc_ua_cfg_read_nodeid_fields(pObject,
            &pCfg->node_attr[i].ns,
            &pCfg->node_attr[i].n,
            pCfg->node_attr[i].s,
            sizeof(pCfg->node_attr[i].s));
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       config_json_group_control_read
** Descriptions:        解析设备 JSON 中 control_attr 数组
** input parameters:    pCfg        设备点表配置
**                      num         期望控制点数量
**                      pArrayItem  control_attr 数组
** output parameters:   NONE
** Returned value:      0 成功，-1 失败
*********************************************************************************************************/
static int config_json_group_control_read(struct config_opcua_devices_attr *pCfg, int num, cJSON *pArrayItem)
{
    int i;
    int size;
    cJSON *pObject, *pItem;

    size = cJSON_GetArraySize(pArrayItem);
    if (size != num) {
        printf("opc_ua cfg warn: control_num=%d but control_attr size=%d, use array size\n", num, size);
        num = size;
        pCfg->control_num = (uint8_t)size;
    }

    for (i = 0; i < num; i++) {
        pObject = cJSON_GetArrayItem(pArrayItem, i);
        if (pObject == NULL) {
            printf("opc_ua cfg error: control_attr[%d] is null\n", i);
            return -1;
        }

        pItem = cJSON_GetObjectItem(pObject, "id");
        if (pItem == NULL) {
            printf("opc_ua cfg error: control_attr[%d] missing id\n", i);
            return -1;
        }
        pCfg->control_attr[i].id = (uint8_t)pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "datattr");
        if (pItem != NULL)
            pCfg->control_attr[i].datattr = (uint8_t)pItem->valueint;

        pItem = cJSON_GetObjectItem(pObject, "kfactor");
        pCfg->control_attr[i].kfactor = (pItem != NULL) ? pItem->valuedouble : 1.0;

        opc_ua_cfg_read_nodeid_fields(pObject,
            &pCfg->control_attr[i].ns,
            &pCfg->control_attr[i].n,
            pCfg->control_attr[i].s,
            sizeof(pCfg->control_attr[i].s));
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       opc_ua_cfg_load
** Descriptions:        从设备点表 JSON 文件加载 OPC UA 采集配置
** input parameters:    pCfg  设备点表配置结构
**                      file  点表 JSON 路径
** output parameters:   NONE
** Returned value:      0 成功，-1 失败
*********************************************************************************************************/
int opc_ua_cfg_load(struct config_opcua_devices_attr *pCfg, char *file)
{
    char *p;
    int size;
    int ret;
    cJSON *pJson, *pjsClass, *pjsGroup, *pItem;

    memset(pCfg, 0, sizeof(*pCfg));
    pCfg->poll_interval_ms = 1000;

    p = config_jsonfile_alloc(file, &size);
    if (p == NULL)
        return -1;

    p = config_jsonfile_read(file, p, size);
    if (p == NULL)
        goto __err_free;

    pJson = cJSON_Parse(p);
    if (!pJson) {
        const char *err = cJSON_GetErrorPtr();
        printf("opc_ua cfg error: JSON parse failed: %s\n", err ? err : "unknown");
        goto __err_free;
    }

    pjsClass = cJSON_GetObjectItem(pJson, "attr");
    if (!pjsClass) {
        printf("opc_ua cfg error: missing attr\n");
        goto __err_json_format;
    }

    pItem = cJSON_GetObjectItem(pjsClass, "checked");
    if (!pItem) {
        printf("opc_ua cfg error: missing attr.checked\n");
        goto __err_json_format;
    }
    pCfg->checked = (uint8_t)pItem->valueint;

    pjsGroup = cJSON_GetObjectItem(pjsClass, "node");
    if (!pjsGroup) {
        printf("opc_ua cfg error: missing attr.node\n");
        goto __err_json_format;
    }

    pItem = cJSON_GetObjectItem(pjsGroup, "node_num");
    if (!pItem) {
        printf("opc_ua cfg error: missing attr.node.node_num\n");
        goto __err_json_format;
    }
    pCfg->node_num = (uint16_t)pItem->valueint;

    pjsGroup = cJSON_GetObjectItem(pjsGroup, "node_attr");
    if (!pjsGroup) {
        printf("opc_ua cfg error: missing attr.node.node_attr\n");
        goto __err_json_format;
    }

    pCfg->node_num = (pCfg->node_num > OPCUA_CFG_MAX_POINT) ? OPCUA_CFG_MAX_POINT : pCfg->node_num;
    if (pCfg->node_num > 0) {
        pCfg->node_attr = malloc(sizeof(struct config_opcua_node_attr) * pCfg->node_num);
        if (pCfg->node_attr == NULL)
            goto __err_json_format;
        memset(pCfg->node_attr, 0, sizeof(struct config_opcua_node_attr) * pCfg->node_num);

        ret = config_json_group_node_read(pCfg, pCfg->node_num, pjsGroup);
        if (ret < 0)
            goto __err_json_format;
    } else {
        pCfg->node_num = 0;
    }

    pjsGroup = cJSON_GetObjectItem(pjsClass, "control");
    if (pjsGroup) {
        pItem = cJSON_GetObjectItem(pjsGroup, "control_num");
        if (pItem)
            pCfg->control_num = (uint8_t)pItem->valueint;

        if (pCfg->control_num > OPCUA_CFG_MAX_CTRLNODESIZE_PER_DEVICE)
            pCfg->control_num = OPCUA_CFG_MAX_CTRLNODESIZE_PER_DEVICE;

        pjsGroup = cJSON_GetObjectItem(pjsGroup, "control_attr");
        if (pjsGroup && pCfg->control_num > 0) {
            ret = config_json_group_control_read(pCfg, pCfg->control_num, pjsGroup);
            if (ret < 0)
                goto __err_json_format;
        } else if (pCfg->control_num > 0) {
            printf("opc_ua cfg warn: control_num=%u but control_attr missing, reset to 0\n",
                   pCfg->control_num);
            pCfg->control_num = 0;
        }
    }

    pjsGroup = cJSON_GetObjectItem(pjsClass, "extend_param");
    if (pjsGroup) {
        pItem = cJSON_GetObjectItem(pjsGroup, "poll_interval_ms");
        if (pItem)
            pCfg->poll_interval_ms = (uint16_t)pItem->valueint;
        if (pCfg->poll_interval_ms == 0)
            pCfg->poll_interval_ms = 1000;

        pItem = cJSON_GetObjectItem(pjsGroup, "username");
        if (pItem && pItem->valuestring)
            strncpy(pCfg->username, pItem->valuestring, sizeof(pCfg->username) - 1);

        pItem = cJSON_GetObjectItem(pjsGroup, "password");
        if (pItem && pItem->valuestring)
            strncpy(pCfg->password, pItem->valuestring, sizeof(pCfg->password) - 1);

        pItem = cJSON_GetObjectItem(pjsGroup, "debug_enabled");
        if (pItem)
            pCfg->debug_enabled = pItem->valueint ? true : false;
    }

    cJSON_Delete(pJson);
    config_jsonfile_delete(p);
    return 0;

__err_json_format:
    printf("opc_ua cfg error: invalid format in %s\n", file ? file : "");
    cJSON_Delete(pJson);

__err_free:
    config_jsonfile_delete(p);
    return -1;
}
