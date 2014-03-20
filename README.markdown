## Name
ts-lua - Embed the Power of Lua into TrafficServer.

## Status
This module is under active development and is production ready.

## Version
This document describes ts-lua [v0.1.0](https://github.com/portl4t/ts-lua/tags) released on 16 March 2014.

## Google group
https://groups.google.com/forum/#!forum/ts-lua

##Synopsis
**test_hdr.lua**

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


## Description
This module embeds Lua, via the standard Lua 5.1 interpreter, into Apache Traffic Server. This module acts as remap plugin of Traffic Server, so we should realize **'do_remap'** function in each lua script. We can write this in remap.config:

> map http://a.tbcdn.cn/ http://inner.tbcdn.cn/ @plugin=/XXX/libtslua.so @pparam=/XXX/test_hdr.lua

Sometimes we want to receive parameters and process them in the script, we should realize **'\__init__'** function in the lua script([sethost.lua](https://github.com/portl4t/ts-lua/blob/master/business/sethost.lua) is a reference), and we can write this in remap.config:

> map http://a.x.cn/ http://b.x.cn/ @plugin=/X/libtslua.so @pparam=/X/sethost.lua @pparam=a.st.cn


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

**step2**: build, requires linux/freebsd (64bits is recommended)

    make

**step3**: modify remap.config and restart the trafficserver



## TODO
* ts.fetch
* ts.cache_xxx

