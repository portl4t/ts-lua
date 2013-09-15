
#ifndef _TS_LUA_COMMON_H
#define _TS_LUA_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <ts/ts.h>
#include <ts/experimental.h>
#include <ts/remap.h>

#include "ts_lua_atomic.h"

#define TS_LUA_FUNCTION_REMAP                   "do_remap"
#define TS_LUA_FUNCTION_CACHE_LOOKUP_COMPLETE   "do_cache_lookup_complete"
#define TS_LUA_FUNCTION_SEND_REQUEST            "do_send_request"
#define TS_LUA_FUNCTION_READ_RESPONSE           "do_read_response"
#define TS_LUA_FUNCTION_SEND_RESPONSE           "do_send_response"

#define TS_LUA_MAX_SCRIPT_FNAME_LENGTH      1024
#define TS_LUA_MAX_URL_LENGTH               2048


typedef struct {
    char    script[TS_LUA_MAX_SCRIPT_FNAME_LENGTH];
} ts_lua_instance_conf;


typedef struct {
    lua_State   *lua;
    TSMutex     mutexp;
    int         gref;
} ts_lua_main_ctx;


typedef struct {
    lua_State   *lua;
    TSHttpTxn   txnp;
    TSCont      main_contp;

    TSMBuffer   client_request_bufp;
    TSMLoc      client_request_hdrp;
    TSMLoc      client_request_url;

    TSMBuffer   client_response_bufp;
    TSMLoc      client_response_hdrp;

    ts_lua_main_ctx   *mctx;

    int         ref;

} ts_lua_http_ctx;

#endif

