#ifndef EDGE_SCRIPT_SERVICE_H
#define EDGE_SCRIPT_SERVICE_H

#include "../edge_script_config.h"

typedef struct EdgeScriptService EdgeScriptService;

EdgeScriptService *edge_script_service_create(const char *config_base, int debug);
void edge_script_service_destroy(EdgeScriptService *svc);
int edge_script_service_reload(EdgeScriptService *svc);
int edge_script_service_poll_reload(EdgeScriptService *svc, unsigned interval_sec);

#endif /* EDGE_SCRIPT_SERVICE_H */
