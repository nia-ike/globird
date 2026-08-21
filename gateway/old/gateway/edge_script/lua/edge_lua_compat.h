#ifndef EDGE_LUA_COMPAT_H
#define EDGE_LUA_COMPAT_H

#include "lua.h"

#if LUA_VERSION_NUM >= 502
#define EDGE_LUA_OK LUA_OK
#else
#define EDGE_LUA_OK 0
#endif

#if LUA_VERSION_NUM >= 502
#define edge_lua_setfuncs(L, funcs) luaL_setfuncs((L), (funcs), 0)
#define edge_lua_checkinteger(L, idx) luaL_checkinteger((L), (idx))
#define edge_lua_newlib(L, funcs, name) \
    do { \
        luaL_newlib((L), (funcs)); \
        lua_setglobal((L), (name)); \
    } while (0)
#else
#define edge_lua_setfuncs(L, funcs) luaL_register((L), NULL, (funcs))
#define edge_lua_checkinteger(L, idx) ((lua_Integer)luaL_checknumber((L), (idx)))
#define edge_lua_newlib(L, funcs, name) luaL_register((L), (name), (funcs))
#endif

#endif /* EDGE_LUA_COMPAT_H */
