#ifndef __DB_CFG_JSON_H__
#define __DB_CFG_JSON_H__

char *db_cfg_jsonfile_alloc(char *file, int *size);
void db_cfg_jsonfile_delete(char *p);
char *db_cfg_jsonfile_read(char *file, char *p, int size);
int db_cfg_jsonfile_write(char *file, char *jsonstring);

#endif /* __DB_CFG_JSON_H__ */
