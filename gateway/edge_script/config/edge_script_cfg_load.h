#ifndef EDGE_SCRIPT_CFG_LOAD_H
#define EDGE_SCRIPT_CFG_LOAD_H

#include "../edge_script_config.h"

int edge_script_config_load(EdgeScriptConfig *cfg, const char *config_base);

/** 0=成功（含配置文件不存在时的空配置）；-1=文件存在但读/解析失败 */

#endif /* EDGE_SCRIPT_CFG_LOAD_H */
