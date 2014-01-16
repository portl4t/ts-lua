
#include "ts_lua_hash_table.h"

static Tcl_HashEntry * ts_lua_hash_table_lookup_entry(Tcl_HashTable *ht_ptr, const char *key);

static Tcl_HashEntry *
ts_lua_hash_table_iterator_first(Tcl_HashTable *ht_ptr, Tcl_HashSearch * state_ptr)
{
    Tcl_HashTable *tcl_ht_ptr;
    Tcl_HashSearch *tcl_search_state_ptr;
    Tcl_HashEntry *tcl_he_ptr;
    Tcl_HashEntry *he_ptr;

    tcl_ht_ptr = (Tcl_HashTable *) ht_ptr;
    tcl_search_state_ptr = (Tcl_HashSearch *) state_ptr;

    tcl_he_ptr = Tcl_FirstHashEntry(tcl_ht_ptr, tcl_search_state_ptr);
    he_ptr = tcl_he_ptr;

    return he_ptr;
}

static Tcl_HashEntry * 
ts_lua_hash_table_iterator_next(Tcl_HashTable *ht_ptr, Tcl_HashSearch * state_ptr)
{
    Tcl_HashSearch *tcl_search_state_ptr;
    Tcl_HashEntry *tcl_he_ptr;
    Tcl_HashEntry *he_ptr;

    tcl_search_state_ptr = (Tcl_HashSearch *) state_ptr;

    tcl_he_ptr = Tcl_NextHashEntry(tcl_search_state_ptr);
    he_ptr = (Tcl_HashEntry*) tcl_he_ptr;

    return he_ptr;
}

char *
ts_lua_hash_table_entry_key(Tcl_HashTable * ht_ptr, Tcl_HashEntry * entry_ptr)
{
    char *tcl_key;

    tcl_key = (char*) Tcl_GetHashKey((Tcl_HashTable *) ht_ptr, (Tcl_HashEntry *) entry_ptr);
    return tcl_key;
}  

ClientData
ts_lua_hash_table_entry_value(Tcl_HashTable *ht_ptr, Tcl_HashEntry *entry_ptr)
{
    ClientData tcl_value;

    tcl_value = Tcl_GetHashValue((Tcl_HashEntry *) entry_ptr);
    return tcl_value;
}

void
ts_lua_hash_table_iterate(Tcl_HashTable *ht_ptr, TclHashEntryFunction func)
{
    int retcode;
    Tcl_HashEntry *e;
    Tcl_HashSearch state;

    for (e = ts_lua_hash_table_iterator_first(ht_ptr, &state); e != NULL; e = ts_lua_hash_table_iterator_next(ht_ptr, &state)) {
        retcode = (*func) (ht_ptr, e);
        if (retcode != 0)
            break;
    }
}

int
ts_lua_hash_table_lookup(Tcl_HashTable *ht_ptr, const char* key, ClientData *value_ptr)
{
    Tcl_HashEntry   *he_ptr;
    ClientData      value;

    he_ptr = ts_lua_hash_table_lookup_entry(ht_ptr, key);
    if (he_ptr == NULL)
        return 0;

    value = ts_lua_hash_table_entry_value(ht_ptr, he_ptr);
    *value_ptr = value;
    return 1;
}

static Tcl_HashEntry *
ts_lua_hash_table_lookup_entry(Tcl_HashTable *ht_ptr, const char *key)
{
    Tcl_HashTable *tcl_ht_ptr;
    Tcl_HashEntry *tcl_he_ptr;    
    Tcl_HashEntry *he_ptr;

    tcl_ht_ptr = (Tcl_HashTable *) ht_ptr;
    tcl_he_ptr = Tcl_FindHashEntry(tcl_ht_ptr, key);
    he_ptr = tcl_he_ptr;

    return he_ptr;
}

