#include "edge_point_util.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "db_struct.h"

/*************************************************************************/
/** Function Name:       edge_dblink_parse                                 **/
/** Purpose:             将 8 位十六进制 dblink 字符串解析为 uint32_t ID    **/
/** Params:            hex_str - 8 字符 hex 串；out_id - 输出 ID           **/
/** Return:            0-成功；-1-格式无效                                 **/
/** Notice:            与上位机 Blockly 导出的 DBLINK 格式一致             **/
/*************************************************************************/
int edge_dblink_parse(const char *hex_str, uint32_t *out_id)
{
    char *endptr;
    unsigned long v;
    size_t len;
    size_t i;

    if (!hex_str || !out_id)
        return -1;

    len = strlen(hex_str);
    if (len != 8)
        return -1;

    for (i = 0; i < len; i++)
    {
        if (!isxdigit((unsigned char)hex_str[i]))
            return -1;
    }

    v = strtoul(hex_str, &endptr, 16);
    if (!endptr || *endptr != '\0' || v > 0xFFFFFFFFUL)
        return -1;

    *out_id = (uint32_t)v;
    return 0;
}

/*************************************************************************/
/** Function Name:       edge_dblink_channel                                 **/
/** Purpose:             从 dblink_id 提取通道号                            **/
/** Params:            id - 测点 dblink_id                                 **/
/** Return:            通道号（chnl_id）                                   **/
/** Notice:            封装 DB_CONVENT_ID_2_CHNLNO 宏                      **/
/*************************************************************************/
uint8_t edge_dblink_channel(uint32_t id)
{
    return DB_CONVENT_ID_2_CHNLNO(id);
}
