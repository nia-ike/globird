#include "edge_lua_vm.h"

#include "../log/edge_log.h"
#include "../lua/edge_lua_ctx.h"

#include <stdio.h>
#include <string.h>

#include "edge_lua_compat.h"
#include "lauxlib.h"
#include "lualib.h"

extern int edge_lua_ctx_push(lua_State *L);

/*************************************************************************/
/** Function Name:       edge_lua_to_int                                   **/
/** Purpose:             将 Lua number 转为 lua_Integer                      **/
/** Params:            n - Lua 浮点数值                                    **/
/** Return:            整数值                                              **/
/** Notice:            兼容 Lua 5.1/5.3 整数类型差异                       **/
/*************************************************************************/
static lua_Integer edge_lua_to_int(lua_Number n)
{
#if LUA_VERSION_NUM >= 503
    return (lua_Integer)n;
#else
    return (lua_Integer)(long long)n;
#endif
}

/*************************************************************************/
/** Function Name:       edge_bit_band                                     **/
/** Purpose:             bit.band(a, b) 按位与                              **/
/** Params:            L - Lua 状态机                                      **/
/** Return:            1（返回整数结果）                                   **/
/** Notice:            供 Blockly 位运算块使用                             **/
/*************************************************************************/
static int edge_bit_band(lua_State *L)
{
    lua_Integer a = edge_lua_to_int(luaL_checknumber(L, 1));
    lua_Integer b = edge_lua_to_int(luaL_checknumber(L, 2));
    lua_pushinteger(L, a & b);
    return 1;
}

/*************************************************************************/
/** Function Name:       edge_bit_bor                                      **/
/** Purpose:             bit.bor(a, b) 按位或                               **/
/** Params:            L - Lua 状态机                                      **/
/** Return:            1（返回整数结果）                                   **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int edge_bit_bor(lua_State *L)
{
    lua_Integer a = edge_lua_to_int(luaL_checknumber(L, 1));
    lua_Integer b = edge_lua_to_int(luaL_checknumber(L, 2));
    lua_pushinteger(L, a | b);
    return 1;
}

/*************************************************************************/
/** Function Name:       edge_bit_bxor                                     **/
/** Purpose:             bit.bxor(a, b) 按位异或                            **/
/** Params:            L - Lua 状态机                                      **/
/** Return:            1（返回整数结果）                                   **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int edge_bit_bxor(lua_State *L)
{
    lua_Integer a = edge_lua_to_int(luaL_checknumber(L, 1));
    lua_Integer b = edge_lua_to_int(luaL_checknumber(L, 2));
    lua_pushinteger(L, a ^ b);
    return 1;
}

/*************************************************************************/
/** Function Name:       edge_bit_bnot                                     **/
/** Purpose:             bit.bnot(a) 按位取反                               **/
/** Params:            L - Lua 状态机                                      **/
/** Return:            1（返回整数结果）                                   **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int edge_bit_bnot(lua_State *L)
{
    lua_Integer a = edge_lua_to_int(luaL_checknumber(L, 1));
    lua_pushinteger(L, ~a);
    return 1;
}

/*************************************************************************/
/** Function Name:       edge_bit_lshift                                   **/
/** Purpose:             bit.lshift(a, b) 逻辑左移                          **/
/** Params:            L - Lua 状态机                                      **/
/** Return:            1（返回整数结果）；移位量为负时抛错                 **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int edge_bit_lshift(lua_State *L)
{
    lua_Integer a = edge_lua_to_int(luaL_checknumber(L, 1));
    lua_Integer b = edge_lua_to_int(luaL_checknumber(L, 2));
    if (b < 0)
        return luaL_error(L, "bad argument #2 to 'lshift' (shift must be non-negative)");
    lua_pushinteger(L, (lua_Integer)((unsigned)a << (unsigned)b));
    return 1;
}

/*************************************************************************/
/** Function Name:       edge_bit_rshift                                   **/
/** Purpose:             bit.rshift(a, b) 逻辑右移                          **/
/** Params:            L - Lua 状态机                                      **/
/** Return:            1（返回整数结果）；移位量为负时抛错                 **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int edge_bit_rshift(lua_State *L)
{
    lua_Integer a = edge_lua_to_int(luaL_checknumber(L, 1));
    lua_Integer b = edge_lua_to_int(luaL_checknumber(L, 2));
    if (b < 0)
        return luaL_error(L, "bad argument #2 to 'rshift' (shift must be non-negative)");
    lua_pushinteger(L, (lua_Integer)((unsigned)a >> (unsigned)b));
    return 1;
}

/*************************************************************************/
/** Function Name:       edge_lua_register_bit                             **/
/** Purpose:             向 Lua 全局注册 bit 库                             **/
/** Params:            L - Lua 状态机                                      **/
/** Return:            None.                                               **/
/** Notice:            与上位机 Blockly 生成的 bit.* 调用一致              **/
/*************************************************************************/
static void edge_lua_register_bit(lua_State *L)
{
    const luaL_Reg bit_lib[] = {
        {"band", edge_bit_band},
        {"bor", edge_bit_bor},
        {"bxor", edge_bit_bxor},
        {"bnot", edge_bit_bnot},
        {"lshift", edge_bit_lshift},
        {"rshift", edge_bit_rshift},
        {NULL, NULL}
    };

    edge_lua_newlib(L, bit_lib, "bit");
}

/*************************************************************************/
/** Function Name:       edge_lua_vm_open                                  **/
/** Purpose:             创建并初始化脚本 Lua 虚拟机                          **/
/** Params:            vm - VM 对象；script_id - 脚本 ID；debug - 调试标志  **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            打开标准库、bit 库并注册 ctx                        **/
/*************************************************************************/
int edge_lua_vm_open(EdgeLuaVm *vm, const char *script_id, int debug)
{
    if (!vm)
        return -1;

    memset(vm, 0, sizeof(*vm));
    vm->worker.stop = 0;
    vm->worker.debug = debug;
    vm->worker.script_id = script_id;

    vm->L = luaL_newstate();
    if (!vm->L)
        return -1;

    luaL_openlibs(vm->L);
    edge_lua_register_bit(vm->L);
    edge_lua_ctx_register(vm->L, &vm->worker);
    return 0;
}

/*************************************************************************/
/** Function Name:       edge_lua_vm_close                                 **/
/** Purpose:             关闭 Lua 虚拟机并释放资源                          **/
/** Params:            vm - VM 对象                                        **/
/** Return:            None.                                               **/
/** Notice:            vm 为 NULL 时直接返回                               **/
/*************************************************************************/
void edge_lua_vm_close(EdgeLuaVm *vm)
{
    if (!vm)
        return;
    if (vm->L)
    {
        lua_close(vm->L);
        vm->L = NULL;
    }
}

/*************************************************************************/
/** Function Name:       edge_lua_vm_load_file                             **/
/** Purpose:             加载并执行 Lua 脚本文件（定义 run 等全局函数）     **/
/** Params:            vm - VM 对象；path - 脚本文件路径                   **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            先 loadfile 再 pcall 执行顶层代码                   **/
/*************************************************************************/
int edge_lua_vm_load_file(EdgeLuaVm *vm, const char *path)
{
    if (!vm || !vm->L || !path)
        return -1;

    if (luaL_loadfile(vm->L, path) != EDGE_LUA_OK)
    {
        const char *err = lua_tostring(vm->L, -1);
        edge_log_script_error(vm->worker.script_id, "load %s failed: %s",
                              path, err ? err : "(unknown)");
        fprintf(stderr, "[edge_script] load %s failed: %s\n", path, err ? err : "(unknown)");
        lua_pop(vm->L, 1);
        return -1;
    }

    if (lua_pcall(vm->L, 0, 0, 0) != EDGE_LUA_OK)
    {
        const char *err = lua_tostring(vm->L, -1);
        edge_log_script_error(vm->worker.script_id, "exec %s failed: %s",
                              path, err ? err : "(unknown)");
        fprintf(stderr, "[edge_script] exec %s failed: %s\n", path, err ? err : "(unknown)");
        lua_pop(vm->L, 1);
        return -1;
    }

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_lua_vm_call_run                              **/
/** Purpose:             调用脚本全局函数 run(ctx)                          **/
/** Params:            vm - VM 对象                                        **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            run 未定义或执行出错时返回 -1                       **/
/*************************************************************************/
int edge_lua_vm_call_run(EdgeLuaVm *vm)
{
    if (!vm || !vm->L)
        return -1;

    lua_getglobal(vm->L, "run");
    if (!lua_isfunction(vm->L, -1))
    {
        edge_log_script_error(vm->worker.script_id, "run(ctx) not found");
        fprintf(stderr, "[edge_script] run(ctx) not found\n");
        lua_pop(vm->L, 1);
        return -1;
    }

    edge_lua_ctx_push(vm->L);
    if (lua_pcall(vm->L, 1, 0, 0) != EDGE_LUA_OK)
    {
        const char *err = lua_tostring(vm->L, -1);
        if (!err || strcmp(err, EDGE_LUA_ERR_STOP) != 0)
        {
            edge_log_script_error(vm->worker.script_id, "run(ctx) error: %s",
                                  err ? err : "(unknown)");
            fprintf(stderr, "[edge_script] run(ctx) error: %s\n", err ? err : "(unknown)");
        }
        lua_pop(vm->L, 1);
        return -1;
    }

    return 0;
}
