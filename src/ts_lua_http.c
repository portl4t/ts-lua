
#include "ts_lua_util.h"


static int ts_lua_http_set_retstatus(lua_State *L);
static int ts_lua_http_set_retbody(lua_State *L);


void
ts_lua_inject_http_api(lua_State *L)
{
    lua_newtable(L);

    lua_pushcfunction(L, ts_lua_http_set_retstatus);
    lua_setfield(L, -2, "set_retstatus");

    lua_pushcfunction(L, ts_lua_http_set_retbody);
    lua_setfield(L, -2, "set_retbody");


    lua_setfield(L, -2, "http");
}


static int
ts_lua_http_set_retstatus(lua_State *L)
{
    int                 status;
    ts_lua_http_ctx     *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    status = luaL_checkinteger(L, 1);
    TSHttpTxnSetHttpRetStatus(http_ctx->txnp, status);
    return 0;
}

static int
ts_lua_http_set_retbody(lua_State *L)
{
    const char          *body;
    size_t              body_len;
    ts_lua_http_ctx     *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    body = luaL_checklstring(L, 1, &body_len);
    TSHttpTxnSetHttpRetBody(http_ctx->txnp, body, 1);
    return 0;
}

