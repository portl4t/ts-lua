
#include "ts_lua_util.h"

static int ts_lua_client_request_header_get(lua_State *L);
static int ts_lua_client_request_header_set(lua_State *L);
static void ts_lua_inject_client_request_header_api(lua_State *L);


void
ts_lua_inject_client_request_api(lua_State *L)
{
    lua_newtable(L);

    ts_lua_inject_client_request_header_api(L);

    lua_setfield(L, -2, "client_request");
}

static void
ts_lua_inject_client_request_header_api(lua_State *L)
{
    lua_newtable(L);         /* .header */

    lua_createtable(L, 0, 2);       /* metatable for .header */

    lua_pushcfunction(L, ts_lua_client_request_header_get);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, ts_lua_client_request_header_set);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2); 

    lua_setfield(L, -2, "header");
}

static int
ts_lua_client_request_header_get(lua_State *L)
{
    const char  *key;
    const char  *val;
    int         val_len;
    size_t      key_len;

    TSMLoc      field_loc;
    ts_lua_ctx  *ctx;

    ctx = ts_lua_get_ctx(L);

    /*  we skip the first argument that is the table */
    key = luaL_checklstring(L, 2, &key_len);

    if (key && key_len) {

        field_loc = TSMimeHdrFieldFind(ctx->client_request_bufp, ctx->client_request_hdrp, key, key_len);
        if (field_loc) {
            val = TSMimeHdrFieldValueStringGet(ctx->client_request_bufp, ctx->client_request_hdrp, field_loc, -1, &val_len);
            lua_pushlstring(L, val, val_len);
            TSHandleMLocRelease(ctx->client_request_bufp, ctx->client_request_hdrp, field_loc);

        } else {
            lua_pushnil(L);
        }

    } else {
        lua_pushnil(L);
    }

    return 1;
}

static int
ts_lua_client_request_header_set(lua_State *L)
{
    return 0;
}

