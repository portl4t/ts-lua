
#include "ts_lua_util.h"


static int ts_lua_get_now_time(lua_State *L);
static int ts_lua_debug(lua_State *L);


void
ts_lua_inject_misc_api(lua_State *L)
{
    /* ts.now() */
    lua_pushcfunction(L, ts_lua_get_now_time);
    lua_setfield(L, -2, "now");

    lua_pushcfunction(L, ts_lua_debug);
    lua_setfield(L, -2, "debug");
}

static int
ts_lua_get_now_time(lua_State *L)
{
    time_t    now;

    now = TShrtime() / 1000000000;
    lua_pushnumber(L, now);
    return 1;
}

static int
ts_lua_debug(lua_State *L)
{
    const char      *msg;

    msg = luaL_checkstring(L, 1);
    TSDebug(TS_LUA_DEBUG_TAG, msg);
    return 0;
}

