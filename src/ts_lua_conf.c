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


#include "ts_lua_util.h"


static int ts_lua_conf_var_get(lua_State *L);
static int ts_lua_conf_var_set(lua_State *L);


void
ts_lua_inject_conf_api(lua_State *L)
{
    lua_newtable(L);         /* .conf */

    lua_createtable(L, 0, 2);       /* metatable for conf */

    lua_pushcfunction(L, ts_lua_conf_var_get);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, ts_lua_conf_var_set);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2); 

    lua_setfield(L, -2, "conf");
}

static int
ts_lua_conf_var_get(lua_State *L)
{
    int     n;
    ts_lua_instance_conf *conf;

    n = luaL_checkint(L, 2);

    if (n < 0 || n >= TS_LUA_MAX_CONFIG_VARS_COUNT) {
        lua_pushnil(L);
        return 1;
    }

    conf = ts_lua_get_instance_conf(L);

    if (conf && conf->conf_vars[n]) {
        lua_pushnumber(L, (long)conf->conf_vars[n]);

    } else {
        lua_pushnil(L);
    }

    return 1;
}

static int
ts_lua_conf_var_set(lua_State *L)
{
    int     n;
    long    val;
    int     type;

    ts_lua_instance_conf *conf;

    n = luaL_checkint(L, 2);
    type = lua_type(L, 3);

    if (type == LUA_TNUMBER) {
        val = luaL_checknumber(L, 3);

    } else {
        val = 0;
    }

    if (n < 0 || n >= TS_LUA_MAX_CONFIG_VARS_COUNT)
        return 0;

    conf = ts_lua_get_instance_conf(L);

    if (conf) {
        conf->conf_vars[n] = (void*)val;
    }

    return 0;
}

