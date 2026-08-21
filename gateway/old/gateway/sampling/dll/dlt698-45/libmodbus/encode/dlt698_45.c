#include <string.h>
#include "dlt698_45.h"
#include "dlt698_45_data.h"

static uint16_t dlt698_crc16_x25(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : (crc >> 1);
        }
    }
    return (uint16_t)~crc;
}

static void dlt698_set_addr(uint8_t *pAddr, uint64_t wAddr)
{
    uint8_t i;
    for (i = 0; i < 6; i++) {
        uint64_t t = wAddr % 100;
        pAddr[i] = (uint8_t)(((t / 10) << 4) | (t % 10));
        wAddr /= 100;
    }
}

static int32_t dlt698_packet_11h(uint8_t *pBuff, uint8_t LeadByteNum, uint64_t Address, uint32_t OAD)
{
    uint16_t hcs, fcs;
    uint8_t lead;
    dlt698_45_request_t *p;

    /*
     * 前导码：配置为 0 时仍发 4 字节 0xFE（与协议常用格式一致）；
     * 前导与帧体分离，避免 memset(帧体) 把结构体内的前导清成 00。
     */
    lead = LeadByteNum ? LeadByteNum : 4;
    memset(pBuff, 0xFE, lead);
    p = (dlt698_45_request_t *)(pBuff + lead);

    memset(p, 0, sizeof(*p));
    p->start_flag = 0x68;
    /* 长度域不含起始符和结束符 */
    p->length = (uint16_t)(sizeof(*p) - 2U);
    p->control = 0x43;
    p->sa = 0x45;

    if (Address == DLT698_45_BROADCAST_PHYADDR) {
        memset(p->addr, 0xAA, sizeof(p->addr));
    } else {
        dlt698_set_addr(p->addr, Address);
    }

    /* 客户机地址：与参考工程一致，常用 0xA0 */
    p->client_addr = 0xA0;
    hcs = dlt698_crc16_x25((uint8_t *)&p->length, (uint16_t)((uint8_t *)&p->hcs - (uint8_t *)&p->length));
    p->hcs = hcs;

    /* GET-Request-Normal：05 01 + PIID + OAD(4B)，不携带时间标签 */
    p->apdu[0] = 0x05;
    p->apdu[1] = 0x01;
    p->apdu[2] = 0x02;
    p->apdu[3] = (uint8_t)((OAD >> 24) & 0xFF);
    p->apdu[4] = (uint8_t)((OAD >> 16) & 0xFF);
    p->apdu[5] = (uint8_t)((OAD >> 8) & 0xFF);
    p->apdu[6] = (uint8_t)(OAD & 0xFF);

    fcs = dlt698_crc16_x25((uint8_t *)&p->length, (uint16_t)((uint8_t *)&p->fcs - (uint8_t *)&p->length));
    p->fcs = fcs;
    p->end_flag = 0x16;

    return lead + (int32_t)sizeof(*p);
}

static int32_t dlt698_unpacket_91h(uint8_t *pBuff, uint16_t len, uint32_t *pOad, double *pVal)
{
    uint8_t *p = pBuff;
    uint8_t *apdu;
    uint32_t v;
    int32_t sv;
    const uint16_t head_len = 14; /* 68 + L(2) + C + SA + addr(6) + CA + HCS(2) */

    if (!pBuff || len < 20 || !pOad || !pVal) return -1;
    while (len > 0 && *p == 0xFE) {
        p++;
        len--;
    }
    if (len < 20 || p[0] != 0x68 || p[len - 1] != 0x16) return -1;
    if (len <= (head_len + 2)) return -1; /* 至少要有 APDU 与 FCS/结束符 */

    apdu = p + head_len;
    if (apdu[0] != 0x85 || apdu[1] != 0x01) return -1; /* Get-Response-Normal */

    /* APDU: 85 01 PIID OAD(4) GetResult(1) [data...] */
    *pOad = ((uint32_t)apdu[3] << 24) | ((uint32_t)apdu[4] << 16) | ((uint32_t)apdu[5] << 8) | (uint32_t)apdu[6];

    if (apdu[7] != 0x01) return -1; /* Get-Result: data */
    if (len < (head_len + 13 + 3)) return -1; /* 到 value(4B) + FCS(2) + 16 */

    switch (apdu[8]) {
    case 0x06: /* double-long-unsigned, u32 */
        v = ((uint32_t)apdu[9] << 24) | ((uint32_t)apdu[10] << 16) | ((uint32_t)apdu[11] << 8) | (uint32_t)apdu[12];
        *pVal = (double)v;
        return 0;
    case 0x05: /* double-long, s32 */
        sv = (int32_t)(((uint32_t)apdu[9] << 24) | ((uint32_t)apdu[10] << 16) | ((uint32_t)apdu[11] << 8) | (uint32_t)apdu[12]);
        *pVal = (double)sv;
        return 0;
    default:
        return -1;
    }
}

const struct dlt698_45_encode_decode dlt698_45_codec = {
    .Packet11H = dlt698_packet_11h,
    .UnPacket91H = dlt698_unpacket_91h,
};
