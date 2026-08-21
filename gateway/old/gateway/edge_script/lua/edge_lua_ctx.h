#ifndef EDGE_LUA_CTX_H
#define EDGE_LUA_CTX_H

struct lua_State;

typedef struct EdgeLuaWorkerCtx
{
    volatile int stop;
    int debug;
    const char *script_id;
} EdgeLuaWorkerCtx;

#define EDGE_LUA_ERR_STOP  "edge_script worker stopped"

void edge_lua_ctx_register(struct lua_State *L, EdgeLuaWorkerCtx *worker);

#endif /* EDGE_LUA_CTX_H */
