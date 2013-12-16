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


#include <pcre.h>
#include "ts_lua_util.h"

static int ts_lua_regex_compile(lua_State *L);
static int ts_lua_regex_free(lua_State *L);
static int ts_lua_regex_match(lua_State *L);


void
ts_lua_inject_regex_api(lua_State *L)
{
    lua_newtable(L);

    lua_pushcfunction(L, ts_lua_regex_compile);
    lua_setfield(L, -2, "compile");

    lua_pushcfunction(L, ts_lua_regex_free);
    lua_setfield(L, -2, "free");

    lua_pushcfunction(L, ts_lua_regex_match);
    lua_setfield(L, -2, "match");

    lua_setfield(L, -2, "re");
}

static int
ts_lua_regex_compile(lua_State *L)
{
    pcre        *re;
    const char  *pattern;
    const char  *err;
    int         erroffset;
    size_t      len;

    pattern = luaL_checklstring(L, 1, &len);

    if (len == 0) {
        lua_pushnil(L);

    } else {
        re = pcre_compile(pattern, 0, &err, &erroffset, NULL);

        if (re == NULL) {
            fprintf(stderr, "PCRE compilation failed at offset %d: %s/n", erroffset, err);
            lua_pushnil(L);

        } else {
            lua_pushnumber(L, (long)re);
        }
    }

    return 1;
}

static int
ts_lua_regex_free(lua_State *L)
{
    long    val;
    pcre    *re;

    val = luaL_checknumber(L, 1);

    re = (pcre*)val;
    pcre_free(re);

    return 0;
}

static int
ts_lua_regex_match(lua_State *L)
{
    int         rc, i;
    const char  *src;
    const char  *sub;
    size_t      len, sub_len;
    pcre        *re;
    long        val;
    int         ovector[TS_LUA_MAX_OVEC_SIZE];

    val = luaL_checknumber(L, 1);
    re = (pcre*)val;

    src = luaL_checklstring(L, 2, &len);

    rc = pcre_exec(re, NULL, src, len, 0, 0, ovector, TS_LUA_MAX_OVEC_SIZE);

    if (rc < 0) {
        lua_pushnil(L);

    } else {
        lua_newtable(L);

        for (i = 1; i < rc; i++) {
            sub = src + ovector[2*i];
            sub_len = ovector[2*i+1] - ovector[2*i];
            lua_pushlstring(L, sub, sub_len);
            lua_rawseti(L, -2, i-1);
        }
    }

    return 1;
}

