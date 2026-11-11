/*
 * FCAS 半波注入：选择性发布 + 工程值/内部值换算
 * 分辨率见 application/docs/电参量系数梳理.md（halfwave.primary）
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "db_fcas_inject.h"

static int fcas_mask_on(const struct db_fcas_inject_type *inj, int col)
{
    if (inj == NULL)
        return 0;
    if (inj->magic != DB_FCAS_INJECT_MAGIC)
        return 0;
    if (inj->state == DB_FCAS_INJECT_STATE_IDLE)
        return 0;
    if (col < 0 || col >= DB_FCAS_INJECT_MAX_COLS)
        return 0;
    return inj->col_mask[col] != 0;
}

/* 把 keep 中某一注入列的原始字段拷到 out（用于 publish 时跳过覆盖） */
static void fcas_preserve_col(struct db_basic_realtime_type *out,
                              const struct db_basic_realtime_type *keep,
                              int col)
{
    if (out == NULL || keep == NULL)
        return;

    switch (col) {
    case DB_FCAS_COL_FREQ_HZ:
        out->Hz_hp = keep->Hz_hp;
        out->Hz    = keep->Hz;
        break;
    case DB_FCAS_COL_VA:
        out->Va = keep->Va;
        break;
    case DB_FCAS_COL_VB:
        out->Vb = keep->Vb;
        break;
    case DB_FCAS_COL_VC:
        out->Vc = keep->Vc;
        break;
    case DB_FCAS_COL_IA:
        out->Ia = keep->Ia;
        break;
    case DB_FCAS_COL_IB:
        out->Ib = keep->Ib;
        break;
    case DB_FCAS_COL_IC:
        out->Ic = keep->Ic;
        break;
    case DB_FCAS_COL_PA:
        out->Pa = keep->Pa;
        break;
    case DB_FCAS_COL_PB:
        out->Pb = keep->Pb;
        break;
    case DB_FCAS_COL_PC:
        out->Pc = keep->Pc;
        break;
    case DB_FCAS_COL_PTOT:
        out->Ptot = keep->Ptot;
        break;
    case DB_FCAS_COL_QA:
        out->Qa = keep->Qa;
        break;
    case DB_FCAS_COL_QB:
        out->Qb = keep->Qb;
        break;
    case DB_FCAS_COL_QC:
        out->Qc = keep->Qc;
        break;
    case DB_FCAS_COL_QTOT:
        out->Qtot = keep->Qtot;
        break;
    case DB_FCAS_COL_PFA:
        out->PFa = keep->PFa;
        break;
    case DB_FCAS_COL_PFB:
        out->PFb = keep->PFb;
        break;
    case DB_FCAS_COL_PFC:
        out->PFc = keep->PFc;
        break;
    case DB_FCAS_COL_PFTOT:
        out->PFtot = keep->PFtot;
        break;
    default:
        break;
    }
}

void db_fcas_halfwave_publish(struct db_basic_realtime_type *dst,
                              const struct db_basic_realtime_type *src,
                              const struct db_fcas_inject_type *inj)
{
    struct db_basic_realtime_type tmp;
    int col;

    if (dst == NULL || src == NULL)
        return;

    tmp = *src;

    /*
     * mask=1 列由 fcas_inject 写入半波 SHM；此处从 dst 保留这些字段，
     * 避免计算结果覆盖注入值。整表不在 vmem，msr 只认 col_mask。
     */
    if (inj != NULL &&
        inj->magic == DB_FCAS_INJECT_MAGIC &&
        inj->state != DB_FCAS_INJECT_STATE_IDLE) {
        for (col = 0; col < DB_FCAS_INJECT_MAX_COLS; col++) {
            if (fcas_mask_on(inj, col))
                fcas_preserve_col(&tmp, dst, col);
        }
    }

    *dst = tmp;
}

void db_fcas_write_col(struct db_basic_realtime_type *dst, int col, float eng)
{
    if (dst == NULL)
        return;

    switch (col) {
    case DB_FCAS_COL_FREQ_HZ:
        dst->Hz_hp = (uint32_t)lroundf(eng * 10000.0f);
        dst->Hz    = (uint16_t)lroundf(eng * 100.0f);
        break;
    case DB_FCAS_COL_VA:
        dst->Va = (uint32_t)lroundf(eng * 100.0f);
        break;
    case DB_FCAS_COL_VB:
        dst->Vb = (uint32_t)lroundf(eng * 100.0f);
        break;
    case DB_FCAS_COL_VC:
        dst->Vc = (uint32_t)lroundf(eng * 100.0f);
        break;
    case DB_FCAS_COL_IA:
        dst->Ia = (uint32_t)lroundf(eng * 1000.0f);
        break;
    case DB_FCAS_COL_IB:
        dst->Ib = (uint32_t)lroundf(eng * 1000.0f);
        break;
    case DB_FCAS_COL_IC:
        dst->Ic = (uint32_t)lroundf(eng * 1000.0f);
        break;
    case DB_FCAS_COL_PA:
        dst->Pa = (int32_t)lroundf(eng * 10.0f);
        break;
    case DB_FCAS_COL_PB:
        dst->Pb = (int32_t)lroundf(eng * 10.0f);
        break;
    case DB_FCAS_COL_PC:
        dst->Pc = (int32_t)lroundf(eng * 10.0f);
        break;
    case DB_FCAS_COL_PTOT:
        /* CSV: Ptot_kW → W 内部 0.1W */
        dst->Ptot = (int32_t)lroundf(eng * 1000.0f * 10.0f);
        break;
    case DB_FCAS_COL_QA:
        dst->Qa = (int32_t)lroundf(eng * 10.0f);
        break;
    case DB_FCAS_COL_QB:
        dst->Qb = (int32_t)lroundf(eng * 10.0f);
        break;
    case DB_FCAS_COL_QC:
        dst->Qc = (int32_t)lroundf(eng * 10.0f);
        break;
    case DB_FCAS_COL_QTOT:
        dst->Qtot = (int32_t)lroundf(eng * 1000.0f * 10.0f);
        break;
    case DB_FCAS_COL_PFA:
        dst->PFa = (int16_t)lroundf(eng * 1000.0f);
        break;
    case DB_FCAS_COL_PFB:
        dst->PFb = (int16_t)lroundf(eng * 1000.0f);
        break;
    case DB_FCAS_COL_PFC:
        dst->PFc = (int16_t)lroundf(eng * 1000.0f);
        break;
    case DB_FCAS_COL_PFTOT:
        dst->PFtot = (int16_t)lroundf(eng * 1000.0f);
        break;
    default:
        break;
    }
}

float db_fcas_read_col(const struct db_basic_realtime_type *src, int col)
{
    if (src == NULL)
        return 0.0f;

    switch (col) {
    case DB_FCAS_COL_FREQ_HZ:
        return (float)src->Hz_hp / 10000.0f;
    case DB_FCAS_COL_VA:
        return (float)src->Va / 100.0f;
    case DB_FCAS_COL_VB:
        return (float)src->Vb / 100.0f;
    case DB_FCAS_COL_VC:
        return (float)src->Vc / 100.0f;
    case DB_FCAS_COL_IA:
        return (float)src->Ia / 1000.0f;
    case DB_FCAS_COL_IB:
        return (float)src->Ib / 1000.0f;
    case DB_FCAS_COL_IC:
        return (float)src->Ic / 1000.0f;
    case DB_FCAS_COL_PA:
        return (float)src->Pa / 10.0f;
    case DB_FCAS_COL_PB:
        return (float)src->Pb / 10.0f;
    case DB_FCAS_COL_PC:
        return (float)src->Pc / 10.0f;
    case DB_FCAS_COL_PTOT:
        return (float)src->Ptot / 10.0f / 1000.0f;
    case DB_FCAS_COL_QA:
        return (float)src->Qa / 10.0f;
    case DB_FCAS_COL_QB:
        return (float)src->Qb / 10.0f;
    case DB_FCAS_COL_QC:
        return (float)src->Qc / 10.0f;
    case DB_FCAS_COL_QTOT:
        return (float)src->Qtot / 10.0f / 1000.0f;
    case DB_FCAS_COL_PFA:
        return (float)src->PFa / 1000.0f;
    case DB_FCAS_COL_PFB:
        return (float)src->PFb / 1000.0f;
    case DB_FCAS_COL_PFC:
        return (float)src->PFc / 1000.0f;
    case DB_FCAS_COL_PFTOT:
        return (float)src->PFtot / 1000.0f;
    default:
        return 0.0f;
    }
}

static const char *s_col_names[DB_FCAS_INJECT_MAX_COLS] = {
    "freq_hz", "Va", "Vb", "Vc", "Ia", "Ib", "Ic",
    "Pa_W", "Pb_W", "Pc_W", "Ptot_kW",
    "Qa_var", "Qb_var", "Qc_var", "Qtot_kvar",
    "PFa", "PFb", "PFc", "PFtot",
};

const char *db_fcas_col_name(int col)
{
    if (col < 0 || col >= DB_FCAS_INJECT_MAX_COLS)
        return "?";
    return s_col_names[col];
}

static int name_eq_ci(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

int db_fcas_parse_header_token(const char *token, int *enable)
{
    char name[64];
    const char *eq;
    int i, n;

    if (token == NULL || enable == NULL)
        return -1;

    while (*token && isspace((unsigned char)*token))
        token++;

    eq = strchr(token, '=');
    if (eq == NULL || strchr(eq + 1, '=') != NULL)
        return -1;

    n = (int)(eq - token);
    if (n <= 0 || n >= (int)sizeof(name))
        return -1;
    memcpy(name, token, (size_t)n);
    name[n] = '\0';
    /* trim trailing space in name */
    while (n > 0 && isspace((unsigned char)name[n - 1])) {
        name[--n] = '\0';
    }

    /* '=' 右边只能是单个 0 或 1，拒绝 Va=1Vb=1、Va=2、Va= */
    {
        const char *v = eq + 1;
        const char *p;

        while (*v && isspace((unsigned char)*v))
            v++;
        if (*v != '0' && *v != '1')
            return -1;
        for (p = v + 1; *p; p++) {
            if (!isspace((unsigned char)*p))
                return -1;
        }
        *enable = *v - '0';
    }

    for (i = 0; i < DB_FCAS_INJECT_MAX_COLS; i++) {
        if (name_eq_ci(name, s_col_names[i]))
            return i;
    }
    return -1;
}

void db_fcas_inject_clear(struct db_fcas_inject_type *inj)
{
    if (inj == NULL)
        return;
    memset(inj, 0, sizeof(*inj));
    inj->magic = DB_FCAS_INJECT_MAGIC;
    inj->state = DB_FCAS_INJECT_STATE_IDLE;
}
