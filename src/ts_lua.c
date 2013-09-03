

#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "ts_lua_util.h"

#define TS_LUA_MAX_SCRIPT_FNAME_LENGTH          1024
#define TS_LUA_FUNCTION_REMAP                   "do_remap"
#define TS_LUA_FUNCTION_CACHE_LOOKUP_COMPLETE   "do_cache_lookup_complete"
#define TS_LUA_FUNCTION_SEND_REQUEST            "do_send_request"
#define TS_LUA_FUNCTION_READ_RESPONSE           "do_read_response"
#define TS_LUA_FUNCTION_SEND_RESPONSE           "do_send_response"


typedef struct {
    pthread_key_t lua_state_key;
    char script_file[TS_LUA_MAX_SCRIPT_FNAME_LENGTH];
} ts_lua_conf;


ts_lua_thread_ctx *
ts_lua_get_thread_ctx(ts_lua_conf *lua_conf)
{
    int                 ret;
    ts_lua_thread_ctx   *thread_ctx;
    lua_State           *l;

    thread_ctx = (ts_lua_thread_ctx*)pthread_getspecific(lua_conf->lua_state_key);

    if (thread_ctx == NULL) {
        l = ts_lua_new_state();

        ret = luaL_loadfile(l, lua_conf->script_file);
        if (ret) {
            fprintf(stderr, "luaL_loadfile failed: %s\n", lua_tostring(l, -1));
        }

        ret = lua_pcall(l, 0, 0, 0);
        if (ret) {
            fprintf(stderr, "lua_pcall failed: %s\n", lua_tostring(l, -1));
        }

        thread_ctx = ts_lua_init_thread(l);
        pthread_setspecific(lua_conf->lua_state_key, thread_ctx);
    }

    return thread_ctx;
}

static int
ts_lua_cont_handler(TSCont contp, TSEvent event, void *edata)
{
    int                 ret;
    TSHttpTxn           txnp = (TSHttpTxn)edata;
    lua_State           *l;
    ts_lua_http_ctx     *http_ctx;

    http_ctx = (ts_lua_http_ctx*)TSContDataGet(contp);
    l = http_ctx->lua;

    switch (event) {
        case TS_EVENT_HTTP_SEND_RESPONSE_HDR:

            lua_getglobal(l, TS_LUA_FUNCTION_SEND_RESPONSE);
            if (lua_type(l, -1) == LUA_TFUNCTION) {
                lua_pcall(l, 0, 1, 0);
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

    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
    return 0;
}

TSReturnCode
TSRemapInit(TSRemapInterface *api_info, char *errbuf, int errbuf_size)
{
    if (!api_info || api_info->size < sizeof(TSRemapInterface))
        return TS_ERROR;

    return TS_SUCCESS;
}

TSReturnCode
TSRemapNewInstance(int argc, char* argv[], void** ih, char* errbuf, int errbuf_size)
{
    if (argc < 3) {
        TSError("[%s] lua script file required !!", __FUNCTION__);
        return TS_ERROR;
    }

    ts_lua_conf *conf = TSmalloc(sizeof(ts_lua_conf));
    if (!conf) {
        TSError("[%s] TSmalloc failed !!", __FUNCTION__);
        return TS_ERROR;
    }

    int status = pthread_key_create(&conf->lua_state_key, NULL);
    if (status) {
        TSError("[%s] pthread_key_create Error: %s", __FUNCTION__, strerror(errno));
        TSfree(conf);
        return TS_ERROR;
    }

    sprintf(conf->script_file, "%s", argv[2]);

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
    int             ret, base;

    TSCont          contp;
    lua_State       *L;
    lua_State       *l;
    ts_lua_conf     *lua_conf;

    ts_lua_thread_ctx    *thread_ctx;
    ts_lua_http_ctx      *http_ctx;

    lua_conf = (ts_lua_conf*)ih;

    thread_ctx = ts_lua_get_thread_ctx(lua_conf);
    L = thread_ctx->lua;

    http_ctx = ts_lua_create_http_ctx(thread_ctx);

    http_ctx->txnp = rh;
    http_ctx->client_request_bufp = rri->requestBufp;
    http_ctx->client_request_hdrp = rri->requestHdrp;

    l = http_ctx->lua;

    lua_getglobal(l, TS_LUA_FUNCTION_REMAP);
    if (lua_type(l, -1) != LUA_TFUNCTION) {
        return TSREMAP_NO_REMAP;
    }

    lua_pcall(l, 0, 1, 0);

    ret = lua_tointeger(l, -1);
    lua_pop(l, 1);

    switch (ret) {
        case 1:         // hook 一下
            contp = TSContCreate(ts_lua_cont_handler, NULL);
            TSContDataSet(contp, http_ctx);

            TSHttpTxnHookAdd(rh, TS_HTTP_SEND_RESPONSE_HDR_HOOK, contp);
            TSHttpTxnHookAdd(rh, TS_HTTP_TXN_CLOSE_HOOK, contp);
            break;

        case 0:
        default:
            lua_rawget(L, LUA_REGISTRYINDEX);       // reclaim the newthread
            luaL_unref(L, -1, http_ctx->ref);
            lua_pop(L, 1);

            TSfree(http_ctx);
            break;
    }

    ts_lua_reclaim_unref(thread_ctx);

    return TSREMAP_NO_REMAP;
}

