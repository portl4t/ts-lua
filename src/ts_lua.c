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


#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "ts_lua_util.h"

#define TS_LUA_MAX_STATE_COUNT              128

static uint64_t ts_lua_http_next_id = 0;
static ts_lua_main_ctx *ts_lua_main_ctx_array = NULL;

TSReturnCode
TSRemapInit(TSRemapInterface *api_info, char *errbuf, int errbuf_size)
{
    int     ret;

    if (!api_info || api_info->size < sizeof(TSRemapInterface)) {
        strncpy(errbuf, "[TSRemapInit] - Incorrect size of TSRemapInterface structure", errbuf_size - 1);
        return TS_ERROR;
    }

    if (ts_lua_main_ctx_array != NULL)
        return TS_SUCCESS;

    ts_lua_main_ctx_array = TSmalloc(sizeof(ts_lua_main_ctx) * TS_LUA_MAX_STATE_COUNT);
    memset(ts_lua_main_ctx_array, 0, sizeof(ts_lua_main_ctx) * TS_LUA_MAX_STATE_COUNT);

    ret = ts_lua_create_vm(ts_lua_main_ctx_array, TS_LUA_MAX_STATE_COUNT);

    if (ret) {
        ee("TSRemapInit failed, ts_lua version: %s", TS_LUA_RELEASE_VERSION);

        ts_lua_destroy_vm(ts_lua_main_ctx_array, TS_LUA_MAX_STATE_COUNT);
        TSfree(ts_lua_main_ctx_array);
        return TS_ERROR;
    }

    return TS_SUCCESS;
}

TSReturnCode
TSRemapNewInstance(int argc, char* argv[], void** ih, char* errbuf, int errbuf_size)
{
    int     fn;
    int     ret;

    if (argc < 3) {
        strncpy(errbuf, "[TSRemapNewInstance] - lua script file or string is required !!", errbuf_size - 1);
        return TS_ERROR;
    }

    fn = 1;

    if (argv[2][0] != '/') {
        fn = 0;

    } else if (strlen(argv[2]) >= TS_LUA_MAX_SCRIPT_FNAME_LENGTH - 16) {
        return TS_ERROR;
    }

    ts_lua_instance_conf *conf = TSmalloc(sizeof(ts_lua_instance_conf));

    if (!conf) {
        fprintf(stderr, "[%s] TSmalloc failed !!\n", __FUNCTION__);
        return TS_ERROR;
    }

    memset(conf, 0, sizeof(ts_lua_instance_conf));

    if (fn) {
        sprintf(conf->script, "%s", argv[2]);

    } else {
        conf->content = argv[2];
    }

    ts_lua_init_instance(conf);

    ret = ts_lua_add_module(conf, ts_lua_main_ctx_array, TS_LUA_MAX_STATE_COUNT, argc-2, &argv[2]);

    if (ret != 0) {
        fprintf(stderr, "[%s] ts_lua_add_module failed\n", __FUNCTION__);
        return TS_ERROR;
    }

    *ih = conf;

    return TS_SUCCESS;
}

void
TSRemapDeleteInstance(void* ih)
{
    ts_lua_del_module((ts_lua_instance_conf*)ih, ts_lua_main_ctx_array, TS_LUA_MAX_STATE_COUNT);
    ts_lua_del_instance(ih);
    TSfree(ih);
    return;
}

TSRemapStatus
TSRemapDoRemap(void* ih, TSHttpTxn rh, TSRemapRequestInfo *rri)
{
    int                         ret;
    uint64_t                    req_id;
    TSCont                      contp;
    lua_State                   *L;
    ts_lua_main_ctx             *main_ctx;
    ts_lua_http_ctx             *http_ctx;
    ts_lua_cont_info            *ci;
    ts_lua_instance_conf        *instance_conf;

    instance_conf = (ts_lua_instance_conf*)ih;
    req_id = __sync_fetch_and_add(&ts_lua_http_next_id, 1);

    main_ctx = &ts_lua_main_ctx_array[req_id%TS_LUA_MAX_STATE_COUNT];

    TSMutexLock(main_ctx->mutexp);

    http_ctx = ts_lua_create_http_ctx(main_ctx, instance_conf);

    http_ctx->txnp = rh;
    http_ctx->client_request_bufp = rri->requestBufp;
    http_ctx->client_request_hdrp = rri->requestHdrp;
    http_ctx->client_request_url = rri->requestUrl;
    http_ctx->rri = rri;

    ci = &http_ctx->cinfo;
    L = ci->routine.lua;

    contp = TSContCreate(ts_lua_http_cont_handler, NULL);
    TSContDataSet(contp, http_ctx);

    ci->contp = contp;
    ci->mutex = TSContMutexGet((TSCont)rh);

    // push do_remap function on the stack, and no async operation should exist here.
    lua_getglobal(L, TS_LUA_FUNCTION_REMAP);

    if (lua_pcall(L, 0, 1, 0) != 0) {
        ee("lua_pcall failed: %s", lua_tostring(L, -1));
        ret = TSREMAP_NO_REMAP;

    } else {
        ret = lua_tointeger(L, -1);
    }

    lua_pop(L, 1);      // pop the result

    if (http_ctx->hooks > 0) {
        TSMutexUnlock(main_ctx->mutexp);
        TSHttpTxnHookAdd(rh, TS_HTTP_TXN_CLOSE_HOOK, contp);

    } else {
        ts_lua_destroy_http_ctx(http_ctx);
        TSMutexUnlock(main_ctx->mutexp);
    }

    return ret;
}
