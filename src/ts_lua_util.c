
#include "ts_lua_util.h"
#include "ts_lua_client_request.h"

static void ts_lua_init_registry(lua_State *L);
static void ts_lua_init_globals(lua_State *L);
static void ts_lua_inject_ts_api(lua_State *L);


lua_State *
ts_lua_new_state()
{
    lua_State       *L;

    L = luaL_newstate();

    if (L == NULL) {
        return NULL;
    }

    luaL_openlibs(L);

    ts_lua_init_registry(L);

    ts_lua_init_globals(L);

    return L;
}

static
void ts_lua_init_registry(lua_State *L)
{
    return;
}

static
void ts_lua_init_globals(lua_State *L)
{
    ts_lua_inject_ts_api(L);
}

static void
ts_lua_inject_ts_api(lua_State *L)
{
    lua_newtable(L);

    ts_lua_inject_client_request_api(L);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaded");
    lua_pushvalue(L, -3);
    lua_setfield(L, -2, "ts");
    lua_pop(L, 2);

    lua_setglobal(L, "ts");
}

void
ts_lua_set_ctx(lua_State *L, ts_lua_ctx  *ctx)
{
    lua_pushliteral(L, "__ts_ctx");
    lua_pushlightuserdata(L, ctx);
    lua_rawset(L, LUA_GLOBALSINDEX);
}

ts_lua_ctx *
ts_lua_get_ctx(lua_State *L)
{
    ts_lua_ctx  *ctx;

    lua_pushliteral(L, "__ts_ctx");
    lua_rawget(L, LUA_GLOBALSINDEX);
    ctx = lua_touserdata(L, -1);

    lua_pop(L, 1);                      // pop the ctx out

    return ctx;
}

