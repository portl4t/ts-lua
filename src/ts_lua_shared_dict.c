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
#include "ts_lua_hash_table.h"

#define TS_LUA_SHDICT_FLAG_INTKEY       (1 << 0)
#define TS_LUA_SHDICT_FLAG_STATIC       (1 << 1)

#define TS_LUA_SHDICT_MAX_KEY_LENGTH    ((1 << sizeof(uint16_t) * 8) - 1)
#define TS_LUA_SHDICT_MAX_VAL_LENGTH    TS_LUA_SHDICT_MAX_KEY_LENGTH


typedef struct {
    ts_lua_shared_dict  *dct;
    lua_State           *l;
    int                 n;
    int                 max;
} ts_lua_shared_dict_keys_ctx;


static int ts_lua_get_shared_dict(lua_State *L);

static int ts_lua_shared_dict_set_metatable(lua_State *L);

static int ts_lua_shared_dict_init(lua_State *L);
static int ts_lua_shared_dict_set(lua_State *L);
static int ts_lua_shared_dict_get(lua_State *L);
static int ts_lua_shared_dict_del(lua_State *L);
static int ts_lua_shared_dict_get_keys(lua_State *L);
static int ts_lua_shared_dict_get_size(lua_State *L);

static int dump_entry_key(Tcl_HashTable *t, Tcl_HashEntry *entry, void *data);
static int free_entry(Tcl_HashTable *t, Tcl_HashEntry *entry, void *data);

void
ts_lua_inject_shared_dict_api(lua_State *L)
{
    lua_newtable(L);

    lua_pushcfunction(L, ts_lua_get_shared_dict);
    lua_setfield(L, -2, "DICT");

    lua_setfield(L, -2, "shared");
}

static int
ts_lua_get_shared_dict(lua_State *L)
{
    char                    c;
    int                     n, i, option;
    int64_t                 quota;
    const char              *name, *opt;
    size_t                  name_len, opt_len;
    ts_lua_shared_dict      *dct;
    ts_lua_instance_conf    *conf;

    n = lua_gettop(L);
    conf = ts_lua_get_instance_conf(L);

    if (n < 1) {
        return luaL_error(L, "ts.shared.DICT expecting name argument.");
    }

    name = lua_tolstring(L, 1, &name_len);

    if (name == NULL || name_len == 0) {
        return luaL_error(L, "The 1st param of ts.shared.DICT should be string");

    } else if (name_len >= TS_LUA_MAX_SHARED_DICT_NAME_LENGTH - 4) {
        return luaL_error(L, "length of name for ts.shared.DICT is more than %d",
                          TS_LUA_MAX_SHARED_DICT_NAME_LENGTH);
    }

    for (i = 0; i < conf->shdict_n; i++) {

        dct = &conf->shdict[i];

        if (strlen(dct->name) == name_len &&
                memcmp(dct->name, name, name_len) == 0) {

            lua_pushlightuserdata(L, dct);
            ts_lua_shared_dict_set_metatable(L);
            return 1;
        }
    }

    if (conf->shdict_n >= TS_LUA_MAX_SHARED_DICT_COUNT)
        return luaL_error(L, "There are too many shared dicts.");

    option = quota = 0;

    if (n == 2) {
        if (!lua_istable(L, 2)) {
            return luaL_error(L, "The 2nd param of ts.shared.DICT should be a table.");
        }
        
        /* quota */
        lua_pushlstring(L, "quota", sizeof("quota") - 1);
        lua_gettable(L, 2);

        if (lua_isnil(L, -1)) {
            quota = 0;

        } else if (lua_isnumber(L, -1)) {
            quota = lua_tonumber(L, -1);

        } else {
            return luaL_error(L, "ts.shared.DICT: 'quota' is invalid");
        }

        lua_pop(L, 1);

        /* option */
        lua_pushlstring(L, "options", sizeof("options") - 1);
        lua_gettable(L, 2);
        
        if (lua_isstring(L, -1)) {
            opt = luaL_checklstring(L, -1, &opt_len);
            i = 0;

            while (i < opt_len) {
                c = *(opt + i);

                switch (c) {

                    case 'i':
                        option |= TS_LUA_SHDICT_FLAG_INTKEY;
                        break;

                    case 's':
                        option |= TS_LUA_SHDICT_FLAG_STATIC;
                        break;

                    default:
                        break;
                }

                i++;
            }
        }
    }

    dct = &(conf->shdict[conf->shdict_n++]);

    /* basic set */
    snprintf(dct->name, TS_LUA_MAX_SHARED_DICT_NAME_LENGTH - 4, "%.*s", (int)name_len, name);
    dct->quota = quota;
    dct->flags = option;

    /* init hashtable */
    if (option & TS_LUA_SHDICT_FLAG_INTKEY) {
        Tcl_InitHashTable(&(dct->map.t), TCL_ONE_WORD_KEYS);

    } else {
        Tcl_InitHashTable(&(dct->map.t), TCL_STRING_KEYS);
    }

    dct->map.mutexp = TSMutexCreate();

    /* return to lua */
    lua_pushlightuserdata(L, dct);
    ts_lua_shared_dict_set_metatable(L);

    return 1;
}

static int
ts_lua_shared_dict_set_metatable(lua_State *L)
{
    lua_newtable(L);

    lua_pushcfunction(L, ts_lua_shared_dict_init);
    lua_setfield(L, -2, "init");

    lua_pushcfunction(L, ts_lua_shared_dict_set);
    lua_setfield(L, -2, "set");

    lua_pushcfunction(L, ts_lua_shared_dict_get);
    lua_setfield(L, -2, "get");

    lua_pushcfunction(L, ts_lua_shared_dict_del);
    lua_setfield(L, -2, "del");

    lua_pushcfunction(L, ts_lua_shared_dict_get_keys);
    lua_setfield(L, -2, "get_keys");

    lua_pushcfunction(L, ts_lua_shared_dict_get_size);
    lua_setfield(L, -2, "get_size");

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_setmetatable(L, -2);

    return 0;
}

static int
ts_lua_shared_dict_init(lua_State *L)
{
    int                     n;
    ts_lua_shared_dict      *dct;

    n = lua_gettop(L);

    if (n != 2) {
        return luaL_error(L, "invalid param for xx:init(func)");
    }

    dct = (ts_lua_shared_dict*)lua_touserdata(L, 1);
    if (dct == NULL) {
        return luaL_error(L, "userdata is required for xx:init(func)");
    }

    if (dct->initialized) {             // initialized already
        lua_pushnumber(L, 0);
        return 1;
    }

    if (!lua_isfunction(L, 2)) {
        return luaL_error(L, "function is required for xx:init(func)");
    }

    lua_pushvalue(L, 2);        // push a copy of the function
    lua_pushvalue(L, 1);        // push a copy of the userdata

    if (lua_pcall(L, 1, 1, 0)) {
        return luaL_error(L, "lua_pcall failed inside xx:init(func): %s", lua_tostring(L, -1));
    }

    dct->initialized = 1;

    return 1;
}

static int
ts_lua_shared_dict_set(lua_State *L)
{
    int                     n;
    int64_t                 nkey, nval;
    const char              *key, *val;
    size_t                  key_len, val_len;
    int                     ktype, vtype;
    void                    *hKey;
    int                     isNew;
    Tcl_HashEntry           *hPtr;
    ts_lua_shared_dict_item *item, *old_item;
    ts_lua_shared_dict      *dct;

    n = lua_gettop(L);

    if (n != 3) {
        return luaL_error(L, "invalid param for xx:set(key, value)");
    }

    dct = (ts_lua_shared_dict*)lua_touserdata(L, 1);
    if (dct == NULL) {
        return luaL_error(L, "userdata is required for xx:set()");
    }

    if (dct->quota && dct->used > dct->quota) {
        lua_pushnumber(L, 0);
        lua_pushlstring(L, "no memory", sizeof("no memory") - 1);
        return 2;
    }

    ktype = lua_type(L, 2);
    vtype = lua_type(L, 3);

    switch (ktype) {

        case LUA_TNUMBER:
            if (dct->flags & TS_LUA_SHDICT_FLAG_INTKEY) {
                nkey = luaL_checknumber(L, 2);
                key_len = sizeof(long);
                hKey = (void*)nkey;

            } else {
                key = luaL_checklstring(L, 2, &key_len);
                hKey = (void*)key;
            }

            break;

        case LUA_TSTRING:
            if (dct->flags & TS_LUA_SHDICT_FLAG_INTKEY) {
                return luaL_error(L, "key for xx:set() should be number");
            }

            key = luaL_checklstring(L, 2, &key_len);
            hKey = (void*)key;
            break;

        default:
            return luaL_error(L, "invalid key type: %s for xx:set()", lua_typename(L, ktype));
    }

    if (key_len > TS_LUA_SHDICT_MAX_KEY_LENGTH)
        return luaL_error(L, "key length is too long: %d", (int)key_len);

    val_len = 0;
    switch (vtype) {

        case LUA_TNUMBER:
            nval = lua_tonumber(L, 3);
            break;

        case LUA_TSTRING:
            val = lua_tolstring(L, 3, &val_len);
            break;

        case LUA_TBOOLEAN:
            nval = lua_toboolean(L, 3);
            break;

        case LUA_TNIL:
            nval = 0;
            break;

        default:
            return luaL_error(L, "invalid val type: %s for xx:set()", lua_typename(L, vtype));
    }

    if (val_len > TS_LUA_SHDICT_MAX_VAL_LENGTH)
        return luaL_error(L, "val length is too long: %d", (int)val_len);

    item = (ts_lua_shared_dict_item*)TSmalloc(sizeof(ts_lua_shared_dict_item) + val_len);
    item->vtype = vtype;
    item->ksize = key_len;
    item->vsize = val_len;

    if (vtype == LUA_TSTRING) {
        item->v.s = (char*)(((char*)item) + sizeof(ts_lua_shared_dict_item));
        memcpy(item->v.s, val, val_len);

    } else {
        item->v.n = nval;
    }

    isNew = 0;
    old_item = NULL;

    TSMutexLock(dct->map.mutexp);
    hPtr = Tcl_CreateHashEntry(&(dct->map.t), hKey, &isNew);

    if (isNew) {
        Tcl_SetHashValue(hPtr, item);
        dct->used += sizeof(ts_lua_shared_dict_item) + item->ksize + item->vsize;

    } else {
        old_item = (ts_lua_shared_dict_item*)Tcl_GetHashValue(hPtr);
        Tcl_SetHashValue(hPtr, item);
        dct->used += (int)(item->ksize + item->vsize) - (int)(old_item->ksize + old_item->vsize);
    }

    TSMutexUnlock(dct->map.mutexp);

    if (!isNew && old_item) {
        TSfree(old_item);
    }

    lua_pushnumber(L, 1);
    lua_pushnil(L);

    return 2;
}

static int
ts_lua_shared_dict_get(lua_State *L)
{
    int                     n, type;
    int64_t                 nkey;
    const char              *key;
    void                    *val;
    size_t                  key_len;
    void                    *hKey;
    ts_lua_shared_dict      *dct;
    ts_lua_shared_dict_item *item;

    n = lua_gettop(L);

    if (n != 2) {
        return luaL_error(L, "invalid param for xx:get(key)");
    }

    dct = (ts_lua_shared_dict*)lua_touserdata(L, 1);
    if (dct == NULL) {
        return luaL_error(L, "userdata is required for xx:get()");
    }

    type = lua_type(L, 2);

    if (dct->flags & TS_LUA_SHDICT_FLAG_INTKEY) {

        if (type != LUA_TNUMBER) {
            return luaL_error(L, "key for xx:get() is not number.");
        }

        nkey = lua_tonumber(L, 2);
        hKey = (void*)nkey;

    } else if (type != LUA_TSTRING) {
        return luaL_error(L, "key for xx:get() is not string.");

    } else {
        key = lua_tolstring(L, 2, &key_len);
        hKey = (void*)key;
    }

    if (!(dct->flags & TS_LUA_SHDICT_FLAG_STATIC))
         TSMutexLock(dct->map.mutexp);

    if (ts_lua_hash_table_lookup(&dct->map.t, hKey, &val)) {        // found

        item = (ts_lua_shared_dict_item*)val;
        switch (item->vtype) {

            case LUA_TNUMBER:
                lua_pushnumber(L, item->v.n);
                break;

            case LUA_TBOOLEAN:
                lua_pushboolean(L, item->v.n);
                break;

            case LUA_TSTRING:
                lua_pushlstring(L, item->v.s, item->vsize);
                break;

            case LUA_TNIL:
            default:
                lua_pushnil(L);
        }

    } else {                                                        // not found
        lua_pushnil(L);
    }

    if (!(dct->flags & TS_LUA_SHDICT_FLAG_STATIC))
         TSMutexUnlock(dct->map.mutexp);

    return 1;
}

static int
ts_lua_shared_dict_del(lua_State *L)
{
    int                     n, type;
    int64_t                 nkey;
    const char              *key;
    size_t                  key_len;
    void                    *hKey;
    Tcl_HashEntry           *entry;
    ts_lua_shared_dict_item *item;
    ts_lua_shared_dict      *dct;

    n = lua_gettop(L);

    if (n != 2) {
        return luaL_error(L, "invalid param for xx:del(key)");
    }

    dct = (ts_lua_shared_dict*)lua_touserdata(L, 1);
    if (dct == NULL) {
        return luaL_error(L, "userdata is required for xx:del()");
    }

    type = lua_type(L, 2);

    if (dct->flags & TS_LUA_SHDICT_FLAG_INTKEY) {

        if (type != LUA_TNUMBER) {
            return luaL_error(L, "key for xx:del() is not number.");
        }

        nkey = lua_tonumber(L, 2);
        hKey = (void*)nkey;

    } else if (type != LUA_TSTRING) {
        return luaL_error(L, "key for xx:del() is not string.");

    } else {
        key = lua_tolstring(L, 2, &key_len);
        hKey = (void*)key;
    }

    item = NULL;

    TSMutexLock(dct->map.mutexp);

    entry = ts_lua_hash_table_lookup_entry(&dct->map.t, hKey);

    if (entry) {
        item = ts_lua_hash_table_entry_value(&dct->map.t, entry);
        Tcl_DeleteHashEntry(entry);

        dct->used -= item->ksize + item->vsize + sizeof(ts_lua_shared_dict_item);
    }

    TSMutexUnlock(dct->map.mutexp);

    if (item) {
        TSfree(item);
    }
 
    return 0;
}

static int
ts_lua_shared_dict_get_keys(lua_State *L)
{
    int                         n, max;
    ts_lua_shared_dict          *dct;
    ts_lua_shared_dict_keys_ctx ctx;

    n = lua_gettop(L);

    if (n < 1) {
        return luaL_error(L, "invalid param for xx:get_keys()");
    }

    dct = (ts_lua_shared_dict*)lua_touserdata(L, 1);
    if (dct == NULL) {
        return luaL_error(L, "userdata is required for xx:get_keys()");
    }

    if (n == 2) {
        max = lua_tonumber(L, 2);

    } else {
        max = 0;
    }

    ctx.l = L;
    ctx.dct = dct;
    ctx.max = max;
    ctx.n = 0;

    lua_newtable(L);

    if (!(dct->flags & TS_LUA_SHDICT_FLAG_STATIC))
        TSMutexLock(dct->map.mutexp);

    ts_lua_hash_table_iterate(&dct->map.t, dump_entry_key, &ctx);

    if (!(dct->flags & TS_LUA_SHDICT_FLAG_STATIC))
        TSMutexUnlock(dct->map.mutexp);
 
    return 1;
}

static int
ts_lua_shared_dict_get_size(lua_State *L)
{
    int                     n;
    int64_t                 size;
    ts_lua_shared_dict      *dct;

    n = lua_gettop(L);

    if (n < 1) {
        return luaL_error(L, "invalid param for xx:get_size()");
    }

    dct = (ts_lua_shared_dict*)lua_touserdata(L, 1);
    if (dct == NULL) {
        return luaL_error(L, "userdata is required for xx:get_size()");
    }

    size = dct->used;

    lua_pushnumber(L, size);

    return 1;
}

static int
dump_entry_key(Tcl_HashTable *t, Tcl_HashEntry *entry, void *data)
{
    char                        *key;
    lua_State                   *L;

    ts_lua_shared_dict_keys_ctx *ctx = (ts_lua_shared_dict_keys_ctx*)data;
    L = ctx->l;

    key = ts_lua_hash_table_entry_key(t, entry);

    if (key == NULL)
        return 0;

    lua_pushnumber(L, ++ctx->n);

    if (ctx->dct->flags & TS_LUA_SHDICT_FLAG_INTKEY) {
        lua_pushnumber(L, (int64_t)key);

    } else {
        lua_pushstring(L, key);
    }

    lua_rawset(L, -3);

    if (ctx->max && ctx->n >= ctx->max)
        return 1;

    return 0;
}

int
ts_lua_del_shared_dict(ts_lua_shared_dict *dct)
{
    ts_lua_hash_table_iterate(&dct->map.t, free_entry, NULL);
    Tcl_DeleteHashTable(&dct->map.t);
    return 0;
}

static int
free_entry(Tcl_HashTable *t, Tcl_HashEntry *entry, void *data)
{
    ts_lua_shared_dict_item     *item;

    item = (ts_lua_shared_dict_item*)ts_lua_hash_table_entry_value(t, entry);

    if (item != NULL) {
        TSfree(item);
    }

    return 0;
}

