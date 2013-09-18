
#ifndef _TS_LUA_CONTEXT_H
#define _TS_LUA_CONTEXT_H

#include "ts_lua_common.h"

void ts_lua_inject_context_api(lua_State *L);
void ts_lua_create_context_table(lua_State *L);

#endif

