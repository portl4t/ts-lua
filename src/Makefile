.SUFFIXES: .c .o .lo

COMPILE = $(CC) -std=c99 -Wall -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -lssl -llua -g
INC_PATH = -Imodules -I/usr/local/ats4/include
LIB_PATH = -L/usr/local/ats4/lib

SHARED_OBJS = ts_lua.lo  ts_lua_client_request.lo  ts_lua_client_response.lo ts_lua_server_request.lo ts_lua_server_response.lo \
	      ts_lua_log.lo ts_lua_cached_response.lo ts_lua_util.lo ts_lua_atomic.lo \
	      ts_lua_hook.lo ts_lua_context.lo ts_lua_misc.lo ts_lua_http.lo ts_lua_transform.lo\

ALL_OBJS = $(SHARED_OBJS)

ALL_PRGS = 
ALL_LIBS = libtslua.so

all: $(ALL_OBJS) $(ALL_PRGS) $(ALL_LIBS)

libtslua.so:
	$(COMPILE) -o $@ $< -shared $(SHARED_OBJS) $(LIB_PATH)
.c:
	$(COMPILE) -o $@ $< $(SHARED_OBJS) $(LIB_PATH) $(INC_PATH)
.c.o:
	$(COMPILE) -c -o $@ $<  $(INC_PATH)
.c.lo:
	$(COMPILE) -c -fPIC -o $@ $<  $(INC_PATH)
install:
	/bin/cp -f $(ALL_LIBS)  /usr/local/ats4/libexec/trafficserver/
clean:
	rm -f $(ALL_OBJS) $(ALL_PRGS) $(ALL_LIBS)

