
#include "ts_lua_hook.h"
#include "ts_lua_util.h"


typedef enum {
    TS_LUA_HOOK_DUMMY = 0,
    TS_LUA_HOOK_SEND_RESPONSE_HDR,
    TS_LUA_HOOK_LAST
} TSLuaHookID;


char * ts_lua_hook_id_string[] = {
    "TS_LUA_HOOK_DUMMY",
    "TS_LUA_HOOK_SEND_RESPONSE_HDR",
    "TS_LUA_HOOK_LAST"
};


static int ts_lua_add_hook(lua_State *L);
static void ts_lua_inject_hook_variables(lua_State *L);


void
ts_lua_inject_hook_api(lua_State *L)
{
    lua_pushcfunction(L, ts_lua_add_hook);
    lua_setfield(L, -2, "hook");

    ts_lua_inject_hook_variables(L);
}

static void
ts_lua_inject_hook_variables(lua_State *L)
{
    int     i;

    for (i = 0; i < sizeof(ts_lua_hook_id_string)/sizeof(char*); i++) {
        lua_pushinteger(L, i);
        lua_setglobal(L, ts_lua_hook_id_string[i]);
    }
}

static int
ts_lua_add_hook(lua_State *L)
{
    int                 type;
    int                 entry;

    ts_lua_http_ctx     *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    entry = lua_tointeger(L, 1);            // get hook id

    type = lua_type(L, 2);
    if (type != LUA_TFUNCTION)
        return 0;

    switch (entry) {

        case TS_LUA_HOOK_SEND_RESPONSE_HDR:
            TSHttpTxnHookAdd(http_ctx->txnp, TS_HTTP_SEND_RESPONSE_HDR_HOOK, http_ctx->main_contp);
            lua_pushvalue(L, 2);
            lua_setglobal(L, TS_LUA_FUNCTION_SEND_RESPONSE);
            break;

        default:
            break;
    }

    return 0;
}

