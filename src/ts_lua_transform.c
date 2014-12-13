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


static int ts_lua_transform_handler(TSCont contp, ts_lua_http_transform_ctx *transform_ctx);


int
ts_lua_transform_entry(TSCont contp, TSEvent event, void *edata)
{
    TSVIO       input_vio;

    ts_lua_http_transform_ctx *transform_ctx = (ts_lua_http_transform_ctx*)TSContDataGet(contp);

    if (TSVConnClosedGet(contp)) {
        TSContDestroy(contp);
        ts_lua_destroy_http_transform_ctx(transform_ctx);
        return 0;
    }

    switch (event) {

        case TS_EVENT_ERROR:
            input_vio = TSVConnWriteVIOGet(contp);
            TSContCall(TSVIOContGet(input_vio), TS_EVENT_ERROR, input_vio);
            break;

        case TS_EVENT_VCONN_WRITE_COMPLETE:
            TSVConnShutdown(TSTransformOutputVConnGet(contp), 0, 1);
            break;

        case TS_EVENT_VCONN_WRITE_READY:
        default:
            ts_lua_transform_handler(contp, transform_ctx);
            break;
    }

    return 0;
}

static int
ts_lua_transform_handler(TSCont contp, ts_lua_http_transform_ctx *transform_ctx)
{
    TSVConn             output_conn;
    TSVIO               input_vio;
    TSIOBufferReader    input_reader;
    TSIOBufferBlock     blk;
    int64_t             toread, towrite, blk_len, upstream_done, input_avail, n;
    const char          *start;
    const char          *res;
    size_t              res_len;
    int                 ret, eos, write_down;

    lua_State           *L;
    TSMutex             mtxp;

    L = transform_ctx->hctx->lua;
    mtxp = transform_ctx->hctx->mctx->mutexp;

    output_conn = TSTransformOutputVConnGet(contp);
    input_vio = TSVConnWriteVIOGet(contp);
    input_reader = TSVIOReaderGet(input_vio);

    if (!transform_ctx->output.buffer) {
        transform_ctx->output.buffer = TSIOBufferCreate();
        transform_ctx->output.reader = TSIOBufferReaderAlloc(transform_ctx->output.buffer);
        transform_ctx->upstream_bytes = TSVIONBytesGet(input_vio);
        transform_ctx->downstream_bytes = INT64_MAX;
    }

    if (!TSVIOBufferGet(input_vio)) {
        if (transform_ctx->output.vio) {
            TSVIONBytesSet(transform_ctx->output.vio, transform_ctx->total);
            TSVIOReenable(transform_ctx->output.vio);
        }

        return 1;
    }

    input_avail = TSIOBufferReaderAvail(input_reader);
    upstream_done = TSVIONDoneGet(input_vio);
    toread = TSVIONTodoGet(input_vio);

    if (toread == input_avail) {        // upstream finished
        eos = 1;

    } else {
        eos = 0;
    }

    write_down = 0;
    towrite = input_avail;

    blk = TSIOBufferReaderStart(input_reader);

    TSMutexLock(mtxp);

    while (blk && towrite > 0) {
        start = TSIOBufferBlockReadStart(blk, input_reader, &blk_len);
        if (start == NULL || blk_len == 0) {
            blk = TSIOBufferBlockNext(blk);
            continue;
        }

        lua_pushlightuserdata(L, transform_ctx);
        lua_rawget(L, LUA_GLOBALSINDEX);                    /* push function */

        if (towrite > blk_len) {
            lua_pushlstring(L, start, (size_t)blk_len);
            towrite -= blk_len;

        } else {
            lua_pushlstring(L, start, (size_t)towrite);
            towrite = 0;
        }

        if (!towrite && eos) {
            lua_pushinteger(L, 1);                          /* second param, finished */ 

        } else {
            lua_pushinteger(L, 0);                          /* second param, not finish */ 
        }

        if (lua_pcall(L, 2, 2, 0)) {
            fprintf(stderr, "lua_pcall failed: %s\n", lua_tostring(L, -1));
        }

        ret = lua_tointeger(L, -1);                         /* 0 is not finished, 1 is finished */
        res = lua_tolstring(L, -2, &res_len);

        if (res && res_len > 0) {
            if (!transform_ctx->output.vio) {
                n = transform_ctx->downstream_bytes;
                if (ret)
                    n = res_len;

                transform_ctx->output.vio = TSVConnWrite(output_conn, contp, transform_ctx->output.reader, n);
            }

            TSIOBufferWrite(transform_ctx->output.buffer, res, res_len);
            transform_ctx->total += res_len;
            write_down = 1;
        }

        lua_pop(L, 2);

        if (ret || (eos && !towrite)) {            // EOS
            eos = 1;
            break;
        }

        blk = TSIOBufferBlockNext(blk);
    }

    TSMutexUnlock(mtxp);

    TSIOBufferReaderConsume(input_reader, input_avail);
    TSVIONDoneSet(input_vio, upstream_done + input_avail);

    if (eos && !transform_ctx->output.vio)
        transform_ctx->output.vio = TSVConnWrite(output_conn, contp, transform_ctx->output.reader, 0);

    if (write_down || eos)
        TSVIOReenable(transform_ctx->output.vio);

    if (toread > input_avail) {     // upstream not finished.
        if (eos) {
            TSVIONBytesSet(transform_ctx->output.vio, transform_ctx->total);
            TSContCall(TSVIOContGet(input_vio), TS_EVENT_VCONN_EOS, input_vio);

        } else {
            TSContCall(TSVIOContGet(input_vio), TS_EVENT_VCONN_WRITE_READY, input_vio);
        }

    } else {                        // upstream is finished.
        TSVIONBytesSet(transform_ctx->output.vio, transform_ctx->total);
        TSContCall(TSVIOContGet(input_vio), TS_EVENT_VCONN_WRITE_COMPLETE, input_vio);
    }

    return 1;
}

