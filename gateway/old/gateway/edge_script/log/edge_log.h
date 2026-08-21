#ifndef EDGE_LOG_H
#define EDGE_LOG_H

/*
 * 边缘脚本日志：写入 dev_log，供 modbus_slave 49H + 上位机 EdgeScriptLogClient 拉取。
 * app_name 须与 SmartGate EdgeScriptLogClient.AppName 一致。
 */

#define EDGE_LOG_APP_NAME           "edge_script_service"
#define EDGE_LOG_CHANNEL            0
#define EDGE_LOG_DEV                0

int edge_log_init(int debug);
void edge_log_shutdown(void);

void edge_log_service_info(const char *fmt, ...);
void edge_log_service_warn(const char *fmt, ...);
void edge_log_service_error(const char *fmt, ...);

void edge_log_script_info(const char *script_id, const char *fmt, ...);
void edge_log_script_warn(const char *script_id, const char *fmt, ...);
void edge_log_script_error(const char *script_id, const char *fmt, ...);

#endif /* EDGE_LOG_H */
