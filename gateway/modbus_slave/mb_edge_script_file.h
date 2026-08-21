#ifndef __MB_EDGE_SCRIPT_FILE_H__
#define __MB_EDGE_SCRIPT_FILE_H__

#include <stdint.h>

#include "mb_configfile.h"

#define MB_EDGE_SCRIPT_FILE             "/tmp/edge_script.tar.gz"
#define MB_EDGE_SCRIPT_LIVE_DIR         "/userdata/config/edge_script"
#define MB_EDGE_SCRIPT_NEW_DIR          "/userdata/config/edge_script.new"
#define MB_EDGE_SCRIPT_JSON             "/userdata/config/edge_script/edge_scripts.json"

struct mb_config_ctx_t *mb_edge_script_getctx(void);
void mb_edge_script_file_config(struct mb_config_ctx_t *pNewCtx);
int mb_edge_script_file_upload(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len);
int mb_edge_script_file_download(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len);
int mb_edge_script_file_update(void);
uint32_t mb_edge_script_create_download_file(struct mb_config_ctx_t *pNewCtx);

#endif /* __MB_EDGE_SCRIPT_FILE_H__ */
