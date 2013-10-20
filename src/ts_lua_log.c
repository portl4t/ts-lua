#include "ts_lua_util.h"

static TSTextLogObject log;

static int ts_lua_log_object_creat(lua_State *L);
static int ts_lua_log_object_write(lua_State *L);
static int ts_lua_log_object_destroy(lua_State *L);


static void ts_lua_inject_log_object_creat_api(lua_State *L);
static void ts_lua_inject_log_object_write_api(lua_State *L);
static void ts_lua_inject_log_object_destroy_api(lua_State *L);

void
ts_lua_inject_log_api(lua_State *L)
{
    lua_newtable(L);

    ts_lua_inject_log_object_creat_api(L);
	ts_lua_inject_log_object_write_api(L);
	ts_lua_inject_log_object_destroy_api(L);

    lua_setfield(L, -2, "log");
}

static void
ts_lua_inject_log_object_creat_api(lua_State * L)
{
  lua_pushcfunction(L, ts_lua_log_object_creat);
  lua_setfield(L, -2, "object_creat");
}

static int 
ts_lua_log_object_creat(lua_State *L)
{
     const char *log_name;
     size_t name_len;
	 int log_mode;
 	 TSReturnCode error;

     ts_lua_http_ctx  *http_ctx;

     http_ctx = ts_lua_get_http_ctx(L);

     log_name = luaL_checklstring(L, -2, &name_len);

     if (lua_isnil(L, 3)) {
        TSError("no log name!!");
        return -1;
     } else {
        log_mode = luaL_checknumber(L, 3);
 
     }
  
     error = TSTextLogObjectCreate(log_name, log_mode, &log);

     if(!log || error == TS_ERROR)
     {
       TSError("creat log error <%s>",log_name);
       return -1;
     }
      return 0;
}

static void 
ts_lua_inject_log_object_write_api(lua_State * L)
{
    lua_pushcfunction(L, ts_lua_log_object_write);
    lua_setfield(L,-2,"object_write");
}

static int
ts_lua_log_object_write(lua_State *L)
{
    const char  *text;
    size_t      text_len;

    ts_lua_http_ctx  *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    text = luaL_checklstring(L, 1, &text_len);
	if(log){
        TSTextLogObjectWrite(log,text);
	}else{
	  TSError("[%s] log is not exsited!",__FUNCTION__);
	}
		
	return 0;
}

static void 
ts_lua_inject_log_object_destroy_api(lua_State * L)
{
    lua_pushcfunction(L, ts_lua_log_object_destroy);
    lua_setfield(L,-2,"object_destroy");
}

static int
ts_lua_log_object_destroy(lua_State *L)
{
    ts_lua_http_ctx *http_ctx;

	http_ctx = ts_lua_get_http_ctx(L);

	if(TSTextLogObjectDestroy(log) != TS_SUCCESS)
		TSError("[%s] TSTextLogObjectDestroy error!",__FUNCTION__);

	return 0;
}
