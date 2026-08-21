#ifndef __SAMPLING_CFG_H__
#define __SAMPLING_CFG_H__

#include <stdint.h>
#include <stdbool.h>
#include "db_struct.h"
#include "list.h"
#include "opc_ua_cfg.h"

struct sampling_optarg_t
{
    uint32_t sn;
    uint8_t  dump;
    uint8_t  channl_no;
    uint16_t tcp_port;
    char     cfg_file[64];
};

struct config_serial_attr
{
    uint32_t baud;
    uint8_t  datasize;
    uint8_t  parity;
    uint8_t  stopbit;
};

struct config_working_attr
{
    uint8_t  checked;
    uint8_t  id;
    char     name[16];
    char     dev[48];
    char     dst_url[32];
    int      dst_port;
    char     mode[16];
    char     link[24];
    uint8_t  repeat;
    uint32_t inv_time;
    uint32_t RecvTimeOut;
    uint32_t framing_time;
    struct config_serial_attr serial_attr;
};

struct config_devices_info_attr
{
    slist_t slist;

    uint8_t  channel_id;
    uint8_t  logic_id;
    uint16_t loop_id;
    uint64_t phy_id;
    char     protocol[96];
    char     dll[96];
    char     cfg_file[96];

    struct db_spl_realtime_data_tbl_t *pVmem;
    struct config_opcua_devices_attr   DevAttr;

    void *ext_func;
    void *ext_args;
};

struct config_channel_attr
{
    struct config_working_attr      working_attr;
    struct config_devices_info_attr devices_list_head;
};

#endif
