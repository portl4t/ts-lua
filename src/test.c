

#include <pthread.h>
#include <ts/ts.h>
#include <ts/experimental.h>
#include <ts/remap.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


pthread_key_t lua_state_key;


TSReturnCode
TSRemapInit(TSRemapInterface *api_info, char *errbuf, int errbuf_size)
{
    if (!api_info)
        return TS_ERROR;

    if (api_info->size < sizeof(TSRemapInterface))
        return TS_ERROR;

    int status = pthread_key_create(&lua_state_key, NULL);

    if (status)
        return TS_ERROR;

    return TS_SUCCESS;
}

TSReturnCode
TSRemapNewInstance(int argc, char* argv[], void** ih, char* errbuf, int errbuf_size)
{
    return TS_SUCCESS;
}

void
TSRemapDeleteInstance(void* ih)
{
    return;
}

TSRemapStatus
TSRemapDoRemap(void* ih, TSHttpTxn rh, TSRemapRequestInfo *rri)
{
    const char  *uri;
    int         uri_len;
    int         ret, match;

    lua_State *l = (lua_State*)pthread_getspecific(lua_state_key);
    if (l == NULL) {
        l = luaL_newstate();
        luaopen_string(l);
        ret = luaL_loadfile(l, "/home/quehan/programming/lua/invoke/ss.lua");
        ret = lua_pcall(l, 0, 0, 0);
        pthread_setspecific(lua_state_key, l);
    }

    uri = TSUrlPathGet(rri->requestBufp, rri->requestUrl, &uri_len);

    lua_getglobal(l, "ssub");
    lua_pushlstring(l, uri, uri_len);

    ret = lua_pcall(l, 1, 1, 0);

    match = lua_tointeger(l, -1);

    lua_pop(l, 1);

    if (match) {
        TSDebug("ts_lua", "url matches :)");
    }

    return TSREMAP_NO_REMAP;
}

