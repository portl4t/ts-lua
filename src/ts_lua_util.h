
#ifndef _TS_LUA_UTIL_H
#define _TS_LUA_UTIL_H

#include "ts_lua_common.h"

int ts_lua_create_vm(ts_lua_main_ctx *arr, int n);
void ts_lua_destroy_vm(ts_lua_main_ctx *arr, int n);

int ts_lua_add_module(ts_lua_instance_conf *conf, ts_lua_main_ctx *arr, int n, int argc, char *argv[]);

void ts_lua_set_http_ctx(lua_State *L, ts_lua_http_ctx  *ctx);
ts_lua_http_ctx * ts_lua_get_http_ctx(lua_State *L);

ts_lua_http_ctx * ts_lua_create_http_ctx(ts_lua_main_ctx *mctx, ts_lua_instance_conf *conf);
void ts_lua_destroy_http_ctx(ts_lua_http_ctx* http_ctx);

int ts_lua_http_cont_handler(TSCont contp, TSEvent event, void *edata);

#endif

