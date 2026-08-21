/*
 * mb_product_cfg_local.h  — TASO1302 产品私有，禁止被 TASO1000 同步覆盖/删除
 *
 * 网关配置上传/下载时排除电表 meter 目录，并在升级/重启前通知电表侧落盘。
 */
#ifndef MB_PRODUCT_CFG_LOCAL_H
#define MB_PRODUCT_CFG_LOCAL_H

#include "db_msgevent_api.h"

/* 上传应用配置：清空 config 时保留 /userdata/config/meter */
#define MB_CONFIG_APPLY_CMD \
    "mkdir /tmp/config;tar zxvf /tmp/config.tar.gz -C /tmp/config/;" \
    "rm -r /tmp/config/ech /tmp/config/userdata;rm -r /tmp/config/system;" \
    "find /userdata/config/* | grep -v /userdata/config/meter | xargs rm; " \
    "cp -r /tmp/config/* /userdata/config/ > /dev/null"

/* 下载打包配置：排除 meter */
#define MB_CONFIG_TAR_PACK_CMD \
    "cd /userdata/config;tar zcvf /tmp/config.tar.gz * " \
    "/ech/system/version /userdata/system/* --exclude=meter >/dev/null 2>&1"

#define MB_CONFIG_BEFORE_REBOOT() \
    db_msg_event_cmd(DB_EVENT_TYPE_REBOOT)

#define MB_UPDATE_BEFORE_UPGRADE() \
    db_msg_event_cmd(DB_EVENT_TYPE_SAVE_ALL)

#endif /* MB_PRODUCT_CFG_LOCAL_H */
