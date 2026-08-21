#ifndef __OPC_UA_CFG_H__
#define __OPC_UA_CFG_H__

#include <stdint.h>
#include <stdbool.h>

#define OPCUA_CFG_MAX_POINT                     2048
#define OPCUA_CFG_MAX_CTRLNODESIZE_PER_DEVICE   8
#define OPCUA_CFG_NODEID_STR_LEN                  128

struct config_opcua_node_attr
{
    uint16_t    id;
    uint8_t     datattr;
    uint32_t    catagory;
    double      kfactor;
    uint16_t    ns;
    int32_t     n;
    char        s[OPCUA_CFG_NODEID_STR_LEN];
};

struct config_opcua_control_attr
{
    uint8_t     id;
    uint8_t     datattr;
    uint16_t    ns;
    int32_t     n;
    char        s[OPCUA_CFG_NODEID_STR_LEN];
    double      kfactor;
};

struct config_opcua_devices_attr
{
    uint8_t                         checked;
    uint16_t                        node_num;
    struct config_opcua_node_attr   *node_attr;
    uint8_t                         control_num;
    struct config_opcua_control_attr control_attr[OPCUA_CFG_MAX_CTRLNODESIZE_PER_DEVICE];
    uint16_t                        poll_interval_ms;
    char                            username[32];
    char                            password[32];
    bool                            debug_enabled;
};

int opc_ua_cfg_load(struct config_opcua_devices_attr *pCfg, char *file);
int32_t opc_ua_cfg_node_attr_size(struct config_opcua_devices_attr *pDevAttr);
struct config_opcua_node_attr *opc_ua_cfg_node_attr(struct config_opcua_devices_attr *pDevAttr, uint8_t id);
int32_t opc_ua_cfg_control_attr_size(struct config_opcua_devices_attr *pDevAttr);
struct config_opcua_control_attr *opc_ua_cfg_control_attr(struct config_opcua_devices_attr *pDevAttr, uint8_t id);

#endif
