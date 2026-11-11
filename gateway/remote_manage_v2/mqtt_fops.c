/*
* Copyright (c) 2013,珠海一构物联有限公司
* All rights reserved.
*
* 文件名称：db_cfg.h
* 文件标识：
* 描    述：
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录
*  作者       时间         版本      修改描述
*  chenzy   2013.11.08   V1.00      创建
*/
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "mqtt_fops.h"

#include "if_data_json.h"
#include "if_process.h"

#include "com_baseclass.h"

/* 客户约定固定文件名 */
#define MQTT_CA_BUNDLE_NAME   "azure-ca-bundle.crt"
#define MQTT_CA_FALLBACK_NAME "ca.crt"
#define MQTT_CERT_PEM_NAME    "certificate.pem"
#define MQTT_KEY_PEM_NAME     "private-key.pem"
#define MQTT_CURRENT_NAME     "current"

/*************************************************************************/
/** Purpose: root + name，避免 root 已带 '/' 时出现双斜杠               **/
/*************************************************************************/
static void mqtt_tls_path_join(char *out, size_t out_sz, const char *root, const char *name)
{
    size_t n;

    if (!out || out_sz == 0)
        return;
    out[0] = '\0';
    if (!root || !name)
        return;

    n = strlen(root);
    while (n > 1 && root[n - 1] == '/')
        n--;
    snprintf(out, out_sz, "%.*s/%s", (int)n, root, name);
}

/*************************************************************************/
/** Purpose: 确认文件可打开；ENOENT 表示可能撞上证书轮换               **/
/*************************************************************************/
static int mqtt_tls_try_open(const char *path)
{
    int fd;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    close(fd);
    return 0;
}

/*************************************************************************/
/** Purpose: 解析 CA 路径：优先 azure-ca-bundle.crt，其次 ca.crt        **/
/*************************************************************************/
static int mqtt_tls_resolve_ca(const char *root, char *cafile, size_t cafile_sz)
{
    if (!root || !cafile || cafile_sz == 0)
        return -1;

    mqtt_tls_path_join(cafile, cafile_sz, root, MQTT_CA_BUNDLE_NAME);
    if (mqtt_tls_try_open(cafile) == 0)
        return 0;

    mqtt_tls_path_join(cafile, cafile_sz, root, MQTT_CA_FALLBACK_NAME);
    if (mqtt_tls_try_open(cafile) == 0)
        return 0;

    cafile[0] = '\0';
    return -1;
}

/*************************************************************************/
/** Purpose: readlink <root>/current，拼出本代绝对目录                   **/
/** Notice:  不缓存 generations/<hash>；每次连接重新调用                **/
/*************************************************************************/
static int mqtt_tls_resolve_generation(const char *root, char *gen_dir, size_t gen_dir_sz)
{
    char current_link[256];
    char linkbuf[256];
    ssize_t n;

    if (!root || root[0] == '\0' || !gen_dir || gen_dir_sz == 0)
        return -1;

    mqtt_tls_path_join(current_link, sizeof(current_link), root, MQTT_CURRENT_NAME);
    n = readlink(current_link, linkbuf, sizeof(linkbuf) - 1);
    if (n < 0)
        return -1;
    linkbuf[n] = '\0';

    if (linkbuf[0] == '/')
        snprintf(gen_dir, gen_dir_sz, "%s", linkbuf);
    else
        snprintf(gen_dir, gen_dir_sz, "%s/%s", root, linkbuf);

    return 0;
}

/*************************************************************************/
/** Purpose: 一次解析证书路径                                           **/
/** Notice:  优先 Azure 代际(current)；无 current 时回退扁平目录         **/
/*************************************************************************/
static int mqtt_tls_resolve_once(const char *root,
                                 char *cafile, size_t cafile_sz,
                                 char *certfile, size_t certfile_sz,
                                 char *keyfile, size_t keyfile_sz)
{
    char gen_dir[256];

    cafile[0] = certfile[0] = keyfile[0] = '\0';

    if (!root || root[0] == '\0')
        return -1;

    if (mqtt_tls_resolve_ca(root, cafile, cafile_sz) < 0)
        return -1;

    if (mqtt_tls_resolve_generation(root, gen_dir, sizeof(gen_dir)) == 0) {
        /* Azure: leaf/key 在 current 指向的代际目录 */
        mqtt_tls_path_join(certfile, certfile_sz, gen_dir, MQTT_CERT_PEM_NAME);
        mqtt_tls_path_join(keyfile, keyfile_sz, gen_dir, MQTT_KEY_PEM_NAME);
    } else {
        /* 扁平: cert/key 与 CA 同目录 */
        mqtt_tls_path_join(certfile, certfile_sz, root, MQTT_CERT_PEM_NAME);
        mqtt_tls_path_join(keyfile, keyfile_sz, root, MQTT_KEY_PEM_NAME);
    }

    /* cert/key 必须能打开；任一 ENOENT 视为轮换冲突，由上层重试 */
    if (mqtt_tls_try_open(certfile) < 0)
        return -1;
    if (mqtt_tls_try_open(keyfile) < 0)
        return -1;

    return 0;
}

/*************************************************************************/
/** Purpose: 解析证书布局；ENOENT 时重新 readlink 再读（撞上轮换）       **/
/*************************************************************************/
static int mqtt_tls_resolve(const char *root,
                            char *cafile, size_t cafile_sz,
                            char *certfile, size_t certfile_sz,
                            char *keyfile, size_t keyfile_sz)
{
    int attempt;
    int saved_errno = 0;

    for (attempt = 0; attempt < 3; attempt++) {
        if (mqtt_tls_resolve_once(root, cafile, cafile_sz,
                                  certfile, certfile_sz,
                                  keyfile, keyfile_sz) == 0)
            return 0;
        saved_errno = errno;
        if (saved_errno != ENOENT && saved_errno != 0)
            break;
        /* ENOENT：current 可能刚切代，短暂重试 */
        usleep(50 * 1000);
    }

    if (saved_errno)
        errno = saved_errno;
    return -1;
}

/*************************************************************************/
/** Purpose: 每条连接开始：刷新注入根路径 + 解析 current + tls_set       **/
/** Notice:  不在启动时读一次常驻；断线重连前再次调用                    **/
/*************************************************************************/
static int mqtt_tls_apply(struct remote_m_ctx_t *pctx)
{
    char cafile[256];
    char certfile[256];
    char keyfile[256];
    int ret;

    if (!pctx || !pctx->mosq)
        return -1;

    /* 未锁定时，从 remote_m.json 的 cert_path 刷新 */
    if (if_process_cert_root_refresh(pctx) < 0) {
        if (pctx->debug)
            printf("MQTT TLS: cert root not injected\n");
        return -1;
    }

    ret = mqtt_tls_resolve(pctx->attr.cert_path,
                           cafile, sizeof(cafile),
                           certfile, sizeof(certfile),
                           keyfile, sizeof(keyfile));
    if (ret < 0) {
        if (pctx->debug) {
            printf("MQTT TLS: resolve failed under root '%s' errno=%d (%s)\n",
                   pctx->attr.cert_path, errno, strerror(errno));
            printf("  expect: azure-ca-bundle.crt|ca.crt + certificate.pem + private-key.pem\n");
            printf("  (flat dir OK; or current -> generations/<hash>/)\n");
        }
        return -1;
    }

    ret = mosquitto_tls_set(pctx->mosq, cafile, NULL, certfile, keyfile, NULL);
    if (ret != MOSQ_ERR_SUCCESS) {
        if (pctx->debug)
            printf("MQTT TLS: mosquitto_tls_set failed %d\n", ret);
        return -1;
    }

    if (pctx->debug) {
        printf("MQTT TLS apply: ca=%s cert=%s key=%s\n",
               cafile, certfile, keyfile);
    }

    /* 连 IP 时证书 CN/SAN 常为域名，关闭 hostname 校验（仍校验证书链） */
    if (inet_addr(pctx->attr.ip) != (in_addr_t)-1) {
        mosquitto_tls_insecure_set(pctx->mosq, true);
        if (pctx->debug)
            printf("MQTT TLS: host is IP (%s), tls_insecure=1\n", pctx->attr.ip);
    }

    return 0;
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static void mqtt_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    struct remote_m_ctx_t *pctx = (struct remote_m_ctx_t *)userdata;

    (void)mosq;
    if (!pctx || !pctx->debug || !str)
        return;
    /* 只打警告/错误，避免刷屏 */
    if (level == MOSQ_LOG_ERR || level == MOSQ_LOG_WARNING || level == MOSQ_LOG_NOTICE)
        printf("MQTT log[%d]: %s\n", level, str);
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static void mqtt_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
    struct remote_m_ctx_t *pctx = (struct remote_m_ctx_t *)userdata;

    (void)mosq;
    (void)qos_count;
    (void)granted_qos;
    if (pctx && pctx->debug)
        printf("MQTT subscribe ok mid=%d\n", mid);
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static void mqtt_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
    int  ret;
    char buffer[48];
    struct mqtt_msg_t mqtt_send_msg;
    struct remote_m_ctx_t *pctx = (struct remote_m_ctx_t *)userdata;

    if (result) {
        if (pctx && pctx->debug)
            printf("MQTT CONNACK refused: %d (%s)\n",
                   result, mosquitto_connack_string(result));
        return;
    }

    if (pctx->debug)
        printf("MQTT CONNACK ok, sn=%s\n", pctx->sn);

    /* Subscribe to broker information topics on successful connect. */
    snprintf(buffer, sizeof(buffer), "%s%s", MQTT_TOPIC_PREFIX, pctx->sn);
    ret = mosquitto_subscribe(mosq, NULL, buffer, MQTT_QOS);
    if (pctx->debug)
        printf("MQTT subscribed topic: %s (ret=%d)\n", buffer, ret);

    /* 生成注册帧 */
    ret = if_msg_pack_register(pctx, &mqtt_send_msg);
    if (ret < 0)
        return;

    ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC_WILL,
                            mqtt_send_msg.len + sizeof(mqtt_send_msg.len),
                            (uint8_t *)&mqtt_send_msg, MQTT_QOS, false);
    if (pctx->debug) {
        printf("MQTT register: publish ret=%d length=%d\n", ret, mqtt_send_msg.len);
        printf("Context:\n%s\n", mqtt_send_msg.data);
    }
}

/*************************************************************************/
/** Purpose: 断线日志；证书在重连前由 loop 外层重新 apply               **/
/*************************************************************************/
static void mqtt_disconnect_callback(struct mosquitto *mosq, void *userdata, int rc)
{
    struct remote_m_ctx_t *pctx = (struct remote_m_ctx_t *)userdata;

    (void)mosq;
    if (pctx && pctx->debug)
        printf("MQTT disconnect: rc=%d (%s)\n", rc, mosquitto_strerror(rc));

    /*
     * 注意：此处不要 mosquitto_tls_set。
     * 在 disconnect 回调里改 TLS 易导致 loop_forever 直接退出；
     * 证书轮换放到 mqtt_process 重连前处理。
     */
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
static void mqtt_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    struct mqtt_msg_t *pmsg;
    struct remote_m_ctx_t *pctx = (struct remote_m_ctx_t *)userdata;

    (void)mosq;
    if (message->payloadlen <= 0)
        return;

    pmsg = (struct mqtt_msg_t *)(message->payload);
    if (pctx->debug)
        printf("MQTT RX topic=%s func=%d len=%u payloadlen=%d\n",
               message->topic ? message->topic : "?",
               pmsg->func, (unsigned)pmsg->len, message->payloadlen);

    switch (pmsg->func)
    {
        case FUNC_WILL:
            break;
        case FUNC_STATUS:
            break;
        case FUNC_VPN_CON:
            if_process_vpn_conn(pctx, pmsg);
            break;
        case FUNC_VPN_DISC:
            if_process_vpn_disconn(pctx, pmsg);
            break;
        case FUNC_TRANS:
            if_process_trans_request(pctx, pmsg);
            break;
        case FUNC_REBOOT:
            system("sync; reboot;");
            break;
        default:
            break;
    }
}

/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
int mqtt_init(struct remote_m_ctx_t *pctx)
{
    int  ret;
    bool session = true;
    struct mqtt_msg_t msg;
    int  cret;
    char client_id[48];
    static uint8_t conntime_index = 0;
    const uint8_t conntimeout[] = {30, 60, 120, 240};

    mosquitto_lib_init();

    /*
     * Client ID 与 Topic 分离：
     *   Topic 仍用 SN（调试工具发 GBE... / 订 GBE...-TRANS）
     *   Client ID 用 SN-rm，避免与 MeterDebugTool 同 ID 被 Broker 互踢（CONNACK 后马上 rc=7）
     */
    snprintf(client_id, sizeof(client_id), "%s-rm", pctx->sn);
    pctx->mosq = mosquitto_new(client_id, session, pctx);
    if (!pctx->mosq) {
        mosquitto_lib_cleanup();
        return -1;
    }
    if (pctx->debug)
        printf("MQTT client_id=%s (topic sn=%s)\n", client_id, pctx->sn);

    mosquitto_username_pw_set(pctx->mosq, pctx->attr.user, pctx->attr.passwd);
    mosquitto_log_callback_set(pctx->mosq, mqtt_log_callback);
    mosquitto_connect_callback_set(pctx->mosq, mqtt_connect_callback);
    mosquitto_disconnect_callback_set(pctx->mosq, mqtt_disconnect_callback);
    mosquitto_message_callback_set(pctx->mosq, mqtt_message_callback);
    mosquitto_subscribe_callback_set(pctx->mosq, mqtt_subscribe_callback);
    mosquitto_reconnect_delay_set(pctx->mosq, 30, 480, false);

    /* 遗愿设定 */
    ret = if_msg_pack_will(pctx, &msg);
    if (ret < 0)
        return -1;

    mosquitto_will_set(pctx->mosq, MQTT_TOPIC_WILL,
                       msg.len + sizeof(msg.len), (uint8_t *)&msg, 1, false);

    /* 解析证书并 connect（CONNACK 仍需 loop 才能确认） */
    while (1) {
        if (pctx->attr.cert_checked) {
            if (mqtt_tls_apply(pctx) < 0) {
                if (conntime_index >= sizeof(conntimeout) / sizeof(conntimeout[0]))
                    conntime_index = 0;
                if (pctx->debug)
                    printf("MQTT TLS apply failed, retry in %us\n",
                           conntimeout[conntime_index]);
                sleep(conntimeout[conntime_index]);
                conntime_index++;
                continue;
            }
        }

        cret = mosquitto_connect(pctx->mosq, pctx->attr.ip, pctx->attr.port, 60);
        if (cret == MOSQ_ERR_SUCCESS) {
            if (pctx->debug)
                printf("MQTT connect() issued to %s:%d (wait CONNACK in loop)\n",
                       pctx->attr.ip, pctx->attr.port);
            break;
        }

        if (pctx->debug)
            printf("MQTT connect() failed: %d (%s)\n", cret, mosquitto_strerror(cret));

        if (conntime_index >= sizeof(conntimeout) / sizeof(conntimeout[0]))
            conntime_index = 0;
        sleep(conntimeout[conntime_index]);
        conntime_index++;
    }

    /* 心跳线程放到 loop 里能真正收发之后；此处仅启动线程（首次发包可能仍排队） */
    if_process_heartbeat_start(pctx);

    return 0;
}



/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void mqtt_process(struct remote_m_ctx_t *pctx)
{
    int rc;

    while (1) {
        rc = mosquitto_loop_forever(pctx->mosq, 1000, 1);
        if (pctx->debug)
            printf("MQTT loop_forever returned %d (%s), will re-apply TLS & reconnect\n",
                   rc, mosquitto_strerror(rc));

        /* loop 异常退出：重新装证书再连，避免进程直接结束回到 shell */
        if (pctx->attr.cert_checked)
            mqtt_tls_apply(pctx);

        rc = mosquitto_reconnect(pctx->mosq);
        if (pctx->debug)
            printf("MQTT reconnect: %d (%s)\n", rc, mosquitto_strerror(rc));
        sleep(3);
    }
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void mqtt_destroy(struct remote_m_ctx_t *pctx)
{
    mosquitto_destroy(pctx->mosq);
    mosquitto_lib_cleanup();
}
