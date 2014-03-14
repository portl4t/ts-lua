
#ifndef _TS_LUA_HASH_TABLE_H
#define _TS_LUA_HASH_TABLE_H

#include "ts_lua_common.h"

typedef int (*TclHashEntryFunction) (Tcl_HashTable *ht_ptr, Tcl_HashEntry *entry_ptr, void *data);

void ts_lua_hash_table_iterate(Tcl_HashTable *ht, TclHashEntryFunction func, void *data);
ClientData ts_lua_hash_table_entry_value(Tcl_HashTable *ht_ptr, Tcl_HashEntry *entry_ptr);

char *ts_lua_hash_table_entry_key(Tcl_HashTable * ht_ptr, Tcl_HashEntry * entry_ptr);
Tcl_HashEntry *ts_lua_hash_table_lookup_entry(Tcl_HashTable *ht_ptr, const char *key);

int ts_lua_hash_table_lookup(Tcl_HashTable *ht_ptr, const char* key, ClientData *value_ptr);

#endif

