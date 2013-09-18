
#include "ts_lua_util.h"

static char ts_http_context_key;

static int ts_lua_context_get(lua_State *L);
static int ts_lua_context_set(lua_State *L);


void
ts_lua_inject_context_api(lua_State *L)
{
    lua_newtable(L);         /* .ctx */

    lua_createtable(L, 0, 2);       /* metatable for context */

    lua_pushcfunction(L, ts_lua_context_get);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, ts_lua_context_set);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2); 

    lua_setfield(L, -2, "ctx");
}

void
ts_lua_create_context_table(lua_State *L)
{
    lua_pushlightuserdata(L, &ts_http_context_key);
    lua_newtable(L);
    lua_rawset(L, LUA_GLOBALSINDEX);
}


static int
ts_lua_context_get(lua_State *L)
{
    const char  *key;
    size_t      key_len;

    key = luaL_checklstring(L, 2, &key_len);

    if (key && key_len) {
        lua_pushlightuserdata(L, &ts_http_context_key);
        lua_rawget(L, LUA_GLOBALSINDEX);                    // get the context table

        lua_pushlstring(L, key, key_len);
        lua_rawget(L, -2);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

static int
ts_lua_context_set(lua_State *L)
{
    const char  *key;
    size_t      key_len;

    key = luaL_checklstring(L, 2, &key_len);

    lua_pushlightuserdata(L, &ts_http_context_key);
    lua_rawget(L, LUA_GLOBALSINDEX);                    // get the context table    -3

    lua_pushlstring(L, key, key_len);                   // push key                 -2
    lua_pushvalue(L, 3);                                // push value               -1

    lua_rawset(L, -3);
    lua_pop(L, 1);                                      // pop the context table

    return 0;
}

