#ifndef __DLT698_45_CFG_H__
#define __DLT698_45_CFG_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "cJSON_Utils.h"

#define CONFIG_NODE_NAME_LEN                12
#define CONFIG_MAX_REQUEST_PKT_LEN          16
#define CFG_MAX_POINT                       256
#define CONFIG_MAX_CTRLNODESIZE_PER_DEVICE  8
#define CONFIG_MAX_ALARMRULE_PER_DEVICE     32

struct config_pkt_request_info
{
    uint8_t  code;
    uint32_t reg_no;
    uint16_t reg_num;
};

struct config_pkg_request
{
    uint16_t pkt_num;
    struct config_pkt_request_info pkt_req_info[CONFIG_MAX_REQUEST_PKT_LEN];
};

struct config_node_attr
{
    uint16_t id;
    uint8_t datatype;
    uint8_t leadnum;
    uint8_t code;
    uint8_t datalen;
    uint32_t reg_no;      /* 对698协议复用为OAD */
    uint32_t catagory;
    float kfactor;
    float pt_flag1;
    float ct_flag2;
    uint8_t range_checked;
    int range_min;
    int range_max;
    uint16_t dataoffset;
};

struct config_control_attr
{
    uint8_t  id;
    uint8_t  datatype;
    uint8_t  funcode;
    uint32_t regno;
    uint16_t leadbytenum;
    uint32_t password;
    uint32_t operator;
    uint16_t regvalue;
    uint32_t regno0;
    uint32_t regno1;
    uint16_t regvalue0;
    uint16_t regvalue1;
};

struct alarm_condition
{
    uint32_t pointid;
    char type[12];
    uint32_t level;
    uint8_t method;
    double ref;
    uint8_t ref_per;
    uint8_t Hysteresis;
};

typedef struct config_alarm_attr
{
    uint32_t ruleid;
    char type[16];
    uint8_t mode;
    uint32_t htime;
    struct alarm_condition alarm_cond[2];
} config_alarm_attr, *config_alarm_attr_t;

struct config_devices_attr
{
    uint8_t checked;
    char sn[16];
    char model[16];
    char name[64];
    char function[16];
    struct config_pkg_request pkg_request;
    uint16_t node_num;
    struct config_node_attr node_attr[CFG_MAX_POINT];
    uint8_t control_num;
    struct config_control_attr control_attr[CONFIG_MAX_CTRLNODESIZE_PER_DEVICE];
    uint8_t alarm_num;
    struct config_alarm_attr alarm_attr[CONFIG_MAX_ALARMRULE_PER_DEVICE];
};

int dlt698_45_cfg_load(struct config_devices_attr *pCfg, char *file);
int32_t dlt698_45_cfg_node_attr_size(struct config_devices_attr *pDevAttr);
struct config_node_attr *dlt698_45_cfg_node_attr(struct config_devices_attr *pDevAttr, uint8_t id);
int32_t dlt698_45_cfg_nodectrl_attr_size(struct config_devices_attr *pDevAttr);
struct config_control_attr *dlt698_45_cfg_nodectrl_attr(struct config_devices_attr *pDevAttr, uint8_t id);

#endif
