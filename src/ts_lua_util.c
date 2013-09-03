
#include "ts_lua_util.h"
#include "ts_lua_client_request.h"
#include "ts_lua_client_response.h"

static void ts_lua_init_registry(lua_State *L);
static void ts_lua_init_globals(lua_State *L);
static void ts_lua_inject_ts_api(lua_State *L);


lua_State *
ts_lua_new_state()
{
    lua_State       *L;

    L = luaL_newstate();

    if (L == NULL) {
        return NULL;
    }

    luaL_openlibs(L);

    ts_lua_init_registry(L);

    ts_lua_init_globals(L);

    return L;
}

ts_lua_thread_ctx *
ts_lua_init_thread(lua_State *l)
{
    ts_lua_thread_ctx * ctx = (ts_lua_thread_ctx*)TSmalloc(sizeof(ts_lua_thread_ctx));
    if (!ctx)
        return NULL;

    ctx->lua = l;
    ctx->reclaim_time = 0;
    ts_lua_atomiclist_init(&ctx->reclaim_list, "reclaim list", offsetof(ts_lua_ref, next));

    return ctx;
}

void
ts_lua_reclaim_unref(ts_lua_thread_ctx *ctx)
{
    ts_lua_ref  *refp;
    lua_State   *L;

    time_t now = TShrtime()/1000000;

    if (now - ctx->reclaim_time < 10)
        return;

    L = ctx->lua;
    lua_rawget(L, LUA_REGISTRYINDEX);

    refp = ts_lua_atomiclist_popall(&ctx->reclaim_list);
    while (refp) {
        luaL_unref(L, -1, refp->ref);
        TSfree(refp);
        refp = refp->next;
    }

    lua_pop(L, 1);
    ctx->reclaim_time = now;
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
ts_lua_create_http_ctx(ts_lua_thread_ctx *thread_ctx)
{
    int                 base;
    ts_lua_http_ctx     *http_ctx;
    lua_State           *L;

    L = thread_ctx->lua;

    http_ctx = TSmalloc(sizeof(ts_lua_http_ctx));
    memset(http_ctx, 0, sizeof(ts_lua_http_ctx));

    base = lua_gettop(L);
//    lua_rawget(L, LUA_REGISTRYINDEX);

    http_ctx->lua = lua_newthread(L);

//   int t1 = lua_type(L, -1);
//   int t2 = lua_type(L, -2);

//    if (t1 == t2)
//        return NULL;

    http_ctx->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_settop(L, base);

    http_ctx->th_ctx = thread_ctx;
    ts_lua_set_http_ctx(http_ctx->lua, http_ctx);

    return http_ctx;
}

void
ts_lua_destroy_http_ctx(ts_lua_http_ctx* http_ctx)
{
    ts_lua_ref          *refp;
    ts_lua_thread_ctx   *thread_ctx;

    thread_ctx = http_ctx->th_ctx;

    if (http_ctx->client_response_hdrp) {
        TSHandleMLocRelease(http_ctx->client_response_bufp, TS_NULL_MLOC, http_ctx->client_response_hdrp);
    }

    refp = (ts_lua_ref*)TSmalloc(sizeof(ts_lua_ref));
    refp->ref = http_ctx->ref;
    ts_lua_atomiclist_push(&thread_ctx->reclaim_list, refp);

    TSfree(http_ctx);
}

