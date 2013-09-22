Name
======

ts-lua - Embed the Power of Lua into TrafficServer

Status
======
This module is being tested under our production environment

Version
======
ts-lua has not been released yet.

Synopsis
======

[test_hdr.lua]

    function send_response()
        ts.client_response.header['Rhost'] = ts.ctx['rhost']
        return 0
    end


    function do_remap()
        req_host = ts.client_request.header.Host

        if req_host == nil then
            return 0
        end

        ts.ctx['rhost'] = string.reverse(req_host)

        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)

        return 0
    end



[test_transform.lua]

    function upper_transform(data, eos)
        if eos == 1 then
            return string.upper(data)..'S.H.E.\n', eos
        else
            return string.upper(data), eos
        end
    end

    function send_response()
        ts.client_response.header['SHE'] = ts.ctx['tb']['she']
        return 0
    end


    function do_remap()
        req_host = ts.client_request.header.Host

        if req_host == nil then
            return 0
        end

        ts.ctx['tb'] = {}
        ts.ctx['tb']['she'] = 'wo ai yu ye hua'

        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
        ts.hook(TS_LUA_RESPONSE_TRANSFORM, upper_transform)

        ts.http.resp_cache_transformed(0)
        ts.http.resp_cache_untransformed(1)
        return 0
    end



[test_cache_lookup.lua]

    function send_response()
        ts.client_response.header['Rhost'] = ts.ctx['rhost']
        ts.client_response.header['CStatus'] = ts.ctx['cstatus']
    end


    function cache_lookup()
        cache_status = ts.http.get_cache_lookup_status()
        ts.ctx['cstatus'] = cache_status
    end


    function do_remap()
        req_host = ts.client_request.header.Host

        if req_host == nil then
            return 0
        end

        ts.ctx['rhost'] = string.reverse(req_host)

        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
        ts.hook(TS_LUA_HOOK_CACHE_LOOKUP_COMPLETE, cache_lookup)

        return 0
    end



[test_ret_403.lua]

    function send_response()
        ts.client_response.header['Now'] = ts.now()
        return 0
    end


    function do_remap()

        uri = ts.client_request.get_uri()

        pos, len = string.find(uri, '/css/')
        if pos ~= nil then
            ts.http.set_resp(403, "Document access failed :)\n")
            return 0
        end

        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)

        return 0
    end



[sethost.lua]

    HOSTNAME = ''

    function __init__(argtb)

        if (#argtb) < 1 then
            print(argtb[0], 'hostname parameter required!!')
            return -1
        end

        HOSTNAME = argtb[1]
    end

    function do_remap()
        req_host = ts.client_request.header.Host

        if req_host == nil then
            return 0
        end

        ts.client_request.header['Host'] = HOSTNAME

        return 0
    end


Description
======
This module embeds Lua, via the standard Lua 5.1 interpreter or LuaJIT 2.0, into Apache Traffic Server. This module acts as remap plugin of Traffic Server, so we should realize 'do_remap' function in each lua script. We can write this in remap.config:

map http://a.tbcdn.cn/ http://inner.tbcdn.cn/ @plugin=/usr/lib64/trafficserver/plugins/libtslua.so @pparam=/etc/trafficserver/script/test_hdr.lua

Sometimes we want to receive parameters and process them in the script, we should realize '__init__' function in the lua script, and we can write this in remap.config:

map http://a.tbcdn.cn/ http://inner.tbcdn.cn/ @plugin=/usr/lib64/trafficserver/plugins/libtslua.so @pparam=/etc/trafficserver/script/sethost.lua @pparam=img03.tbcdn.cn



TS API for Lua
  Introduction
    The API is exposed to Lua in the form of one standard packages ts. This package is in the default global scope and is always available within lua script.

  ts.now
    syntax: *val = ts.now()*

    context: global

    This function returns the time since the Epoch (00:00:00 UTC, January 1, 1970), measured in seconds.

    Here is an example:

        function send_response()
           ts.client_response.header['Now'] = ts.now()
           return 0
        end

  ts.debug
    syntax: *ts.debug(MESSAGE)*

    context: global

    Log the MESSAGE to traffic.out if debug is enabled.

    Here is an example:

        function do_remap()
           ts.debug('I am in do_remap now.')
           return 0
        end

  ts.hook

