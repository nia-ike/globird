#ifndef __MB_DATACACHE_FILE_H__
#define __MB_DATACACHE_FILE_H__

#include "mb_configfile.h"

#define MB_DATACACHE_SRC            "/var/run/cache/data.db"
#define MB_DATACACHE_DL             "/tmp/data_cache.db.gz"
#define MB_DATACACHE_DL_PLAIN       "/tmp/data_cache.db"
/* Pack size must fit MB_COM_TXBUFF_SIZE (header + payload). */
#define MB_DATACACHE_PACKSIZE_MAX   4096
#define MB_DATACACHE_PACKSIZE_DEF   4096

/* destinype=3：按日历史；目录写死，不读配置；日库为 YYYYMMDD.db.gz */
#define MB_DATACACHE_HISTORY_DIR    "/userdata/history"
#define MB_DATACACHE_HISTORY_LIST_DL "/tmp/dc_history.list"
#define MB_DATACACHE_PATH_MAX       160

struct mb_config_ctx_t *mb_datacache_file_getctx(void);
void mb_datacache_file_config(struct mb_config_ctx_t *pNewCtx);
int mb_datacache_file_upload(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len);
uint32_t create_down_datacache_file(struct mb_config_ctx_t *pNewCtx, int totalsize);
void mb_datacache_file_finish(void);

/* destinype=3 历史：列表或按日 gzip 库 */
void mb_datacache_history_config(struct mb_config_ctx_t *pNewCtx);
int mb_datacache_history_upload(struct mb_config_ctx_t *pNewCtx, uint8_t *pbuf, uint16_t len);
uint32_t create_down_datacache_history(struct mb_config_ctx_t *pNewCtx, int totalsize);
void mb_datacache_history_finish(void);

/* 设置/清除本会话远程相对名：".list" 或 "YYYYMMDD.db.gz"（由 46H 可选 path 尾巴解析） */
int mb_datacache_history_set_name(const char *name, uint8_t name_len);
void mb_datacache_history_clear_name(void);

void mb_file46_set_devtype(uint8_t destinype);
uint8_t mb_file46_get_devtype(void);

#endif
