## Name
ts-lua - Embed the Power of Lua into TrafficServer.

## Status
This module is under active development and is production ready.

## Version
This document describes ts-lua [v0.1.6](https://github.com/portl4t/ts-lua/tags) released on 13 March 2015.

## Google group
https://groups.google.com/forum/#!forum/ts-lua

##Synopsis
**hdr.lua**
```lua
function send_response()
    ts.client_response.header['Rhost'] = ts.ctx['rhost']
    return 0
end

function do_remap()
    local req_host = ts.client_request.header.Host
    ts.ctx['rhost'] = string.reverse(req_host)
    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
    return 0
end
```

**sethost.lua**
```lua
local HOSTNAME = ''

function __init__(argtb)
    if (#argtb) < 1 then
        print(argtb[0], 'hostname parameter required!!')
        return -1
    end
    HOSTNAME = argtb[1]
end

function do_remap()
    ts.client_request.header['Host'] = HOSTNAME
    return 0
end
```

## Description
This module embeds Lua, via the standard Lua 5.1 interpreter, into Apache Traffic Server. This module acts as remap plugin of Traffic Server, so we should realize **'do_remap'** function in each lua script. We can write this in remap.config:

>map http://a.foo.com/ http://inner.foo.com/ @plugin=/X/libtslua.so @pparam=/X/hdr.lua

Sometimes we want to receive parameters and process them in the script, we should realize **'\__init__'** function in the lua script(`sethost.lua` is a reference), and we can write this in remap.config:

>map http://a.foo.com/ http://b.foo.com/ @plugin=/X/libtslua.so @pparam=/X/sethost.lua @pparam=a.st.com


## Doc
https://github.com/portl4t/ts-lua/wiki/Doc

## System Requirements
* linux/freebsd 64bits
* trafficserver
* pcre
* tcl
* openssl
* lua-5.1

## Build
**step1**: get ts-lua

    git clone https://github.com/portl4t/ts-lua.git
    cd ts-lua/src

**step2**: modify Makefile to insure INC_PATH and LIB_PATH is right for trafficserver, lua-5.1 and tcl

    make

**step3**: modify remap.config and restart the trafficserver

## ChangeLog
https://github.com/portl4t/ts-lua/wiki/ChangeLog

## History
* [v0.1.6](https://github.com/portl4t/ts-lua/releases/tag/v0.1.6), 2015-03-13
* [v0.1.5](https://github.com/portl4t/ts-lua/releases/tag/v0.1.5), 2014-12-18
* [v0.1.4](https://github.com/portl4t/ts-lua/releases/tag/v0.1.4), 2014-05-02

## See Also
* [ts-fetcher](https://github.com/portl4t/ts-fetcher) http fetcher for ats, implement as plugin with c

