#include "opc_ua_client.h"

#include "opc_ua_value.h"

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <stdio.h>
#include <string.h>

/*********************************************************************************************************
** Function name:       opc_ua_nodeid_from_cfg
** Descriptions:        按点表 ns/n/s 构造 OPC UA NodeId（n>=0 数值型，n<0 用字符串 s）
** input parameters:    ns  命名空间索引
**                      n   数值 NodeId，-1 表示使用 s
**                      s   字符串 NodeId
** output parameters:   NONE
** Returned value:      UA_NodeId（字符串型需调用方 UA_NodeId_clear）
*********************************************************************************************************/
UA_NodeId opc_ua_nodeid_from_cfg(uint16_t ns, int32_t n, const char *s)
{
    if (n >= 0)
        return UA_NODEID_NUMERIC(ns, (UA_UInt32)n);

    if (s != NULL && s[0] != '\0')
        return UA_NODEID_STRING_ALLOC(ns, s);

    return UA_NODEID_NULL;
}

/*********************************************************************************************************
** Function name:       opc_ua_nodeid_is_valid
** Descriptions:        判断点表 NodeId 字段是否可用于读写
** input parameters:    ns  命名空间索引
**                      n   数值 NodeId
**                      s   字符串 NodeId
** output parameters:   NONE
** Returned value:      true 有效，false 无效
*********************************************************************************************************/
bool opc_ua_nodeid_is_valid(uint16_t ns, int32_t n, const char *s)
{
    (void)ns;
    if (n >= 0)
        return true;
    return (s != NULL && s[0] != '\0');
}

/*********************************************************************************************************
** Function name:       opc_ua_client_create
** Descriptions:        创建 open62541 客户端并应用默认配置
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      UA_Client 指针，失败返回 NULL
*********************************************************************************************************/
UA_Client *opc_ua_client_create(void)
{
    UA_Client *client = UA_Client_new();
    if (client == NULL)
        return NULL;

    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    return client;
}

/*********************************************************************************************************
** Function name:       opc_ua_client_destroy
** Descriptions:        释放 OPC UA 客户端
** input parameters:    client  客户端指针
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void opc_ua_client_destroy(UA_Client *client)
{
    if (client != NULL)
        UA_Client_delete(client);
}

/*********************************************************************************************************
** Function name:       opc_ua_client_connect
** Descriptions:        连接 OPC UA 服务器（支持匿名或用户名密码）
** input parameters:    client        客户端指针
**                      endpoint_url  端点 URL，如 opc.tcp://host:4840
**                      username      用户名，空串则匿名
**                      password      密码
** output parameters:   NONE
** Returned value:      UA_StatusCode
*********************************************************************************************************/
UA_StatusCode opc_ua_client_connect(UA_Client *client,
                                    const char *endpoint_url,
                                    const char *username,
                                    const char *password)
{
    if (client == NULL || endpoint_url == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if (username != NULL && username[0] != '\0')
        return UA_Client_connectUsername(client, endpoint_url, username, password);

    return UA_Client_connect(client, endpoint_url);
}

/*********************************************************************************************************
** Function name:       opc_ua_client_disconnect
** Descriptions:        断开 OPC UA 会话
** input parameters:    client  客户端指针
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void opc_ua_client_disconnect(UA_Client *client)
{
    if (client != NULL)
        UA_Client_disconnect(client);
}

/*********************************************************************************************************
** Function name:       opc_ua_client_read_double
** Descriptions:        读取 OPC UA 节点值并转为 double
** input parameters:    client  客户端指针
**                      ns,n,s  点表 NodeId
** output parameters:   value   读取结果
** Returned value:      UA_StatusCode
*********************************************************************************************************/
UA_StatusCode opc_ua_client_read_double(UA_Client *client,
                                        uint16_t ns, int32_t n, const char *s,
                                        double *value)
{
    UA_Variant variant;
    UA_NodeId nodeId;
    UA_StatusCode status;

    if (client == NULL || value == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if (!opc_ua_nodeid_is_valid(ns, n, s))
        return UA_STATUSCODE_BADNODEIDINVALID;

    nodeId = opc_ua_nodeid_from_cfg(ns, n, s);
    UA_Variant_init(&variant);

    status = UA_Client_readValueAttribute(client, nodeId, &variant);
    if (n < 0)
        UA_NodeId_clear(&nodeId);

    if (status != UA_STATUSCODE_GOOD) {
        UA_Variant_clear(&variant);
        return status;
    }

    if (!opc_ua_variant_to_double(&variant, value)) {
        UA_Variant_clear(&variant);
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    UA_Variant_clear(&variant);
    return UA_STATUSCODE_GOOD;
}

/*********************************************************************************************************
** Function name:       opc_ua_client_write_value
** Descriptions:        按五遥类型将 double 写入 OPC UA 节点（用于 DO/AO 控制）
** input parameters:    client   客户端指针
**                      ns,n,s   点表 NodeId
**                      datattr  五遥类型
**                      value    待写入值
** output parameters:   NONE
** Returned value:      UA_StatusCode
*********************************************************************************************************/
UA_StatusCode opc_ua_client_write_value(UA_Client *client,
                                        uint16_t ns, int32_t n, const char *s,
                                        uint8_t datattr, double value)
{
    UA_Variant variant;
    UA_NodeId nodeId;
    UA_StatusCode status;

    if (client == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if (!opc_ua_nodeid_is_valid(ns, n, s))
        return UA_STATUSCODE_BADNODEIDINVALID;

    nodeId = opc_ua_nodeid_from_cfg(ns, n, s);
    if (!opc_ua_double_to_variant(value, datattr, &variant)) {
        if (n < 0)
            UA_NodeId_clear(&nodeId);
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    status = UA_Client_writeValueAttribute(client, nodeId, &variant);

    UA_Variant_clear(&variant);
    if (n < 0)
        UA_NodeId_clear(&nodeId);

    return status;
}
