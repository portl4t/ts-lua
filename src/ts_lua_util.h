
#ifndef _TS_LUA_UTIL_H
#define _TS_LUA_UTIL_H

#include "ts_lua_common.h"

lua_State * ts_lua_new_state();

ts_lua_thread_ctx * ts_lua_init_thread(lua_State *l);
void ts_lua_reclaim_unref(ts_lua_thread_ctx *ctx);

void ts_lua_set_http_ctx(lua_State *L, ts_lua_http_ctx  *ctx);
ts_lua_http_ctx * ts_lua_get_http_ctx(lua_State *L);

ts_lua_http_ctx * ts_lua_create_http_ctx(ts_lua_thread_ctx *thread_ctx);
void ts_lua_destroy_http_ctx(ts_lua_http_ctx* http_ctx);

#endif

