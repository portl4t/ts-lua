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
#include "ts_lua_io.h"
#include "ts_lua_cache.h"

typedef enum {
    TS_LUA_CACHE_READ = 1,
    TS_LUA_CACHE_WRITE
} TSLuaCacheOperations;

ts_lua_var_item ts_lua_cache_vars[] = {
    TS_LUA_MAKE_VAR_ITEM(TS_LUA_CACHE_READ),
    TS_LUA_MAKE_VAR_ITEM(TS_LUA_CACHE_WRITE)
};

static void ts_lua_inject_cache_variables(lua_State *L);

static int ts_lua_cache_open(lua_State *L);
static int ts_lua_cache_read(lua_State *L);
static int ts_lua_cache_write(lua_State *L);
static int ts_lua_cache_close(lua_State *L);
static int ts_lua_cache_eof(lua_State *L);
static int ts_lua_cache_err(lua_State *L);
static int ts_lua_cache_open(lua_State *L);
static int ts_lua_cache_remove(lua_State *L);

static TSCacheKey ts_lua_cache_key_create(const char *keystr, size_t key_len, const char *optstr, size_t opt_len);
static int ts_lua_cache_open_read(ts_lua_cont_info *ci, ts_lua_cache_info *info, TSEvent event, void *edata);
static int ts_lua_cache_open_write(ts_lua_cont_info *ci, ts_lua_cache_info *info, TSEvent event, void *edata);
static int ts_lua_cache_main_handler(TSCont contp, TSEvent event, void *edata);
static int ts_lua_cache_handle_read(ts_lua_cont_info *ci, ts_lua_cache_info *info, TSEvent event, void *edata);
static int ts_lua_cache_handle_write(ts_lua_cont_info *ci, ts_lua_cache_info *info, TSEvent event, void *edata);
static int ts_lua_cache_remove_handler(TSCont contp, TSEvent event, void *edata);
static int ts_lua_cache_cleanup(ts_lua_async_item *ai);
static void ts_lua_release_cache_info(ts_lua_cache_info *info, int destroy);


void
ts_lua_inject_cache_api(lua_State *L)
{
    /* variables */
    ts_lua_inject_cache_variables(L);

    /* ts.cache_open() */
    lua_pushcfunction(L, ts_lua_cache_open);
    lua_setfield(L, -2, "cache_open");

    /* ts.cache_read() */
    lua_pushcfunction(L, ts_lua_cache_read);
    lua_setfield(L, -2, "cache_read");

    /* ts.cache_write() */
    lua_pushcfunction(L, ts_lua_cache_write);
    lua_setfield(L, -2, "cache_write");

    /* ts.cache_close() */
    lua_pushcfunction(L, ts_lua_cache_close);
    lua_setfield(L, -2, "cache_close");

    /* ts.cache_eof() */
    lua_pushcfunction(L, ts_lua_cache_eof);
    lua_setfield(L, -2, "cache_eof");

    /* ts.cache_err() */
    lua_pushcfunction(L, ts_lua_cache_err);
    lua_setfield(L, -2, "cache_err");

    /* ts.cache_remove() */
    lua_pushcfunction(L, ts_lua_cache_remove);
    lua_setfield(L, -2, "cache_remove");
}

static void
ts_lua_inject_cache_variables(lua_State *L)
{
    int i;

    for (i = 0; i < sizeof(ts_lua_cache_vars) / sizeof(ts_lua_var_item); i++) {
        lua_pushinteger(L, ts_lua_cache_vars[i].nvar);
        lua_setglobal(L, ts_lua_cache_vars[i].svar);
    }
}

static int
ts_lua_cache_open(lua_State *L)
{
    const char              *keystr, *optstr;
    size_t                  key_len, opt_len;
    int                     operate, n;
    TSCont                  contp;
    TSCacheKey              key;
    TSAction                action;
    ts_lua_cont_info        *ci;
    ts_lua_async_item       *ai;
    ts_lua_cache_info       *info;

    ci = ts_lua_get_cont_info(L);
    if (ci == NULL)
        return 0;

    n = lua_gettop(L);
    if (n < 2) {
        return luaL_error(L, "'ts.cache_open' requires parameter");
    }

    /* keystr */
    if (!lua_isstring(L, 1)) {
        return luaL_error(L, "'ts.cache_open' first param is not string");

    } else if (!lua_isnumber(L, 2)) {
        return luaL_error(L, "'ts.cache_open' second param is not TS_LUA_CACHE_READ/TS_LUA_CACHE_WRITE");
    }

    keystr = luaL_checklstring(L, 1, &key_len);
    operate = lua_tonumber(L, 2);

    if (operate != TS_LUA_CACHE_READ && operate != TS_LUA_CACHE_WRITE) {
        return luaL_error(L, "'ts.cache_open' second param is not TS_LUA_CACHE_READ/TS_LUA_CACHE_WRITE");
    }

    optstr = NULL;
    opt_len = 0;

    /* option */
    if (n >= 3 && lua_isstring(L, 3)) {
        optstr = luaL_checklstring(L, 3, &opt_len);
    }

    key = ts_lua_cache_key_create(keystr, key_len, optstr, opt_len);
    if (key == NULL) {
        return luaL_error(L, "'ts.cache_open' failed");
    }

    info = (ts_lua_cache_info*)TSmalloc(sizeof(ts_lua_cache_info));
    memset(info, 0, sizeof(ts_lua_cache_info));

    info->cache_key = key;
    info->optype = operate;  

    contp = TSContCreate(ts_lua_cache_main_handler, ci->mutex);
    ai = ts_lua_async_create_item(contp, ts_lua_cache_cleanup, info, ci);

    TSContDataSet(contp, ai);

    info->contp = contp;
    info->ioh.buffer = TSIOBufferCreate();
    info->ioh.reader = TSIOBufferReaderAlloc(info->ioh.buffer);

    if (operate == TS_LUA_CACHE_READ) {
        info->reserved.buffer = TSIOBufferCreate();
        info->reserved.reader = TSIOBufferReaderAlloc(info->reserved.buffer);
        info->current_handler = &ts_lua_cache_open_read;

        action = TSCacheRead(contp, key);

    } else {
        info->current_handler = &ts_lua_cache_open_write;
        action = TSCacheWrite(contp, key);
    }

    if (TSActionDone(action)) {     // done
        return 1;

    } else {                        // undone
        info->cache_action = action;
        return lua_yield(L, 0);
    }
}

static int
ts_lua_cache_read(lua_State *L)
{
    int64_t                 length, n, avail;
    char                    *dst;
    ts_lua_cont_info        *ci;
    ts_lua_cache_info       *info;

    ci = ts_lua_get_cont_info(L);
    if (ci == NULL)
        return 0;

    n = lua_gettop(L);
    if (n < 1) {
        return luaL_error(L, "'ts.cache_read' requires parameter");
    }

    /* length */
    if (n >= 2) {
        if (!lua_isnumber(L, 2)) {
            return luaL_error(L, "'ts.cache_read' second param is not number");
        }

        length = lua_tonumber(L, 2);
        if (length <= 0) {
            return luaL_error(L, "'ts.cache_read' second param is a not valid number");
        }

    } else {
        length = INT64_MAX;
    }

    /* table */
    if (!lua_istable(L, 1)) {
        return luaL_error(L, "'ts.cache_read' first param is not valid");
    }

    lua_pushlstring(L, "info", sizeof("info") - 1);
    lua_gettable(L, 1);

    if (!lua_islightuserdata(L, -1)) {
        return luaL_error(L, "'ts.cache_read' first param is not valid");
    }

    info = (ts_lua_cache_info*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (info->hit == 0) {
        return luaL_error(L, "'ts.cache_read' is reading from unhit doc");
    }

    if (info->optype != TS_LUA_CACHE_READ) {
        return luaL_error(L, "'ts.cache_read' is reading from invalid vc");
    }

    if (info->eof || info->err) {
        lua_pushnil(L);
        return 1;
    }

    if (length != INT64_MAX) {
        info->need += length;

    } else {
        info->need = INT64_MAX;
    }

    info->current_handler = &ts_lua_cache_handle_read;

    avail = TSIOBufferReaderAvail(info->reserved.reader);
    if (avail + info->already >= info->need) {
        n = info->need - info->already;

        dst = (char*)TSmalloc(n);
        IOBufferReaderCopy(info->reserved.reader, dst, n);
        lua_pushlstring(L, (char*)dst, n);
        TSfree(dst);

        info->already = info->need;
        TSIOBufferReaderConsume(info->reserved.reader, n);
        return 1;
    }

    if (info->ioh.vio == NULL) {
        info->ioh.vio = TSVConnRead(info->cache_vc, info->contp, info->ioh.buffer, INT64_MAX);

    } else {
        TSVIOReenable(info->ioh.vio);
    }

    info->wait = 1;
    return lua_yield(L, 0);
}

static int
ts_lua_cache_write(lua_State *L)
{
    int64_t                 length, n;
    const char              *data;
    size_t                  dlen;
    ts_lua_cont_info        *ci;
    ts_lua_cache_info       *info;

    ci = ts_lua_get_cont_info(L);
    if (ci == NULL)
        return 0;

    n = lua_gettop(L);
    if (n < 2) {
        return luaL_error(L, "'ts.cache_write' requires parameter");
    }

    /* data */
    if (!lua_isstring(L, 2)) {
        return luaL_error(L, "'ts.cache_write' second param is not string");
    }

    data = luaL_checklstring(L, 2, &dlen);

    /* length */
    length = dlen;

    if (n >= 3) {
        if (!lua_isnumber(L, 3)) {
            return luaL_error(L, "'ts.cache_write' third param is not number");
        }

        length = lua_tonumber(L, 3);
        if (length <= 0) {
            return luaL_error(L, "'ts.cache_write' third param is a not valid number");
        }

        if (length > dlen)
            length = dlen;
    }

    /* table */
    if (!lua_istable(L, 1)) {
        return luaL_error(L, "'ts.cache_write' first param is not valid");
    }

    lua_pushlstring(L, "info", sizeof("info") - 1);
    lua_gettable(L, 1);

    if (!lua_islightuserdata(L, -1)) {
        return luaL_error(L, "'ts.cache_write' first param is not valid");
    }

    info = (ts_lua_cache_info*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (info->optype != TS_LUA_CACHE_WRITE) {
        return luaL_error(L, "'ts.cache_write' is writing to invalid vc");
    }

    if (info->err) {
        lua_pushnil(L);
        return 1;
    }

    info->need += length;
    info->current_handler = &ts_lua_cache_handle_write;
    TSIOBufferWrite(info->ioh.buffer, data, length);

    if (info->ioh.vio == NULL) {
        info->ioh.vio = TSVConnWrite(info->cache_vc, info->contp, info->ioh.reader, INT64_MAX);

    } else {
        TSVIOReenable(info->ioh.vio);
    }

    info->wait = 1;
    return lua_yield(L, 0);
}

static int
ts_lua_cache_main_handler(TSCont contp, TSEvent event, void *edata)
{
    ts_lua_async_item       *ai;
    ts_lua_cache_info       *info;
    ts_lua_cont_info        *ci;

    ai = TSContDataGet(contp);
    ci = ai->cinfo;

    info = (ts_lua_cache_info*)ai->data;

    return info->current_handler(ci, info, event, edata);
}

static int
ts_lua_cache_open_read(ts_lua_cont_info *ci, ts_lua_cache_info *info, TSEvent event, void *edata)
{
    lua_State       *L;
    TSMutex         mtx;
    TSVConn         vc;
    int64_t         sz;

    L = ci->routine.lua;
    mtx = ci->routine.mctx->mutexp;

    TSMutexLock(mtx);

    // result table
    lua_newtable(L);

    switch (event) {

        case TS_EVENT_CACHE_OPEN_READ:
            vc = (TSVConn)edata;
            sz = TSVConnCacheObjectSizeGet(vc);

            info->cache_vc = vc;
            info->hit = 1;
            info->sz = sz;

            lua_pushlstring(L, "hit", sizeof("hit") - 1);
            lua_pushboolean(L, 1);
            lua_rawset(L, -3);

            lua_pushlstring(L, "size", sizeof("size") - 1);
            lua_pushnumber(L, sz);
            lua_rawset(L, -3);

            break;

        case TS_EVENT_CACHE_OPEN_READ_FAILED:               // miss
        default:                                            // error
            lua_pushlstring(L, "hit", sizeof("hit") - 1);
            lua_pushboolean(L, 0);
            lua_rawset(L, -3);

            lua_pushlstring(L, "size", sizeof("size") - 1);
            lua_pushnumber(L, -1);
            lua_rawset(L, -3);

            if (event != TS_EVENT_CACHE_OPEN_READ_FAILED) {
                info->err = 1;
            }

            break;
    }

    lua_pushlstring(L, "info", sizeof("info") - 1);
    lua_pushlightuserdata(L, info);
    lua_rawset(L, -3);

    if (info->cache_action) {
        info->cache_action = NULL;
        TSContCall(ci->contp, TS_LUA_EVENT_COROUTINE_CONT, (void*)1);

    } else {
        // return to the ts.cache_open synchronized
    }

    TSMutexUnlock(mtx);
    return 0;
}

static int
ts_lua_cache_open_write(ts_lua_cont_info *ci, ts_lua_cache_info *info, TSEvent event, void *edata)
{
    lua_State       *L;
    TSMutex         mtx;
    TSVConn         vc;

    L = ci->routine.lua;
    mtx = ci->routine.mctx->mutexp;

    switch (event) {

        case TS_EVENT_CACHE_OPEN_WRITE:
            vc = (TSVConn)edata;
            info->cache_vc = vc;
            break;

        default:                // error
            info->err = 1;
            break;
    }

    TSMutexLock(mtx);

    // result table
    lua_newtable(L);
    lua_pushlstring(L, "info", sizeof("info") - 1);
    lua_pushlightuserdata(L, info);
    lua_rawset(L, -3);

    if (info->cache_action) {
        info->cache_action = NULL;
        TSContCall(ci->contp, TS_LUA_EVENT_COROUTINE_CONT, (void*)1);

    } else {
        // return to the ts.cache_open synchronized
    }

    TSMutexUnlock(mtx);

    return 0;
}

static int
ts_lua_cache_handle_write(ts_lua_cont_info *ci, ts_lua_cache_info *info, TSEvent event, void *edata)
{
    lua_State               *L;
    TSMutex                 mtx;
    int64_t                 avail, done, n;

    n = 0;

    switch (event) {

        case TS_EVENT_VCONN_WRITE_READY:
            done = TSVIONDoneGet(info->ioh.vio);
            if (done < info->need) {
                TSVIOReenable(info->ioh.vio);

            } else {
                n = info->need - info->already;
                avail = TSIOBufferReaderAvail(info->ioh.reader);
                TSIOBufferReaderConsume(info->ioh.reader, avail);
            }

            break;

        default:
            info->err = 1;
            break;
    }

    if (info->wait && (n > 0 || info->err)) {           // resume
        L = ci->routine.lua;
        mtx = ci->routine.mctx->mutexp;

        TSMutexLock(mtx);

        if (n > 0) {
            lua_pushnumber(L, n);
            info->already = info->need;

        } else {
            lua_pushnumber(L, 0);
        }

        info->wait = 0;
        TSContCall(ci->contp, TS_LUA_EVENT_COROUTINE_CONT, (void*)1);
        TSMutexUnlock(mtx);
    }

    return 0;
}

static int
ts_lua_cache_handle_read(ts_lua_cont_info *ci, ts_lua_cache_info *info, TSEvent event, void *edata)
{
    lua_State               *L;
    TSMutex                 mtx;
    char                    *dst;
    int64_t                 avail, n;

    n = 0;

    switch (event) {

        case TS_EVENT_VCONN_READ_READY:
            avail = TSIOBufferReaderAvail(info->ioh.reader);
            if (avail > 0) {
                TSIOBufferCopy(info->reserved.buffer, info->ioh.reader, avail, 0);
                TSIOBufferReaderConsume(info->ioh.reader, avail);
            }

            avail = TSIOBufferReaderAvail(info->reserved.reader);
            if (avail + info->already >= info->need) {
                n = info->need - info->already;

            } else {
                TSVIOReenable(info->ioh.vio);
            }

            break;

        case TS_EVENT_VCONN_READ_COMPLETE:
        case TS_EVENT_VCONN_EOS:
            avail = TSIOBufferReaderAvail(info->ioh.reader);
            if (avail > 0) {
                TSIOBufferCopy(info->reserved.buffer, info->ioh.reader, avail, 0);
                TSIOBufferReaderConsume(info->ioh.reader, avail);
            }

            n = TSIOBufferReaderAvail(info->reserved.reader);

            info->eof = 1;
            break;

        default:                                // error
            info->err = 1;
            break;
    }

    if (info->wait && (n > 0 || info->eof || info->err)) {      // resume
        L = ci->routine.lua;
        mtx = ci->routine.mctx->mutexp;

        TSMutexLock(mtx);

        if (n > 0) {
            dst = TSmalloc(n);
            IOBufferReaderCopy(info->reserved.reader, dst, n);
            lua_pushlstring(L, (char*)dst, n);
            TSfree(dst);

            info->already += n;
            TSIOBufferReaderConsume(info->reserved.reader, n);

        } else {
            lua_pushnil(L);
        }

        info->wait = 0;

        TSContCall(ci->contp, TS_LUA_EVENT_COROUTINE_CONT, (void*)1);
        TSMutexUnlock(mtx);
    }

    return 0;
}

static TSCacheKey
ts_lua_cache_key_create(const char *keystr, size_t key_len, const char *optstr, size_t opt_len)
{
    const char          *ptr, *end, *host;
    char                c;
    int                 host_len;
    TSCacheKey          key;
    TSMBuffer           urlBuf;
    TSMLoc              urlLoc;
    int                 url, http;

    ptr = optstr;
    end = optstr + opt_len;
    url = http = 0;

    while (ptr < end) {
        c = *ptr;

        switch (c) {
            case 'u':
                url = 1;
                break;

            case 'h':
                http = 1;
                break;

            default:
                break;
        }

        ptr++;
    }

    key = TSCacheKeyCreate();
    urlBuf = NULL;

    if (url == 0) {
        TSCacheKeyDigestSet(key, keystr, key_len);

    } else {
        end = keystr + key_len;
        ptr = memchr(keystr, ':', key_len);
        if (ptr == NULL)
            goto fail;

        ptr += 3;
        if (ptr >= end)
            goto fail;

        host = ptr;
        ptr = memchr(host, '/', end-host);
        if (ptr != NULL) {
            host_len = ptr - host;

        } else {
            host_len = end - host;
        }

        if (host_len <= 0)
            goto fail;

        ptr = memchr(host, ':', host_len);
        if (ptr != NULL) {
            host_len = ptr - host;
        }

        urlBuf = TSMBufferCreate();
        TSUrlCreate(urlBuf, &urlLoc);

        if (TSUrlParse(urlBuf, urlLoc, (const char **) &keystr, end) != TS_PARSE_DONE ||
                TSCacheKeyDigestFromUrlSet(key, urlLoc) != TS_SUCCESS) {
            goto fail;
        }

        TSHandleMLocRelease(urlBuf, NULL, urlLoc);
        TSMBufferDestroy(urlBuf);

        TSCacheKeyHostNameSet(key, host, host_len);
    }

    if (http) {
        TSCacheKeyDataTypeSet(key, TS_CACHE_DATA_TYPE_HTTP);
    }

    return key;

fail:

    TSCacheKeyDestroy(key);
    if (urlBuf) {
        TSHandleMLocRelease(urlBuf, NULL, urlLoc);
        TSMBufferDestroy(urlBuf);
    }

    return NULL;
}

static int
ts_lua_cache_close(lua_State *L)
{
    int                     n;
    ts_lua_cont_info        *ci;
    ts_lua_cache_info       *info;

    ci = ts_lua_get_cont_info(L);
    if (ci == NULL)
        return 0;

    n = lua_gettop(L);
    if (n < 1) {
        return luaL_error(L, "'ts.cache_close' requires parameter");
    }

    /* table */
    if (!lua_istable(L, 1)) {
        return luaL_error(L, "'ts.cache_close' first param is not valid");
    }

    lua_pushlstring(L, "info", sizeof("info") - 1);
    lua_gettable(L, 1);

    if (!lua_islightuserdata(L, -1)) {
        return luaL_error(L, "'ts.cache_close' first param is not valid");
    }

    info = (ts_lua_cache_info*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    ts_lua_release_cache_info(info, 0);

    return 0;
}

static int
ts_lua_cache_eof(lua_State *L)
{
    int                     n;
    ts_lua_cont_info        *ci;
    ts_lua_cache_info       *info;

    ci = ts_lua_get_cont_info(L);
    if (ci == NULL)
        return 0;

    n = lua_gettop(L);
    if (n < 1) {
        return luaL_error(L, "'ts.cache_eof' requires parameter");
    }

    /* table */
    if (!lua_istable(L, 1)) {
        return luaL_error(L, "'ts.cache_eof' first param is not valid");
    }

    lua_pushlstring(L, "info", sizeof("info") - 1);
    lua_gettable(L, 1);

    if (!lua_islightuserdata(L, -1)) {
        return luaL_error(L, "'ts.cache_eof' first param is not valid");
    }

    info = (ts_lua_cache_info*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (info->eof) {
        lua_pushboolean(L, 1);

    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

static int
ts_lua_cache_err(lua_State *L)
{
    int                     n;
    ts_lua_cont_info        *ci;
    ts_lua_cache_info       *info;

    ci = ts_lua_get_cont_info(L);
    if (ci == NULL)
        return 0;

    n = lua_gettop(L);
    if (n < 1) {
        return luaL_error(L, "'ts.cache_err' requires parameter");
    }

    /* table */
    if (!lua_istable(L, 1)) {
        return luaL_error(L, "'ts.cache_err' first param is not valid");
    }

    lua_pushlstring(L, "info", sizeof("info") - 1);
    lua_gettable(L, 1);

    if (!lua_islightuserdata(L, -1)) {
        return luaL_error(L, "'ts.cache_err' first param is not valid");
    }

    info = (ts_lua_cache_info*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (info->err) {
        lua_pushboolean(L, 1);

    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

static int
ts_lua_cache_remove(lua_State *L)
{
    const char              *keystr, *optstr;
    size_t                  key_len, opt_len;
    int                     n;
    TSCont                  contp;
    TSCacheKey              key;
    TSAction                action;
    ts_lua_cont_info        *ci;
    ts_lua_async_item       *ai;
    ts_lua_cache_info       *info;

    ci = ts_lua_get_cont_info(L);
    if (ci == NULL)
        return 0;

    n = lua_gettop(L);
    if (n < 1) {
        return luaL_error(L, "'ts.cache_remove' requires parameter");
    }

    /* keystr */
    if (!lua_isstring(L, 1)) {
        return luaL_error(L, "'ts.cache_remove' first param is not string");
    }

    keystr = luaL_checklstring(L, 1, &key_len);

    optstr = NULL;
    opt_len = 0;

    /* option */
    if (n >= 2 && lua_isstring(L, 2)) {
        optstr = luaL_checklstring(L, 2, &opt_len);;
    }

    key = ts_lua_cache_key_create(keystr, key_len, optstr, opt_len);
    if (key == NULL) {
        return luaL_error(L, "'ts.cache_remove' failed");
    }

    info = (ts_lua_cache_info*)TSmalloc(sizeof(ts_lua_cache_info));
    memset(info, 0, sizeof(ts_lua_cache_info));
    info->cache_key = key;

    contp = TSContCreate(ts_lua_cache_remove_handler, ci->mutex);
    ai = ts_lua_async_create_item(contp, ts_lua_cache_cleanup, info, ci);
    TSContDataSet(contp, ai);

    action = TSCacheRemove(contp, key);

    if (!TSActionDone(action)) {
        info->cache_action = action;
        return lua_yield(L, 0);

    } else {            // action done
        ts_lua_release_cache_info(info, 1);
        ai->data = NULL;
        return 0;
    }
}

static int
ts_lua_cache_remove_handler(TSCont contp, TSEvent event, void *edata)
{
    ts_lua_async_item       *ai;
    ts_lua_cache_info       *info;
    ts_lua_cont_info        *ci;

    ai = TSContDataGet(contp);
    ci = ai->cinfo;

    info = (ts_lua_cache_info*)ai->data;

    if (info->cache_action) {
        info->cache_action = NULL;

        ts_lua_release_cache_info(info, 1);
        ai->data = NULL;

        TSContCall(ci->contp, TS_LUA_EVENT_COROUTINE_CONT, 0);
    }

    return 0;
}

static int
ts_lua_cache_cleanup(ts_lua_async_item *ai)
{
    ts_lua_cache_info     *info;

    if (ai->data) {
        info = (ts_lua_cache_info*)ai->data;
        ts_lua_release_cache_info(info, 1);

        ai->data = NULL;
    }

    TSContDestroy(ai->contp);
    ai->deleted = 1;

    return 0;
}

static void
ts_lua_release_cache_info(ts_lua_cache_info *info, int destroy)
{
    if (info == NULL)
        return;

    if (info->cache_action) {
        TSActionCancel(info->cache_action);
        info->cache_action = NULL;
    }

    if (info->cache_key) {
        TSCacheKeyDestroy(info->cache_key);
        info->cache_key = NULL;
    }

    if (info->cache_vc) {
        TSVConnClose(info->cache_vc);
        info->cache_vc = NULL;
    }

    TS_LUA_RELEASE_IO_HANDLE((&info->ioh));
    TS_LUA_RELEASE_IO_HANDLE((&info->reserved));

    if (destroy) {
        TSfree(info);
    }
}
