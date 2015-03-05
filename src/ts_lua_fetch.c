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

#include<sys/socket.h>
#include<netinet/in.h>

#include "ts_lua_util.h"
#include "ts_lua_io.h"
#include "ts_lua_fetch.h"


static int ts_lua_fetch(lua_State *L);
static int ts_lua_fetch_handler(TSCont contp, TSEvent event, void *edata);
static int ts_lua_fetch_cleanup(ts_lua_async_item *ai);


void
ts_lua_inject_fetch_api(lua_State *L)
{
    /* ts.fetch() */
    lua_pushcfunction(L, ts_lua_fetch);
    lua_setfield(L, -2, "fetch");
}


static int
ts_lua_fetch(lua_State *L)
{
    int                     n;
    const char              *url, *method, *key, *value;
    size_t                  url_len, method_len, key_len, value_len;
    TSCont                  contp;
    ts_lua_fetch_info       *fi;
    ts_lua_async_item       *ai;
    ts_lua_cont_info        *ci;

    struct sockaddr_in      clientaddr;

    memset(&clientaddr, 0, sizeof(clientaddr));
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(6666);

    ci = ts_lua_get_cont_info(L);
    if (ci == NULL)
        return 0;

    n = lua_gettop(L);
    if (n < 2)
        return 0;

    /* url */
    url = luaL_checklstring(L, 1, &url_len);

    /* method */
    lua_pushlstring(L, "method", sizeof("method") - 1);
    lua_gettable(L, 2);

    if (lua_isstring(L, -1)) {
        method = luaL_checklstring(L, -1, &method_len);

    } else {
        method = "GET";
        method_len = sizeof("GET") - 1;
    }

    lua_pop(L, 1);

    // create fetcher
    fi = (ts_lua_fetch_info*)TSmalloc(sizeof(ts_lua_fetch_info));
    fi->buffer = TSIOBufferCreate();
    fi->reader = TSIOBufferReaderAlloc(fi->buffer);

    contp = TSContCreate(ts_lua_fetch_handler, ci->mutex);
    fi->fch = TSFetchCreate(contp, method, url, "HTTP/1.1", (struct sockaddr*)&clientaddr, TS_FETCH_FLAGS_DECHUNK);

    /* header */
    lua_pushlstring(L, "header", sizeof("header") - 1);
    lua_gettable(L, 2);

    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_pushvalue(L, -2);

        key = luaL_checklstring(L, -1, &key_len);
        value = luaL_checklstring(L, -2, &value_len);

        TSFetchHeaderAdd(fi->fch, key, key_len, value, value_len);

        lua_pop(L, 2);
    }

    ai = ts_lua_async_create_item(contp, ts_lua_fetch_cleanup, (void*)fi, ci);
    TSContDataSet(contp, ai);

    TSFetchLaunch(fi->fch);

    return lua_yield(L, 0);
}

static int
ts_lua_fetch_handler(TSCont contp, TSEvent event, void *edata)
{
    const char          *name, *value;
    int                 name_len, value_len;
    char                *from, *dst;
    int64_t             n, wavail, ravail;
    TSIOBufferBlock     blk;
    TSMBuffer           bufp;
    TSMLoc              hdrp;
    TSMLoc              field_loc, next_field_loc;
    TSHttpStatus        status;
    lua_State           *L;
    TSMutex             lm;

    ts_lua_async_item   *ai;
    ts_lua_cont_info    *ci;
    ts_lua_fetch_info   *fi;

    ai = TSContDataGet(contp);
    fi = (ts_lua_fetch_info*)ai->data;
    ci = ai->cinfo;

    L = ai->cinfo->routine.lua;
    lm = ai->cinfo->routine.mctx->mutexp;

    switch ((int)event) {

        case TS_FETCH_EVENT_EXT_HEAD_READY:
            break;

        case TS_FETCH_EVENT_EXT_HEAD_DONE:
            break;

        case TS_FETCH_EVENT_EXT_BODY_READY:
        case TS_FETCH_EVENT_EXT_BODY_DONE:

            do {
                blk = TSIOBufferStart(fi->buffer);
                from = TSIOBufferBlockWriteStart(blk, &wavail);
                n = TSFetchReadData(fi->fch, from, wavail);
                TSIOBufferProduce(fi->buffer, n);
            } while (n == wavail);

            if (event == TS_FETCH_EVENT_EXT_BODY_DONE) {        // fetch over
                bufp = TSFetchRespHdrMBufGet(fi->fch);
                hdrp = TSFetchRespHdrMLocGet(fi->fch);

                TSMutexLock(lm);

                lua_newtable(L);

                // status code
                status = TSHttpHdrStatusGet(bufp, hdrp);
                lua_pushlstring(L, "status", sizeof("status") - 1);
                lua_pushnumber(L, status);
                lua_rawset(L, -3);

                // header
                lua_pushlstring(L, "header", sizeof("header") - 1);
                lua_newtable(L);

                field_loc = TSMimeHdrFieldGet(bufp, hdrp, 0);
                while (field_loc) {
                    name = TSMimeHdrFieldNameGet(bufp, hdrp, field_loc, &name_len);
                    value = TSMimeHdrFieldValueStringGet(bufp, hdrp, field_loc, -1, &value_len);

                    lua_pushlstring(L, name, name_len);
                    lua_pushlstring(L, value, value_len);
                    lua_rawset(L, -3);

                    next_field_loc = TSMimeHdrFieldNext(bufp, hdrp, field_loc);
                    TSHandleMLocRelease(bufp, hdrp, field_loc);
                    field_loc = next_field_loc;
                }
                lua_rawset(L, -3);

                // body
                ravail = TSIOBufferReaderAvail(fi->reader);
                if (ravail > 0) {
                    lua_pushlstring(L, "body", sizeof("body") - 1);

                    dst = (char*)TSmalloc(ravail);
                    IOBufferReaderCopy(fi->reader, dst, ravail);
                    lua_pushlstring(L, (char*)dst, ravail);

                    lua_rawset(L, -3);
                    TSfree(dst);
                }

                TSContCall(ci->contp, TS_EVENT_COROUTINE_CONT, (void*)1);     // 1 result: res table
                TSMutexUnlock(lm);
            }

            break;

        default:
            TSContCall(ci->contp, TS_EVENT_COROUTINE_CONT, 0);         // error exist
            break;
    }

    return 0;
}

static int
ts_lua_fetch_cleanup(ts_lua_async_item *ai)
{
    ts_lua_fetch_info       *fi;

    if (ai->data) {
        fi = (ts_lua_fetch_info*)ai->data;

        if (fi->reader) {
            TSIOBufferReaderFree(fi->reader);
        }

        if (fi->buffer) {
            TSIOBufferDestroy(fi->buffer);
        }

        if (fi->fch) {
            TSFetchDestroy(fi->fch);
        }

        TSfree(fi);

        ai->data = NULL;
    }

    TSContDestroy(ai->contp);
    return 0;
}

