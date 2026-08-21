#include "dlt698_45_cfg.h"

static char *cfg_read_file(const char *file)
{
    FILE *fp;
    long len;
    char *buf;

    fp = fopen(file, "rb");
    if (!fp) {
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (len <= 0) {
        fclose(fp);
        return NULL;
    }
    buf = (char *)calloc(1, len + 1);
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    fread(buf, 1, len, fp);
    fclose(fp);
    return buf;
}

int dlt698_45_cfg_load(struct config_devices_attr *pCfg, char *file)
{
    int i;
    cJSON *root = NULL, *attr = NULL, *node = NULL, *node_attr = NULL, *item = NULL;
    char *json = NULL;

    if (!pCfg || !file) {
        return -1;
    }
    memset(pCfg, 0, sizeof(*pCfg));

    json = cfg_read_file(file);
    if (!json) {
        return -1;
    }

    root = cJSON_Parse(json);
    free(json);
    if (!root) {
        return -1;
    }

    attr = cJSON_GetObjectItem(root, "attr");
    if (!attr) {
        cJSON_Delete(root);
        return -1;
    }

    item = cJSON_GetObjectItem(attr, "checked");
    if (cJSON_IsNumber(item)) pCfg->checked = (uint8_t)item->valueint;
    item = cJSON_GetObjectItem(attr, "sn");
    if (cJSON_IsString(item) && item->valuestring) strncpy(pCfg->sn, item->valuestring, sizeof(pCfg->sn) - 1);
    item = cJSON_GetObjectItem(attr, "model");
    if (cJSON_IsString(item) && item->valuestring) strncpy(pCfg->model, item->valuestring, sizeof(pCfg->model) - 1);
    item = cJSON_GetObjectItem(attr, "name");
    if (cJSON_IsString(item) && item->valuestring) strncpy(pCfg->name, item->valuestring, sizeof(pCfg->name) - 1);
    item = cJSON_GetObjectItem(attr, "function");
    if (cJSON_IsString(item) && item->valuestring) strncpy(pCfg->function, item->valuestring, sizeof(pCfg->function) - 1);

    node = cJSON_GetObjectItem(attr, "node");
    if (!node) {
        cJSON_Delete(root);
        return -1;
    }
    item = cJSON_GetObjectItem(node, "node_num");
    if (cJSON_IsNumber(item)) {
        pCfg->node_num = (item->valueint > CFG_MAX_POINT) ? CFG_MAX_POINT : (uint16_t)item->valueint;
    }
    node_attr = cJSON_GetObjectItem(node, "node_attr");
    if (!cJSON_IsArray(node_attr)) {
        cJSON_Delete(root);
        return -1;
    }

    for (i = 0; i < pCfg->node_num; i++) {
        cJSON *e = cJSON_GetArrayItem(node_attr, i);
        if (!e) break;

        item = cJSON_GetObjectItem(e, "id");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].id = (uint16_t)item->valueint;
        item = cJSON_GetObjectItem(e, "datatype");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].datatype = (uint8_t)item->valueint;
        item = cJSON_GetObjectItem(e, "leadnum");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].leadnum = (uint8_t)item->valueint;
        item = cJSON_GetObjectItem(e, "code");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].code = (uint8_t)item->valueint;
        item = cJSON_GetObjectItem(e, "datalen");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].datalen = (uint8_t)item->valueint;
        item = cJSON_GetObjectItem(e, "reg_no");
        if (cJSON_IsNumber(item)) {
            pCfg->node_attr[i].reg_no = (uint32_t)item->valuedouble;
        } else if (cJSON_IsString(item) && item->valuestring) {
            /* 兼容 "0x100201" 或 "1049089" 这类字符串配置 */
            char *end = NULL;
            unsigned long v = strtoul(item->valuestring, &end, 0);
            if (end != item->valuestring) {
                pCfg->node_attr[i].reg_no = (uint32_t)v;
            }
        }
        item = cJSON_GetObjectItem(e, "catagory");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].catagory = (uint32_t)item->valueint;
        item = cJSON_GetObjectItem(e, "kfactor");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].kfactor = (float)item->valuedouble;
        else pCfg->node_attr[i].kfactor = 1.0f;
        item = cJSON_GetObjectItem(e, "pt_flag1");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].pt_flag1 = (float)item->valuedouble;
        else pCfg->node_attr[i].pt_flag1 = 1.0f;
        item = cJSON_GetObjectItem(e, "ct_flag2");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].ct_flag2 = (float)item->valuedouble;
        else pCfg->node_attr[i].ct_flag2 = 1.0f;
        item = cJSON_GetObjectItem(e, "range_checked");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].range_checked = (uint8_t)item->valueint;
        item = cJSON_GetObjectItem(e, "range_min");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].range_min = item->valueint;
        item = cJSON_GetObjectItem(e, "range_max");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].range_max = item->valueint;
        item = cJSON_GetObjectItem(e, "dataoffset");
        if (cJSON_IsNumber(item)) pCfg->node_attr[i].dataoffset = (uint16_t)item->valueint;
    }

    pCfg->node_num = (uint16_t)cJSON_GetArraySize(node_attr);
    if (pCfg->node_num > CFG_MAX_POINT) pCfg->node_num = CFG_MAX_POINT;
    cJSON_Delete(root);
    return 0;
}

int32_t dlt698_45_cfg_node_attr_size(struct config_devices_attr *pDevAttr)
{
    return pDevAttr ? pDevAttr->node_num : 0;
}

struct config_node_attr *dlt698_45_cfg_node_attr(struct config_devices_attr *pDevAttr, uint8_t id)
{
    if (!pDevAttr || id >= pDevAttr->node_num) return NULL;
    return &pDevAttr->node_attr[id];
}

int32_t dlt698_45_cfg_nodectrl_attr_size(struct config_devices_attr *pDevAttr)
{
    return pDevAttr ? pDevAttr->control_num : 0;
}

struct config_control_attr *dlt698_45_cfg_nodectrl_attr(struct config_devices_attr *pDevAttr, uint8_t id)
{
    if (!pDevAttr || id >= pDevAttr->control_num) return NULL;
    return &pDevAttr->control_attr[id];
}
