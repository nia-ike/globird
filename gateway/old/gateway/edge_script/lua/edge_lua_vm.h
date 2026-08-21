#ifndef EDGE_LUA_VM_H
#define EDGE_LUA_VM_H

#include "edge_lua_ctx.h"

struct lua_State;

typedef struct EdgeLuaVm
{
    struct lua_State *L;
    EdgeLuaWorkerCtx worker;
} EdgeLuaVm;

int edge_lua_vm_open(EdgeLuaVm *vm, const char *script_id, int debug);
void edge_lua_vm_close(EdgeLuaVm *vm);
int edge_lua_vm_load_file(EdgeLuaVm *vm, const char *path);
int edge_lua_vm_call_run(EdgeLuaVm *vm);

#endif /* EDGE_LUA_VM_H */
