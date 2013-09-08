
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

#define     TS_LUA_MAX_URL_LENGTH   2048

typedef struct {
    int     ref;
    void    *next;
} ts_lua_ref;


typedef struct {
    lua_State   *lua;
    time_t      reclaim_time;
    ts_lua_atomiclist reclaim_list;
} ts_lua_thread_ctx;


typedef struct {
    lua_State   *lua;
    TSHttpTxn   txnp;

    TSMBuffer   client_request_bufp;
    TSMLoc      client_request_hdrp;
    TSMLoc      client_request_url;

    TSMBuffer   client_response_bufp;
    TSMLoc      client_response_hdrp;

    ts_lua_thread_ctx   *th_ctx;

    int         ref;

} ts_lua_http_ctx;

#endif

