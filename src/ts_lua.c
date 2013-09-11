

#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "ts_lua_util.h"

#define TS_LUA_MAX_STATE_COUNT                  200

static volatile int32_t ts_lua_http_next_id = 0;

ts_lua_main_ctx         *ts_lua_main_ctx_array;


static int
ts_lua_cont_handler(TSCont contp, TSEvent event, void *edata)
{
    int                 ret;
    TSHttpTxn           txnp = (TSHttpTxn)edata;
    lua_State           *l;
    ts_lua_http_ctx     *http_ctx;
    ts_lua_main_ctx     *main_ctx;

    http_ctx = (ts_lua_http_ctx*)TSContDataGet(contp);
    main_ctx = http_ctx->mctx;

    l = http_ctx->lua;

    TSMutexLock(main_ctx->mutexp);

    switch (event) {
        case TS_EVENT_HTTP_SEND_RESPONSE_HDR:

            lua_getglobal(l, TS_LUA_FUNCTION_SEND_RESPONSE);
            if (lua_type(l, -1) == LUA_TFUNCTION) {
                ret = lua_pcall(l, 0, 1, 0);
                if (ret) {
                    fprintf(stderr, "lua_pcall failed: %s\n", lua_tostring(l, -1));
                }
                ret = lua_tointeger(l, -1);
                lua_pop(l, 1);
            }

            break;

        case TS_EVENT_HTTP_TXN_CLOSE:
            ts_lua_destroy_http_ctx(http_ctx);
            TSContDestroy(contp);
            break;

        default:
            break;
    }

    TSMutexUnlock(main_ctx->mutexp);
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
    return 0;
}

TSReturnCode
TSRemapInit(TSRemapInterface *api_info, char *errbuf, int errbuf_size)
{
    int     ret;

    if (!api_info || api_info->size < sizeof(TSRemapInterface))
        return TS_ERROR;

    ts_lua_main_ctx_array = TSmalloc(sizeof(ts_lua_main_ctx) * TS_LUA_MAX_STATE_COUNT);
    memset(ts_lua_main_ctx_array, 0, sizeof(ts_lua_main_ctx) * TS_LUA_MAX_STATE_COUNT);

    ret = ts_lua_create_vm(ts_lua_main_ctx_array, TS_LUA_MAX_STATE_COUNT);

    if (ret) {
        ts_lua_destroy_vm(ts_lua_main_ctx_array, TS_LUA_MAX_STATE_COUNT);
        TSfree(ts_lua_main_ctx_array);
        return TS_ERROR;
    }

    return TS_SUCCESS;
}

TSReturnCode
TSRemapNewInstance(int argc, char* argv[], void** ih, char* errbuf, int errbuf_size)
{
    int     ret;

    if (argc < 3) {
        TSError("[%s] lua script file required !!", __FUNCTION__);
        return TS_ERROR;
    }

    if (strlen(argv[2]) >= TS_LUA_MAX_SCRIPT_FNAME_LENGTH - 16)
        return TS_ERROR;

    ts_lua_instance_conf *conf = TSmalloc(sizeof(ts_lua_instance_conf));
    if (!conf) {
        TSError("[%s] TSmalloc failed !!", __FUNCTION__);
        return TS_ERROR;
    }

    sprintf(conf->script, "%s", argv[2]);

    ret = ts_lua_add_module(conf, ts_lua_main_ctx_array, TS_LUA_MAX_STATE_COUNT);

    *ih = conf;

    return TS_SUCCESS;
}

void
TSRemapDeleteInstance(void* ih)
{
    TSfree(ih);
    return;
}

TSRemapStatus
TSRemapDoRemap(void* ih, TSHttpTxn rh, TSRemapRequestInfo *rri)
{
    int                 ret;
    int64_t             req_id;

    TSCont              contp;
    lua_State           *l;

    ts_lua_main_ctx     *main_ctx;
    ts_lua_http_ctx     *http_ctx;

    ts_lua_instance_conf     *instance_conf;

    instance_conf = (ts_lua_instance_conf*)ih;
    req_id = (int64_t) ts_lua_atomic_increment((&ts_lua_http_next_id), 1);

    main_ctx = &ts_lua_main_ctx_array[req_id%TS_LUA_MAX_STATE_COUNT];

    TSMutexLock(main_ctx->mutexp);

    http_ctx = ts_lua_create_http_ctx(main_ctx, instance_conf);

    http_ctx->txnp = rh;
    http_ctx->client_request_bufp = rri->requestBufp;
    http_ctx->client_request_hdrp = rri->requestHdrp;
    http_ctx->client_request_url = rri->requestUrl;

    l = http_ctx->lua;

    lua_getglobal(l, TS_LUA_FUNCTION_REMAP);
    if (lua_type(l, -1) != LUA_TFUNCTION) {
        fprintf(stderr, "fk here\n");
    }

    ret = lua_pcall(l, 0, 1, 0);
    if (ret) {
        fprintf(stderr, "lua_pcall failed: %s\n", lua_tostring(l, -1));
    }

    ret = lua_tointeger(l, -1);
    lua_pop(l, 1);

    switch (ret) {
        case 1:         // add hook
            contp = TSContCreate(ts_lua_cont_handler, NULL);
            TSContDataSet(contp, http_ctx);

            TSHttpTxnHookAdd(rh, TS_HTTP_SEND_RESPONSE_HDR_HOOK, contp);
            TSHttpTxnHookAdd(rh, TS_HTTP_TXN_CLOSE_HOOK, contp);
            break;

        case 0:
        default:
            ts_lua_destroy_http_ctx(http_ctx);
            break;
    }

    TSMutexUnlock(main_ctx->mutexp);
    return TSREMAP_NO_REMAP;
}

