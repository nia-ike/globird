/*
 * mb_product_cfg.h
 *
 * 产品差异配置（默认 = TASO1000 行为）。
 * TASO1302 通过同目录 mb_product_cfg_local.h 覆盖宏；
 * 从 TASO1000 整目录同步时勿删除/覆盖 *_local.* 与 Makefile.local。
 */
#ifndef MB_PRODUCT_CFG_H
#define MB_PRODUCT_CFG_H

/*
 * 产品本地覆盖头：仅 TASO1302 存在，在此先 include，再填默认值。
 * 无 __has_include 时，由 Makefile.local 定义 MB_HAS_PRODUCT_CFG_LOCAL。
 */
#if defined(__has_include)
#  if __has_include("mb_product_cfg_local.h")
#    include "mb_product_cfg_local.h"
#  endif
#elif defined(MB_HAS_PRODUCT_CFG_LOCAL)
#  include "mb_product_cfg_local.h"
#endif

/* 配置包上传应用：解压并替换 /userdata/config（默认清空全部） */
#ifndef MB_CONFIG_APPLY_CMD
#define MB_CONFIG_APPLY_CMD \
    "mkdir /tmp/config;tar zxvf /tmp/config.tar.gz -C /tmp/config/;" \
    "rm -r /tmp/config/ech /tmp/config/userdata;rm -r /tmp/config/system;" \
    "rm -rf /userdata/config/*; cp -r /tmp/config/* /userdata/config/ > /dev/null"
#endif

/* 配置包下载打包（默认包含全部目录） */
#ifndef MB_CONFIG_TAR_PACK_CMD
#define MB_CONFIG_TAR_PACK_CMD \
    "cd /userdata/config;tar zcvf /tmp/config.tar.gz * " \
    "/ech/system/version /userdata/system/* >/dev/null 2>&1"
#endif

/* 配置更新即将 reboot 前的产品钩子（默认空） */
#ifndef MB_CONFIG_BEFORE_REBOOT
#define MB_CONFIG_BEFORE_REBOOT()  ((void)0)
#endif

/* ipk 升级安装前的产品钩子（默认空） */
#ifndef MB_UPDATE_BEFORE_UPGRADE
#define MB_UPDATE_BEFORE_UPGRADE()  ((void)0)
#endif

#endif /* MB_PRODUCT_CFG_H */
