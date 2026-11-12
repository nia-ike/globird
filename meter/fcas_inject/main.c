/*
 * FCAS 半波测试数据注入进程（手动启动，不进 monit）
 * 用法: fcas_inject [-f csv] [-o log.csv] [-c chip] [-T n] [-t ms] [-C]
 *
 * CSV 整表在本进程 malloc；vmem 仅写 magic/state/chip/col_mask。
 * 进程退出后本地表释放；HOLD 时 mask 仍留在 vmem，msr 跳过这些列。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include "db_struct.h"
#include "db_device.h"
#include "db_fcas_inject.h"

#define FCAS_DEFAULT_CSV   "/userdata/testdata/fcas_injection.csv"
#define FCAS_LINE_MAX      2048

struct fcas_local_table {
    uint8_t  col_mask[DB_FCAS_INJECT_MAX_COLS];
    uint16_t row_count;
    float   *data; /* row-major: data[row * COLS + col] */
};

static void usage(const char *name)
{
    fprintf(stderr,
            "Usage: %s [options]\n"
            "  -f <csv>   input CSV (default: %s)\n"
            "  -o <csv>   optional output log CSV (readback from halfwave SHM)\n"
            "  -c <chip>  measure chip index (default: 0)\n"
            "  -T <n>     repeat the CSV n times (n>=1); clear screen and show progress\n"
            "  -t <ms>    row hold time in ms (default: 20)\n"
            "  -C         clear inject control (resume full msr publish) and exit\n"
            "  -h         help\n"
            "\n"
            "Play at 20ms/row (once, or -T times); then HOLD last row (col_mask stays).\n"
            "Not started by monit; run manually after upgrade.\n",
            name, FCAS_DEFAULT_CSV);
}

static char *trim(char *s)
{
    char *e;
    while (*s && isspace((unsigned char)*s))
        s++;
    if (*s == 0)
        return s;
    e = s + strlen(s) - 1;
    while (e > s && isspace((unsigned char)*e))
        *e-- = 0;
    return s;
}

static int s_ncol;
static int s_col_map[DB_FCAS_INJECT_MAX_COLS];
static uint8_t s_mask[DB_FCAS_INJECT_MAX_COLS];

static void print_csv_header(void)
{
    int i;

    for (i = 0; i < s_ncol; i++) {
        int col = s_col_map[i];
        printf("%-6d%-14s%u\n", i + 1, db_fcas_col_name(col),
               (unsigned)s_mask[col]);
    }
    fflush(stdout);
}

static int parse_number(const char *s, float *out)
{
    char *end;
    float v;

    if (s == NULL || *s == '\0')
        return -1;
    errno = 0;
    v = strtof(s, &end);
    if (end == s || *end != '\0' || errno == ERANGE)
        return -1;
    if (!isfinite(v))
        return -1;
    *out = v;
    return 0;
}

static void table_free(struct fcas_local_table *tbl)
{
    if (tbl == NULL)
        return;
    free(tbl->data);
    tbl->data = NULL;
    tbl->row_count = 0;
}

static float table_get(const struct fcas_local_table *tbl, uint16_t row, int col)
{
    return tbl->data[(size_t)row * DB_FCAS_INJECT_MAX_COLS + (size_t)col];
}

static void table_set(struct fcas_local_table *tbl, uint16_t row, int col, float v)
{
    tbl->data[(size_t)row * DB_FCAS_INJECT_MAX_COLS + (size_t)col] = v;
}

static void publish_ctrl(struct db_fcas_inject_type *inj,
                         const struct fcas_local_table *tbl,
                         uint8_t chip, uint32_t state)
{
    struct db_fcas_inject_type ctrl;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.magic = DB_FCAS_INJECT_MAGIC;
    ctrl.state = state;
    ctrl.chip = chip;
    memcpy(ctrl.col_mask, tbl->col_mask, sizeof(ctrl.col_mask));
    *inj = ctrl;
}

static int load_csv(const char *path, struct fcas_local_table *tbl)
{
    FILE *fp;
    char line[FCAS_LINE_MAX];
    int header_done = 0;
    int row = 0;
    int ncol = 0;
    int seen[DB_FCAS_INJECT_MAX_COLS];
    int i;
    int line_no = 0;
    size_t nbytes;

    memset(tbl, 0, sizeof(*tbl));
    memset(seen, 0, sizeof(seen));
    s_ncol = 0;
    for (i = 0; i < DB_FCAS_INJECT_MAX_COLS; i++)
        s_col_map[i] = -1;

    nbytes = (size_t)DB_FCAS_INJECT_MAX_ROWS * DB_FCAS_INJECT_MAX_COLS * sizeof(float);
    tbl->data = (float *)calloc(1, nbytes);
    if (tbl->data == NULL) {
        fprintf(stderr, "malloc csv table failed (%zu bytes)\n", nbytes);
        return -1;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "open csv failed: %s (%s)\n", path, strerror(errno));
        table_free(tbl);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *p;
        char *save = NULL;
        char *tok;
        int field;
        int expect;

        line_no++;
        if (strchr(line, '\n') == NULL && !feof(fp)) {
            fprintf(stderr, "csv line %d too long\n", line_no);
            fclose(fp);
            table_free(tbl);
            return -1;
        }
        p = trim(line);
        if (*p == 0 || *p == '#')
            continue;

        if (!header_done) {
            field = 0;
            ncol = 0;
            for (tok = strtok_r(p, ",", &save); tok != NULL; tok = strtok_r(NULL, ",", &save)) {
                tok = trim(tok);
                if (*tok == '\0') {
                    fprintf(stderr, "csv header: empty column at line %d\n", line_no);
                    fclose(fp);
                    table_free(tbl);
                    return -1;
                }
                if (field == 0) {
                    if (strcasecmp(tok, "index") != 0) {
                        fprintf(stderr, "csv header: first column must be index, got '%s'\n", tok);
                        fclose(fp);
                        table_free(tbl);
                        return -1;
                    }
                } else {
                    int en = 0;
                    int col = db_fcas_parse_header_token(tok, &en);
                    if (col < 0) {
                        fprintf(stderr, "csv header invalid column: %s\n", tok);
                        fclose(fp);
                        table_free(tbl);
                        return -1;
                    }
                    if (seen[col]) {
                        fprintf(stderr, "csv header duplicate column: %s\n", db_fcas_col_name(col));
                        fclose(fp);
                        table_free(tbl);
                        return -1;
                    }
                    if (ncol >= DB_FCAS_INJECT_MAX_COLS) {
                        fprintf(stderr, "csv exceeds %d data columns\n", DB_FCAS_INJECT_MAX_COLS);
                        fclose(fp);
                        table_free(tbl);
                        return -1;
                    }
                    seen[col] = 1;
                    s_col_map[ncol] = col;
                    tbl->col_mask[col] = (en != 0) ? 1 : 0;
                    ncol++;
                }
                field++;
            }
            if (ncol < 1) {
                fprintf(stderr, "csv header has no data columns\n");
                fclose(fp);
                table_free(tbl);
                return -1;
            }
            s_ncol = ncol;
            header_done = 1;
            continue;
        }

        if (row >= DB_FCAS_INJECT_MAX_ROWS) {
            fprintf(stderr, "csv exceeds %d data rows\n", DB_FCAS_INJECT_MAX_ROWS);
            fclose(fp);
            table_free(tbl);
            return -1;
        }

        field = 0;
        expect = ncol + 1;
        for (tok = strtok_r(p, ",", &save); tok != NULL; tok = strtok_r(NULL, ",", &save)) {
            tok = trim(tok);
            if (*tok == '\0') {
                fprintf(stderr, "csv row %d: empty field\n", row);
                fclose(fp);
                table_free(tbl);
                return -1;
            }
            if (field == 0) {
                char *end = NULL;
                errno = 0;
                strtol(tok, &end, 10);
                if (end == tok || *end != '\0' || errno == ERANGE) {
                    fprintf(stderr, "csv row %d: bad index '%s'\n", row, tok);
                    fclose(fp);
                    table_free(tbl);
                    return -1;
                }
            } else if (field < expect) {
                float v;
                if (parse_number(tok, &v) != 0) {
                    fprintf(stderr, "csv row %d col %s: bad number '%s'\n",
                            row, db_fcas_col_name(s_col_map[field - 1]), tok);
                    fclose(fp);
                    table_free(tbl);
                    return -1;
                }
                table_set(tbl, (uint16_t)row, s_col_map[field - 1], v);
            }
            field++;
        }
        if (field != expect) {
            fprintf(stderr, "csv row %d: got %d fields, header has %d\n", row, field, expect);
            fclose(fp);
            table_free(tbl);
            return -1;
        }
        row++;
    }

    fclose(fp);

    if (!header_done || row == 0) {
        fprintf(stderr, "csv has no data rows\n");
        table_free(tbl);
        return -1;
    }

    tbl->row_count = (uint16_t)row;
    memcpy(s_mask, tbl->col_mask, sizeof(s_mask));
    return 0;
}

static void show_progress(int cycle, int cycles, uint16_t row, uint16_t rows)
{
    uint32_t total = (uint32_t)cycles * rows;
    uint32_t done = (uint32_t)(cycle - 1) * rows + row + 1;

    printf("\033[2J\033[H");
    printf("FCAS inject\n");
    printf("cycles     : %d/%d\n", cycle, cycles);
    printf("current row: %u/%u\n", (unsigned)(row + 1), (unsigned)rows);
    printf("progress   : %.1f%%\n", total ? (100.0 * done / total) : 100.0);
    fflush(stdout);
}

static void inject_row(struct db_basic_realtime_type *half,
                       const struct fcas_local_table *tbl,
                       uint16_t row)
{
    int col;
    for (col = 0; col < DB_FCAS_INJECT_MAX_COLS; col++) {
        if (tbl->col_mask[col])
            db_fcas_write_col(half, col, table_get(tbl, row, col));
    }
}

static int log_open_header(FILE *fp, const struct fcas_local_table *tbl)
{
    int col;
    fprintf(fp, "# FCAS inject log: readback from halfwave.primary, eng units\n");
    fprintf(fp, "# state after play -> HOLD; mask:");
    for (col = 0; col < DB_FCAS_INJECT_MAX_COLS; col++) {
        if (tbl->col_mask[col])
            fprintf(fp, " %s=1", db_fcas_col_name(col));
    }
    fprintf(fp, "\n");
    fprintf(fp, "index");
    for (col = 0; col < DB_FCAS_INJECT_MAX_COLS; col++)
        fprintf(fp, ",%s", db_fcas_col_name(col));
    fprintf(fp, "\n");
    fflush(fp);
    return 0;
}

static void log_row(FILE *fp, uint16_t index,
                    const struct db_basic_realtime_type *half)
{
    int col;
    fprintf(fp, "%u", (unsigned)index);
    for (col = 0; col < DB_FCAS_INJECT_MAX_COLS; col++) {
        float v = db_fcas_read_col(half, col);
        if (col == DB_FCAS_COL_FREQ_HZ)
            fprintf(fp, ",%.4f", v);
        else if (col >= DB_FCAS_COL_PFA)
            fprintf(fp, ",%.3f", v);
        else if (col <= DB_FCAS_COL_VC)
            fprintf(fp, ",%.2f", v);
        else if (col <= DB_FCAS_COL_IC)
            fprintf(fp, ",%.3f", v);
        else
            fprintf(fp, ",%.3f", v);
    }
    fprintf(fp, "\n");
    fflush(fp);
}

int main(int argc, char **argv)
{
    int opt;
    const char *csv_path = FCAS_DEFAULT_CSV;
    const char *log_path = NULL;
    int chip = 0;
    int do_clear = 0;
    int cycles = 1;
    int slot_ms = DB_FCAS_INJECT_SLOT_MS;
    int show_ui = 0;
    int cyc;
    void *vmem;
    struct db_database_type *pdb;
    struct db_fcas_inject_type *inj;
    struct db_basic_realtime_type *half;
    struct fcas_local_table tbl;
    FILE *logfp = NULL;
    uint16_t row;
    uint16_t last_row;

    memset(&tbl, 0, sizeof(tbl));

    while ((opt = getopt(argc, argv, "f:o:c:T:t:Ch")) != -1) {
        switch (opt) {
        case 'f':
            csv_path = optarg;
            break;
        case 'o':
            log_path = optarg;
            break;
        case 'c':
            chip = atoi(optarg);
            break;
        case 'T':
            cycles = atoi(optarg);
            show_ui = 1;
            if (cycles < 1) {
                fprintf(stderr, "-T must be >= 1\n");
                return 1;
            }
            break;
        case 't':
            slot_ms = atoi(optarg);
            if (slot_ms < 1) {
                fprintf(stderr, "-t must be >= 1 ms\n");
                return 1;
            }
            break;
        case 'C':
            do_clear = 1;
            break;
        case 'h':
        default:
            usage(argv[0]);
            return (opt == 'h') ? 0 : 1;
        }
    }

    if (chip < 0 || chip >= NUM) {
        fprintf(stderr, "chip must be 0..%d\n", NUM - 1);
        return 1;
    }

    vmem = rt_database_attach();
    if (vmem == NULL) {
        fprintf(stderr, "rt_database_attach failed (/dev/vmem)\n");
        return 1;
    }
    pdb = (struct db_database_type *)vmem;
    inj = &pdb->db_fcas_inject;
    half = &pdb->db_measurebase[chip].db_measure.numeric.halfwave.primary;

    if (do_clear) {
        db_fcas_inject_clear(inj);
        fprintf(stderr, "fcas inject cleared (IDLE)\n");
        rt_database_deattach();
        return 0;
    }

    if (load_csv(csv_path, &tbl) != 0) {
        rt_database_deattach();
        return 1;
    }

    /* 先落 mask，再置 PLAYING；数值由本进程 inject_row 写入半波 SHM */
    publish_ctrl(inj, &tbl, (uint8_t)chip, DB_FCAS_INJECT_STATE_PLAYING);

    if (log_path != NULL) {
        logfp = fopen(log_path, "w");
        if (logfp == NULL) {
            fprintf(stderr, "open log failed: %s (%s)\n", log_path, strerror(errno));
            db_fcas_inject_clear(inj);
            table_free(&tbl);
            rt_database_deattach();
            return 1;
        }
        log_open_header(logfp, &tbl);
    }

    fprintf(stderr, "fcas_inject: csv=%s rows=%u cycles=%d slot=%dms chip=%d log=%s\n",
            csv_path, (unsigned)tbl.row_count, cycles, slot_ms, chip,
            log_path ? log_path : "(none)");
    if (!show_ui)
        print_csv_header();

    for (cyc = 0; cyc < cycles; cyc++) {
        for (row = 0; row < tbl.row_count; row++) {
            if (show_ui)
                show_progress(cyc + 1, cycles, row, tbl.row_count);

            inject_row(half, &tbl, row);
            if (logfp)
                log_row(logfp, row, half);
            if (!(cyc + 1 == cycles && row + 1 == tbl.row_count))
                usleep((useconds_t)slot_ms * 1000);
        }
    }

    if (show_ui) {
        printf("\033[2J\033[H");
        printf("FCAS inject\n");
        printf("cycles     : %d/%d\n", cycles, cycles);
        printf("current row: %u/%u\n", (unsigned)tbl.row_count, (unsigned)tbl.row_count);
        printf("progress   : 100.0%%\n");
        printf("fcas_inject: csv=%s rows=%u cycles=%d slot=%dms chip=%d log=%s\n",
               csv_path, (unsigned)tbl.row_count, cycles, slot_ms, chip,
               log_path ? log_path : "(none)");
        print_csv_header();
    }

    /* 播完：半波 SHM 保持最后一行；vmem 仅留 mask+HOLD；本地表释放 */
    last_row = (tbl.row_count > 0) ? (uint16_t)(tbl.row_count - 1) : 0;
    inject_row(half, &tbl, last_row);
    publish_ctrl(inj, &tbl, (uint8_t)chip, DB_FCAS_INJECT_STATE_HOLD);
    fprintf(stderr, "fcas_inject: HOLD at row %u (run '/ego/bin/fcas_inject -C' to clear, restore msr publish)\n",
            (unsigned)last_row);

    table_free(&tbl);

    if (logfp)
        fclose(logfp);
    rt_database_deattach();
    return 0;
}
