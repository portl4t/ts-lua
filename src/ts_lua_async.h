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

#ifndef _TS_LUA_ASYNC_H
#define _TS_LUA_ASYNC_H

#include <stdio.h>
#include <ts/ts.h>


struct async_item;
typedef int (*async_clean)(struct async_item *item);

typedef struct async_item {
    struct async_item   *next;

    TSCont              contp;
    TSCont              pcontp;
    void                *data;
    async_clean         cleanup;

    int                 deleted:1;
} ts_lua_async_item;

inline ts_lua_async_item * ts_lua_async_create_item(TSCont parent, async_clean func, void *d, TSCont cont);
inline void ts_lua_async_push_item(ts_lua_async_item **head, ts_lua_async_item *node);
inline void ts_lua_async_destroy_item(ts_lua_async_item *node);

void ts_lua_async_destroy_chain(ts_lua_async_item **head);

#endif
