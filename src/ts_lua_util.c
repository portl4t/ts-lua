
#include "ts_lua_util.h"
#include "ts_lua_client_request.h"
#include "ts_lua_client_response.h"
#include "ts_lua_context.h"

static lua_State * ts_lua_new_state();
static void ts_lua_init_registry(lua_State *L);
static void ts_lua_init_globals(lua_State *L);
static void ts_lua_inject_ts_api(lua_State *L);


int
ts_lua_create_vm(ts_lua_main_ctx *arr, int n)
{
    int         i;
    lua_State   *L;

    for (i = 0; i < n; i++) {

        L = ts_lua_new_state();

        if (L == NULL)
            return -1;

        lua_pushvalue(L, LUA_GLOBALSINDEX);

        arr[i].gref = luaL_ref(L, LUA_REGISTRYINDEX);
        arr[i].lua = L;
        arr[i].mutexp = TSMutexCreate();
    }

    return 0;
}

void
ts_lua_destroy_vm(ts_lua_main_ctx *arr, int n)
{
    int         i;
    lua_State   *L;

    for (i = 0; i < n; i++) {

        L = arr[i].lua;
        if (L)
            lua_close(L);
    }

    return;
}

lua_State *
ts_lua_new_state()
{
    lua_State   *L;

    L = luaL_newstate();

    if (L == NULL) {
        return NULL;
    }

    luaL_openlibs(L);

    ts_lua_init_registry(L);

    ts_lua_init_globals(L);

    return L;
}

int
ts_lua_add_module(ts_lua_instance_conf *conf, ts_lua_main_ctx *arr, int n)
{
    int             i, ret, base;
    lua_State       *L;

    for (i = 0; i < n; i++) {

        L = arr[i].lua;

        base = lua_gettop(L);

        lua_newtable(L);                                    // create this module's global table
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "_G");
        lua_newtable(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, arr[i].gref);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -2);
        lua_replace(L, LUA_GLOBALSINDEX);

        ret = luaL_loadfile(L, conf->script);
        if (ret) {
            TSError("[%s] luaL_loadfile %s failed: %s\n", __FUNCTION__, conf->script, lua_tostring(L, -1));
            lua_pop(L, 1);
            return -1;
        }

        ret = lua_pcall(L, 0, 0, 0);
        if (ret) {
            TSError("[%s] lua_pcall %s failed: %s\n", __FUNCTION__, conf->script, lua_tostring(L, -1));
            lua_pop(L, 1);
            return -1;
        }

        base = lua_gettop(L);

        lua_pushlightuserdata(L, conf);
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        lua_rawset(L, LUA_REGISTRYINDEX);

        lua_newtable(L);
        lua_replace(L, LUA_GLOBALSINDEX);               // set empty table to global
        base = lua_gettop(L);
    }


    return 0;
}


static
void ts_lua_init_registry(lua_State *L)
{
    return;
}

static
void ts_lua_init_globals(lua_State *L)
{
    ts_lua_inject_ts_api(L);
}

static void
ts_lua_inject_ts_api(lua_State *L)
{
    lua_newtable(L);

    ts_lua_inject_client_request_api(L);
    ts_lua_inject_client_response_api(L);

    ts_lua_inject_context_api(L);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaded");
    lua_pushvalue(L, -3);
    lua_setfield(L, -2, "ts");
    lua_pop(L, 2);

    lua_setglobal(L, "ts");
}

void
ts_lua_set_http_ctx(lua_State *L, ts_lua_http_ctx  *ctx)
{
    lua_pushliteral(L, "__ts_http_ctx");
    lua_pushlightuserdata(L, ctx);
    lua_rawset(L, LUA_GLOBALSINDEX);
}

ts_lua_http_ctx *
ts_lua_get_http_ctx(lua_State *L)
{
    ts_lua_http_ctx  *ctx;

    lua_pushliteral(L, "__ts_http_ctx");
    lua_rawget(L, LUA_GLOBALSINDEX);
    ctx = lua_touserdata(L, -1);

    lua_pop(L, 1);                      // pop the ctx out

    return ctx;
}

ts_lua_http_ctx *
ts_lua_create_http_ctx(ts_lua_main_ctx *main_ctx, ts_lua_instance_conf *conf)
{
//    int                 base;
    ts_lua_http_ctx     *http_ctx;
    lua_State           *L;
    lua_State           *l;

    L = main_ctx->lua;

    http_ctx = TSmalloc(sizeof(ts_lua_http_ctx));
    memset(http_ctx, 0, sizeof(ts_lua_http_ctx));

    http_ctx->lua = lua_newthread(L);
    l = http_ctx->lua;

    lua_pushlightuserdata(L, conf);
    lua_rawget(L, LUA_REGISTRYINDEX);

    /* new globals table for coroutine */
    lua_newtable(l);
    lua_pushvalue(l, -1);
    lua_setfield(l, -2, "_G"); 
    lua_newtable(l);
    lua_xmove(L, l, 1);
    lua_setfield(l, -2, "__index");
    lua_setmetatable(l, -2);

    lua_replace(l, LUA_GLOBALSINDEX);

    http_ctx->ref = luaL_ref(L, LUA_REGISTRYINDEX);

    http_ctx->mctx = main_ctx;
    ts_lua_set_http_ctx(http_ctx->lua, http_ctx);

    return http_ctx;
}


void
ts_lua_destroy_http_ctx(ts_lua_http_ctx* http_ctx)
{
    ts_lua_main_ctx   *main_ctx;

    main_ctx = http_ctx->mctx;

    if (http_ctx->client_response_hdrp) {
        TSHandleMLocRelease(http_ctx->client_response_bufp, TS_NULL_MLOC, http_ctx->client_response_hdrp);
    }

    luaL_unref(main_ctx->lua, LUA_REGISTRYINDEX, http_ctx->ref);
    TSfree(http_ctx);
}

