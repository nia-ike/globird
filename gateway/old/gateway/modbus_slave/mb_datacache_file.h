#ifndef __MB_DATACACHE_FILE_H__
#define __MB_DATACACHE_FILE_H__

#include "mb_configfile.h"

#define MB_DATACACHE_SRC            "/var/run/cache/data.db"
#define MB_DATACACHE_DL             "/tmp/data_cache.db"
/* Pack size must fit MB_COM_TXBUFF_SIZE (header + payload). */
#define MB_DATACACHE_PACKSIZE_MAX   4096
#define MB_DATACACHE_PACKSIZE_DEF   4096

struct mb_config_ctx_t *mb_datacache_file_getctx(void);
void mb_datacache_file_config(struct mb_config_ctx_t *pNewCtx);
int mb_datacache_file_upload(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len);
uint32_t create_down_datacache_file(struct mb_config_ctx_t *pNewCtx, int totalsize);
void mb_file46_set_devtype(uint8_t devtype);
uint8_t mb_file46_get_devtype(void);

#endif
