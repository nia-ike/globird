#ifndef EDGE_SCRIPT_CONFIG_H
#define EDGE_SCRIPT_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define EDGE_SCRIPT_CFG_BASE_PATH_LEN   256
#define EDGE_SCRIPT_ID_LEN              64
#define EDGE_SCRIPT_NAME_LEN            128
#define EDGE_SCRIPT_PATH_LEN            256
#define EDGE_SCRIPT_MAX_SCRIPTS         32
#define EDGE_SCRIPT_VERSION_LEN         16

typedef struct EdgeScriptTrigger
{
    char type[32];
} EdgeScriptTrigger;

typedef struct EdgeScriptEntry
{
    char id[EDGE_SCRIPT_ID_LEN];
    char name[EDGE_SCRIPT_NAME_LEN];
    bool enabled;
    char script_file[EDGE_SCRIPT_PATH_LEN];
    EdgeScriptTrigger trigger;
    char script_path[EDGE_SCRIPT_PATH_LEN];
} EdgeScriptEntry;

typedef struct EdgeScriptConfig
{
    char version[EDGE_SCRIPT_VERSION_LEN];
    unsigned script_count;
    EdgeScriptEntry scripts[EDGE_SCRIPT_MAX_SCRIPTS];
} EdgeScriptConfig;

#endif /* EDGE_SCRIPT_CONFIG_H */
