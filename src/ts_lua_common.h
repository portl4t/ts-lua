
#ifndef _TS_LUA_COMMON_H
#define _TS_LUA_COMMON_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <ts/ts.h>
#include <ts/experimental.h>
#include <ts/remap.h>

typedef struct {
    lua_State   *lua;
    TSHttpTxn   txnp;

    TSMBuffer   client_request_bufp;
    TSMLoc      client_request_hdrp;

    TSMBuffer   client_response_bufp;
    TSMLoc      client_response_hdrp;
} ts_lua_ctx;

#endif

