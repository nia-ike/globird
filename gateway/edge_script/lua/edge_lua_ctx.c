#include "edge_lua_ctx.h"

#include "../point/edge_point_util.h"
#include "../point/edge_point_write.h"
#include "../log/edge_log.h"

#include <stdio.h>
#include <unistd.h>

#include "edge_lua_compat.h"
#include "lauxlib.h"

#define EDGE_LUA_CTX_KEY "edge_worker_ctx"

/*************************************************************************/
/** Function Name:       edge_lua_throw_if_stopped                         **/
/** Purpose:             热 reload 置 stop 后抛出 Lua 错误以退出 run(ctx)   **/
/** Params:            L - Lua 状态机                                      **/
/** Return:            无正常返回（stop 时 longjmp）                       **/
/** Notice:            不可仅 break sleep，否则 while true 会空转           **/
/*************************************************************************/
static void edge_lua_throw_if_stopped(lua_State *L, EdgeLuaWorkerCtx *worker)
{
    if (worker && worker->stop)
        luaL_error(L, EDGE_LUA_ERR_STOP);
}

/*************************************************************************/
/** Function Name:       edge_lua_get_worker                             **/
/** Purpose:             从 Lua 注册表获取当前 worker 上下文指针            **/
/** Params:            L - Lua 状态机                                      **/
/** Return:            worker 指针；未注册时可能为 NULL                    **/
/** Notice:            供 sleep 等接口检查 stop 标志                       **/
/*************************************************************************/
static EdgeLuaWorkerCtx *edge_lua_get_worker(lua_State *L)
{
    EdgeLuaWorkerCtx *ctx;

    lua_getfield(L, LUA_REGISTRYINDEX, EDGE_LUA_CTX_KEY);
    ctx = (EdgeLuaWorkerCtx *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ctx;
}

/*************************************************************************/
/** Function Name:       edge_lua_parse_id                                 **/
/** Purpose:             从 Lua 栈指定位置解析 dblink 十六进制字符串        **/
/** Params:            L - Lua 状态机；idx - 参数栈索引                    **/
/** Return:            解析后的 uint32_t dblink_id                         **/
/** Notice:            解析失败时抛出 Lua 错误                             **/
/*************************************************************************/
static uint32_t edge_lua_parse_id(lua_State *L, int idx)
{
    const char *hex_str;
    uint32_t id;

    hex_str = luaL_checkstring(L, idx);
    if (edge_dblink_parse(hex_str, &id) != 0)
        luaL_error(L, "invalid dblink: %s", hex_str);
    return id;
}

/*************************************************************************/
/** Function Name:       edge_lua_readpoint                                **/
/** Purpose:             ctx:readpoint(dblink) Lua 绑定                    **/
/** Params:            L - Lua 状态机（arg2 为 dblink 字符串）             **/
/** Return:            1（返回 number 测点值，失败时为 0）                 **/
/** Notice:            使用冒号调用，self 占栈索引 1                       **/
/*************************************************************************/
static int edge_lua_readpoint(lua_State *L)
{
    EdgeLuaWorkerCtx *worker = edge_lua_get_worker(L);

    edge_lua_throw_if_stopped(L, worker);
    uint32_t id = edge_lua_parse_id(L, 2);
    double val = 0.0;

    if (edge_point_read(id, &val, 0) != 0)
        lua_pushnumber(L, 0);
    else
        lua_pushnumber(L, val);
    return 1;
}

/*************************************************************************/
/** Function Name:       edge_lua_read_ctrlpoint                           **/
/** Purpose:             ctx:read_ctrlpoint(dblink) Lua 绑定               **/
/** Params:            L - Lua 状态机（arg2 为 dblink 字符串）             **/
/** Return:            1（返回 number 控制点值，失败时为 0）               **/
/** Notice:            仅走 readctrlpoint 路径                             **/
/*************************************************************************/
static int edge_lua_read_ctrlpoint(lua_State *L)
{
    EdgeLuaWorkerCtx *worker = edge_lua_get_worker(L);

    edge_lua_throw_if_stopped(L, worker);
    uint32_t id = edge_lua_parse_id(L, 2);
    double val = 0.0;

    if (edge_point_read(id, &val, 1) != 0)
        lua_pushnumber(L, 0);
    else
        lua_pushnumber(L, val);
    return 1;
}

/*************************************************************************/
/** Function Name:       edge_lua_writepoint                               **/
/** Purpose:             ctx:writepoint(dblink, val) Lua 绑定              **/
/** Params:            L - Lua 状态机（arg2 dblink，arg3 数值）           **/
/** Return:            0                                                     **/
/** Notice:            写失败时抛出 Lua 错误                               **/
/*************************************************************************/
static int edge_lua_writepoint(lua_State *L)
{
    EdgeLuaWorkerCtx *worker = edge_lua_get_worker(L);

    edge_lua_throw_if_stopped(L, worker);
    uint32_t id = edge_lua_parse_id(L, 2);
    double val = luaL_checknumber(L, 3);

    if (edge_point_write(id, val, 0) != 0)
        luaL_error(L, "writepoint failed: %X", (unsigned)id);
    return 0;
}

/*************************************************************************/
/** Function Name:       edge_lua_write_ctrlpoint                          **/
/** Purpose:             ctx:write_ctrlpoint(dblink, val) Lua 绑定         **/
/** Params:            L - Lua 状态机（arg2 dblink，arg3 数值）           **/
/** Return:            0                                                     **/
/** Notice:            写失败时抛出 Lua 错误                               **/
/*************************************************************************/
static int edge_lua_write_ctrlpoint(lua_State *L)
{
    EdgeLuaWorkerCtx *worker = edge_lua_get_worker(L);

    edge_lua_throw_if_stopped(L, worker);
    uint32_t id = edge_lua_parse_id(L, 2);
    double val = luaL_checknumber(L, 3);

    if (edge_point_write(id, val, 1) != 0)
        luaL_error(L, "write_ctrlpoint failed: %X", (unsigned)id);
    return 0;
}

/*************************************************************************/
/** Function Name:       edge_lua_sleep                                    **/
/** Purpose:             ctx:sleep(ms) Lua 绑定，毫秒级延时                 **/
/** Params:            L - Lua 状态机（arg2 为毫秒数）                     **/
/** Return:            0                                                     **/
/** Notice:            分段 usleep 以便响应 worker stop 标志               **/
/*************************************************************************/
static int edge_lua_sleep(lua_State *L)
{
    EdgeLuaWorkerCtx *worker = edge_lua_get_worker(L);
    lua_Integer ms = edge_lua_checkinteger(L, 2);
    lua_Integer elapsed = 0;
    const lua_Integer step = 50;

    edge_lua_throw_if_stopped(L, worker);

    if (ms < 0)
        ms = 0;

    while (elapsed < ms)
    {
        edge_lua_throw_if_stopped(L, worker);
        if (ms - elapsed <= step)
        {
            usleep((unsigned int)(ms - elapsed) * 1000U);
            break;
        }
        usleep((unsigned int)step * 1000U);
        elapsed += step;
    }
    return 0;
}

/*************************************************************************/
/** Function Name:       edge_lua_log                                      **/
/** Purpose:             ctx:log(msg) Lua 绑定，输出脚本日志                **/
/** Params:            L - Lua 状态机（arg2 为日志字符串）                 **/
/** Return:            0                                                     **/
/** Notice:            日志前缀带脚本 id                                   **/
/*************************************************************************/
static int edge_lua_log(lua_State *L)
{
    EdgeLuaWorkerCtx *worker = edge_lua_get_worker(L);

    edge_lua_throw_if_stopped(L, worker);
    const char *msg = luaL_checkstring(L, 2);

    if (worker && worker->script_id)
        edge_log_script_info(worker->script_id, "%s", msg);
    else
        edge_log_service_info("%s", msg);
    return 0;
}

static const luaL_Reg edge_lua_ctx_methods[] = {
    {"readpoint", edge_lua_readpoint},
    {"read_ctrlpoint", edge_lua_read_ctrlpoint},
    {"writepoint", edge_lua_writepoint},
    {"write_ctrlpoint", edge_lua_write_ctrlpoint},
    {"sleep", edge_lua_sleep},
    {"log", edge_lua_log},
    {NULL, NULL}
};

/*************************************************************************/
/** Function Name:       edge_lua_ctx_register                             **/
/** Purpose:             注册 ctx 元表及 worker 上下文到 Lua 注册表         **/
/** Params:            L - Lua 状态机；worker - worker 上下文              **/
/** Return:            None.                                               **/
/** Notice:            需在加载脚本前调用                                  **/
/*************************************************************************/
void edge_lua_ctx_register(lua_State *L, EdgeLuaWorkerCtx *worker)
{
    luaL_newmetatable(L, "EdgeScriptCtx");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    edge_lua_setfuncs(L, edge_lua_ctx_methods);
    lua_pop(L, 1);

    lua_pushlightuserdata(L, worker);
    lua_setfield(L, LUA_REGISTRYINDEX, EDGE_LUA_CTX_KEY);
}

/*************************************************************************/
/** Function Name:       edge_lua_ctx_push                                 **/
/** Purpose:             向 Lua 栈压入 ctx 表对象供 run(ctx) 调用           **/
/** Params:            L - Lua 状态机                                      **/
/** Return:            1（栈顶为 ctx 对象）                                **/
/** Notice:            ctx 表挂载 EdgeScriptCtx 元表                       **/
/*************************************************************************/
int edge_lua_ctx_push(lua_State *L)
{
    lua_newtable(L);
    luaL_getmetatable(L, "EdgeScriptCtx");
    lua_setmetatable(L, -2);
    return 1;
}
