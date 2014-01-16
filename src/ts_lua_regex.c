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
#include "ts_lua_hash_table.h"

static int ts_lua_regex_match(lua_State *L);

static int free_pcre_value(Tcl_HashTable *t, Tcl_HashEntry *entry);

void
ts_lua_inject_regex_api(lua_State *L)
{
    lua_newtable(L);

    lua_pushcfunction(L, ts_lua_regex_match);
    lua_setfield(L, -2, "match");

    lua_setfield(L, -2, "re");
}


static int
ts_lua_regex_match(lua_State *L)
{
    int         rc, i, n, flags;
    const char  *src;
    const char  *sub;
    const char  *pattern;
    const char  *opt;
    const char  *key;
    size_t      src_len, sub_len, pattern_len, opt_len;
    pcre        *re;
    int         ovector[TS_LUA_MAX_OVEC_SIZE];
    char        tmp[1024];
    void        *pcre_lookup;
    char        c;
    const char  *err;
    int         erroffset;
    int         isNew, created;
    Tcl_HashEntry *hPtr;
    ts_lua_http_ctx  *http_ctx;
    ts_lua_instance_conf *conf;

    http_ctx = ts_lua_get_http_ctx(L);
    conf = http_ctx->instance_conf;

    n = lua_gettop(L);
    src = luaL_checklstring(L, 1, &src_len);
    pattern = luaL_checklstring(L, 2, &pattern_len);

    if (n == 3) {
        opt = luaL_checklstring(L, 3, &opt_len);
        snprintf(tmp, sizeof(tmp)-8, "%s%s", pattern, opt);
        key = tmp;

    } else {
        opt = NULL;
        key = pattern;
    }

    isNew = 0;
    created = 0;

    if (ts_lua_hash_table_lookup(&conf->regex_map.t, key, &pcre_lookup)) {
        re = pcre_lookup;

    } else {

        i = flags = 0;

        if (opt && opt_len > 0) {

            while  (i < opt_len) {
                c = *(opt + i);

                switch (c) {
                    case 'i':
                        flags |= PCRE_CASELESS;
                        break;

                    case 'a':
                        flags |= PCRE_ANCHORED;
                        break;

                    case 'm':
                        flags |= PCRE_MULTILINE;
                        break;

                    case 'u':
                        flags |= PCRE_UNGREEDY;
                        break;

                    case 's':
                        flags |= PCRE_DOTALL;
                        break;

                    case 'x':
                        flags |= PCRE_EXTENDED;
                        break;

                    case 'd':
                        flags |= PCRE_DOLLAR_ENDONLY;
                        break;

                    default:
                        break;
                }

                i++;
            }
        }

        re = pcre_compile(pattern, flags, &err, &erroffset, NULL);
        created = 1;

        if (re == NULL) {
            return luaL_error(L, "PCRE compilation failed at offset %d: %s/n", erroffset, err);

        } else if (conf->regex_map.t.numEntries < TS_LUA_MAX_RESIDENT_PCRE) {

            TSMutexLock(conf->regex_map.mutexp);
            hPtr = Tcl_CreateHashEntry(&(conf->regex_map.t), key, &isNew);
            if (isNew) {
                Tcl_SetHashValue(hPtr, re);
            }

            TSMutexUnlock(conf->regex_map.mutexp);
        }
    }

    rc = pcre_exec(re, NULL, src, src_len, 0, 0, ovector, TS_LUA_MAX_OVEC_SIZE);

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

    if (created && !isNew) {
        pcre_free(re);
    }

    return 1;
}

int
ts_lua_init_regex_map(ts_lua_hash_map *regex_map)
{
    Tcl_InitHashTable(&(regex_map->t), TCL_STRING_KEYS);
    regex_map->mutexp = TSMutexCreate();
    return 0;
}

int
ts_lua_del_regex_map(ts_lua_hash_map *regex_map)
{
    ts_lua_hash_table_iterate(&(regex_map->t), free_pcre_value);
    Tcl_DeleteHashTable(&(regex_map->t));
    return 0;
}

static int
free_pcre_value(Tcl_HashTable *t, Tcl_HashEntry *entry)
{
    ClientData  val;

    val = ts_lua_hash_table_entry_value(t, entry);

    if (val != NULL) {
        pcre_free((pcre*)val);
    }

    return 0;
}

