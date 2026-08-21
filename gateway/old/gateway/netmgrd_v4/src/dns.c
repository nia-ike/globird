/*************************************************************************/
/** dns.c — DNS 保底管理：确保 resolv.conf 前面有公共 DNS 服务器                 **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "netmgr_v4_dns.h"

/* 默认保底 DNS 服务器 */
static const char *g_fallback_dns[NM4_DNS_FALLBACK_COUNT] = {
    "8.8.8.8",    /* Google Public DNS */
    "1.1.1.1"     /* Cloudflare DNS */
};

/*********************************************************************************************************
** Function name:       is_fallback_server
** Descriptions:        Check if the given IP is one of our fallback servers.
** input parameters:    ip      - IP address string.
** output parameters:   NONE
** Returned value:      true if it matches a fallback server, false otherwise.
*********************************************************************************************************/
static bool is_fallback_server(const char *ip)
{
    for (size_t i = 0; i < NM4_DNS_FALLBACK_COUNT; i++) {
        if (strcmp(ip, g_fallback_dns[i]) == 0) {
            return true;
        }
    }
    return false;
}

/*********************************************************************************************************
** Function name:       strip_newline
** Descriptions:        Remove trailing newline from string (if any).
** input parameters:    s       - null-terminated string (modified in place).
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void strip_newline(char *s)
{
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

/*********************************************************************************************************
** Function name:       nm4_dns_ensure_fallback
** Descriptions:        Ensure fallback DNS servers are at the top of resolv.conf.
**                      This function reads the current resolv.conf, removes any existing
**                      fallback server entries, prepends the fallback servers, and writes
**                      back atomically (using rename).
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_ensure_fallback(void)
{
    FILE *fp_in = NULL;
    FILE *fp_out = NULL;
    char line[512];
    char *other_lines[64];  /* Store non-fallback nameserver lines */
    int other_count = 0;
    int ret = -1;
    size_t i;

    /* Read existing resolv.conf and collect non-fallback nameserver lines */
    fp_in = fopen(NM4_RESOLV_CONF, "r");
    if (fp_in) {
        while (fgets(line, sizeof(line), fp_in) && other_count < 64) {
            strip_newline(line);

            /* Check if this is a nameserver line with a fallback IP */
            if (strncmp(line, "nameserver ", 11) == 0) {
                const char *ip = line + 11;
                /* Skip whitespace after "nameserver " */
                while (*ip == ' ' || *ip == '\t') ip++;
                if (is_fallback_server(ip)) {
                    continue;  /* Skip fallback server entries */
                }
            }

            /* Keep this line */
            other_lines[other_count] = strdup(line);
            if (other_lines[other_count]) {
                other_count++;
            }
        }
        fclose(fp_in);
    }

    /* Write new resolv.conf: fallback servers first, then others */
    fp_out = fopen(NM4_RESOLV_CONF_TMP, "w");
    if (!fp_out) {
        goto cleanup;
    }

    /* First, write fallback DNS servers */
    for (i = 0; i < NM4_DNS_FALLBACK_COUNT; i++) {
        fprintf(fp_out, "nameserver %s\n", g_fallback_dns[i]);
    }

    /* Then write other lines (including other nameservers) */
    for (i = 0; i < (size_t)other_count; i++) {
        fprintf(fp_out, "%s\n", other_lines[i]);
    }

    if (fclose(fp_out) != 0) {
        fp_out = NULL;
        goto cleanup;
    }
    fp_out = NULL;

    /* Atomically replace resolv.conf */
    if (rename(NM4_RESOLV_CONF_TMP, NM4_RESOLV_CONF) != 0) {
        goto cleanup;
    }

    ret = 0;

cleanup:
    /* Free stored lines */
    for (i = 0; i < (size_t)other_count; i++) {
        free(other_lines[i]);
    }
    if (fp_out) {
        fclose(fp_out);
        unlink(NM4_RESOLV_CONF_TMP);
    }
    return ret;
}

/*********************************************************************************************************
** Function name:       nm4_dns_set_fallback_servers
** Descriptions:        Set custom fallback DNS servers (runtime configuration).
** input parameters:    servers - array of DNS server IP strings.
**                      count   - number of servers (max NM4_DNS_FALLBACK_COUNT).
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_set_fallback_servers(const char *servers[], size_t count)
{
    if (!servers || count == 0 || count > NM4_DNS_FALLBACK_COUNT) {
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        if (!servers[i] || !servers[i][0]) {
            return -1;
        }
        g_fallback_dns[i] = servers[i];
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_dns_get_fallback_servers
** Descriptions:        Get current fallback DNS server list.
** input parameters:    NONE
** output parameters:   servers - array to receive server strings.
**                      count   - number of servers returned.
** Returned value:      0 on success.
*********************************************************************************************************/
int nm4_dns_get_fallback_servers(const char *servers[], size_t *count)
{
    if (!servers || !count) {
        return -1;
    }

    *count = NM4_DNS_FALLBACK_COUNT;
    for (size_t i = 0; i < NM4_DNS_FALLBACK_COUNT; i++) {
        servers[i] = g_fallback_dns[i];
    }

    return 0;
}
