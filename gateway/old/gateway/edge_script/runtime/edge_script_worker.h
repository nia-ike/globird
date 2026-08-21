#ifndef EDGE_SCRIPT_WORKER_H
#define EDGE_SCRIPT_WORKER_H

#include "../edge_script_config.h"

typedef struct EdgeScriptWorker EdgeScriptWorker;

int edge_script_worker_start(EdgeScriptWorker **out_worker, const EdgeScriptEntry *entry, int debug);
void edge_script_worker_stop(EdgeScriptWorker *worker);
void edge_script_worker_join(EdgeScriptWorker *worker);

#endif /* EDGE_SCRIPT_WORKER_H */
