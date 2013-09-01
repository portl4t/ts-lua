
#ifndef _TS_LUA_UTIL_H
#define _TS_LUA_UTIL_H

#include "ts_lua_common.h"

lua_State * ts_lua_new_state();

void ts_lua_set_ctx(lua_State *L, ts_lua_ctx  *ctx);

ts_lua_ctx * ts_lua_get_ctx(lua_State *L);

#endif

