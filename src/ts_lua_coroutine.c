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

#include "ts_lua_coroutine.h"

static inline void ts_lua_async_push_item(ts_lua_async_item **head, ts_lua_async_item *node);

inline void
ts_lua_coroutine_assign(ts_lua_coroutine *dst, ts_lua_coroutine *src)
{
    dst->mctx = src->mctx;
    dst->lua = src->lua;
    dst->ref = src->ref;
}

inline ts_lua_async_item *
ts_lua_async_create_item(TSCont cont, async_clean func, void *d, ts_lua_cont_info *ci)
{
    ts_lua_async_item   *ai;

    ai = (ts_lua_async_item*)TSmalloc(sizeof(ts_lua_async_item));
    if (ai == NULL)
        return NULL;

    ai->cinfo = ci;

    ai->cleanup = func;
    ai->data = d;
    ai->contp = cont;
    ai->deleted = 0;

    ts_lua_async_push_item(&ci->async_chain, ai);

    return ai;
}

static inline void
ts_lua_async_push_item(ts_lua_async_item **head, ts_lua_async_item *node)
{
    node->next = *head;
    *head = node;
}

inline void
ts_lua_async_destroy_item(ts_lua_async_item *node)
{
    if (node->cleanup && !node->deleted) {
        node->cleanup(node);
    }

    TSfree(node);
}

void
ts_lua_async_destroy_chain(ts_lua_async_item **head)
{
    ts_lua_async_item   *node, *next;

    node = *head;

    while (node) {
        next = node->next;
        ts_lua_async_destroy_item(node);

        node = next;
    }
}
