/*
  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/


#include "ts_lua_constant.h"


typedef enum {
    TS_LUA_INT64_MAX    = INT64_MAX,
    TS_LUA_INT64_MIN    = INT64_MIN,
    TS_LUA_INT32_MAX    = INT32_MAX,
    TS_LUA_INT32_MIN    = INT32_MIN,
    TS_LUA_UINT32_MAX   = UINT32_MAX,
} TSLuaConstantNumbers;


ts_lua_var64_item ts_lua_number_vars[] = {
    TS_LUA_MAKE_VAR_ITEM(TS_LUA_INT64_MAX),
    TS_LUA_MAKE_VAR_ITEM(TS_LUA_INT64_MIN),
    TS_LUA_MAKE_VAR_ITEM(TS_LUA_INT32_MAX),
    TS_LUA_MAKE_VAR_ITEM(TS_LUA_INT32_MIN),
    TS_LUA_MAKE_VAR_ITEM(TS_LUA_UINT32_MAX)
};


static void ts_lua_inject_number_variables(lua_State *L);


void
ts_lua_inject_constant_api(lua_State *L)
{
    ts_lua_inject_number_variables(L);
}

static void
ts_lua_inject_number_variables(lua_State *L)
{
    int     i;

    for (i = 0; i < sizeof(ts_lua_number_vars) / sizeof(ts_lua_var64_item); i++) {
        lua_pushinteger(L, ts_lua_number_vars[i].nvar);
        lua_setglobal(L, ts_lua_number_vars[i].svar);
    }
}

