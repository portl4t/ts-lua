
#ifndef _TS_LUA_ATOMIC_H
#define _TS_LUA_ATOMIC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct
{
    volatile int64_t head;
    const char *name;
    unsigned int offset;
} ts_lua_atomiclist;


void ts_lua_atomiclist_init(ts_lua_atomiclist * l, const char *name, uint32_t offset_to_next);
void *ts_lua_atomiclist_push(ts_lua_atomiclist * l, void *item);
void *ts_lua_atomiclist_popall(ts_lua_atomiclist * l);

static inline int ts_lua_atomic_increment(volatile int32_t *mem, int value) { return __sync_fetch_and_add(mem, value); }

#endif

