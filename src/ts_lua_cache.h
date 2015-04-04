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

#ifndef _TS_LUA_CACHE_H
#define _TS_LUA_CACHE_H

#include "ts_lua_common.h"

struct cache_info;
typedef int (*cache_fn)(ts_lua_cont_info *ci, struct cache_info *info, TSEvent event, void *data);

typedef struct cache_info {
    TSCont              contp;              // should be destroyed only in cleanup
    TSCacheKey          cache_key;
    TSVConn             cache_vc;
    TSAction            cache_action;
    ts_lua_io_handle    ioh;
    ts_lua_io_handle    reserved;

    int64_t             already;
    int64_t             need;
    int64_t             sz;
    int64_t             seek;
    cache_fn            current_handler;
    int                 optype;

    int                 hit:1;
    int                 eof:1;
    int                 err:1;
    int                 wait:1;
} ts_lua_cache_info;

void ts_lua_inject_cache_api(lua_State *L);

#endif
