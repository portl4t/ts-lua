Name
======

ts-lua - Embed the Power of Lua into TrafficServer.

Status
======
This module is under active development and is production ready.

Version
======
ts-lua has not been released yet.

Synopsis
======

**test_hdr.lua**

    function send_response()
        ts.client_response.header['Rhost'] = ts.ctx['rhost']
        return 0
    end


    function do_remap()
        local req_host = ts.client_request.header.Host

        if req_host == nil then
            return 0
        end

        ts.ctx['rhost'] = string.reverse(req_host)

        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)

        return 0
    end



**test_transform.lua**

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
        local req_host = ts.client_request.header.Host

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



**test_cache_lookup.lua**

    function send_response()
        ts.client_response.header['Rhost'] = ts.ctx['rhost']
        ts.client_response.header['CStatus'] = ts.ctx['cstatus']
    end


    function cache_lookup()
        local cache_status = ts.http.get_cache_lookup_status()
        ts.ctx['cstatus'] = cache_status
    end


    function do_remap()
        local req_host = ts.client_request.header.Host

        if req_host == nil then
            return 0
        end

        ts.ctx['rhost'] = string.reverse(req_host)

        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
        ts.hook(TS_LUA_HOOK_CACHE_LOOKUP_COMPLETE, cache_lookup)

        return 0
    end



**test_ret_403.lua**

    function send_response()
        ts.client_response.header['Now'] = ts.now()
        return 0
    end


    function do_remap()

        local uri = ts.client_request.get_uri()

        pos, len = string.find(uri, '/css/')
        if pos ~= nil then
            ts.http.set_resp(403, "Document access failed :)\n")
            return 0
        end

        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)

        return 0
    end



**sethost.lua**

    HOSTNAME = ''

    function __init__(argtb)

        if (#argtb) < 1 then
            print(argtb[0], 'hostname parameter required!!')
            return -1
        end

        HOSTNAME = argtb[1]
    end

    function do_remap()
        local req_host = ts.client_request.header.Host

        if req_host == nil then
            return 0
        end

        ts.client_request.header['Host'] = HOSTNAME

        return 0
    end


**test_intercept.lua**

    require 'os'

    function send_data()
        local nt = os.time()..' Zheng.\n'
        local resp =  'HTTP/1.1 200 OK\r\n' ..
            'Server: ATS/3.2.0\r\n' ..
            'Content-Type: text/plain\r\n' ..
            'Content-Length: ' .. string.len(nt) .. '\r\n' ..
            'Last-Modified: ' .. os.date("%a, %d %b %Y %H:%M:%S GMT", os.time()) .. '\r\n' ..
            'Connection: keep-alive\r\n' ..
            'Cache-Control: max-age=7200\r\n' ..
            'Accept-Ranges: bytes\r\n\r\n' ..
            nt

        ts.sleep(1)
        ts.say(resp)
        ts.sleep(1)
        ts.say('~~ finish ~~\n')
    end

    function do_remap()
        ts.http.intercept(send_data)
        return 0
    end


**test_server_intercept.lua**

    require 'os'

    function send_data()
        local nt = os.time()..'\n'
        local resp =  'HTTP/1.1 200 OK\r\n' ..
            'Server: ATS/3.2.0\r\n' ..
            'Content-Type: text/plain\r\n' ..
            'Content-Length: ' .. string.len(nt) .. '\r\n' ..
            'Last-Modified: ' .. os.date("%a, %d %b %Y %H:%M:%S GMT", os.time()) .. '\r\n' ..
            'Connection: keep-alive\r\n' ..
            'Cache-Control: max-age=30\r\n' ..
            'Accept-Ranges: bytes\r\n\r\n' ..
            nt

        ts.say(resp)
    end

    function do_remap()
        ts.http.server_intercept(send_data)
        return 0
    end


Description
======
This module embeds Lua, via the standard Lua 5.1 interpreter, into Apache Traffic Server. This module acts as remap plugin of Traffic Server, so we should realize **'do_remap'** function in each lua script. We can write this in remap.config:

map http://a.tbcdn.cn/ http://inner.tbcdn.cn/ @plugin=/usr/lib64/trafficserver/plugins/libtslua.so @pparam=/etc/trafficserver/script/test_hdr.lua

Sometimes we want to receive parameters and process them in the script, we should realize **'\__init__'** function in the lua script(sethost.lua is a reference), and we can write this in remap.config:

map http://a.tbcdn.cn/ http://inner.tbcdn.cn/ @plugin=/usr/lib64/trafficserver/plugins/libtslua.so @pparam=/etc/trafficserver/script/sethost.lua @pparam=img03.tbcdn.cn



TS API for Lua
======
Introduction
------
The API is exposed to Lua in the form of one standard packages ts. This package is in the default global scope and is always available within lua script.


Remap status constants
------
**context**: do_remap

    TS_LUA_REMAP_NO_REMAP (0)
    TS_LUA_REMAP_DID_REMAP (1)
    TS_LUA_REMAP_NO_REMAP_STOP (2)
    TS_LUA_REMAP_DID_REMAP_STOP (3)
    TS_LUA_REMAP_ERROR (-1)
    
These constants are usually used as return value of do_remap.


ts.now
------
**syntax**: *val = ts.now()*

**context**: global

**description**: This function returns the time since the Epoch (00:00:00 UTC, January 1, 1970), measured in seconds.

Here is an example:

    function send_response()
        ts.client_response.header['Now'] = ts.now()
        return 0
    end


ts.debug
------
**syntax**: *ts.debug(MESSAGE)*

**context**: global

**description**: Log the MESSAGE to traffic.out if debug is enabled.

Here is an example:

    function do_remap()
       ts.debug('I am in do_remap now.')
       return 0
    end
    
The debug tag is ts_lua and we should write this in records.config:
    
    CONFIG proxy.config.diags.debug.tags STRING ts_lua
    

ts.hook
------
**syntax**: *ts.hook(HOOK_POINT, FUNCTION)*

**context**: do_remap or later

**description**: Hooks are points in http transaction processing where we can step in and do some work.
FUNCTION will be called when the http transaction steps in to HOOK_POINT.

Here is an example:

    function send_response()
        s.client_response.header['SHE'] = 'belief'
    end
    
    function do_remap()
        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
    end

Hook point constants
------
**context**: do_remap or later

    TS_LUA_HOOK_CACHE_LOOKUP_COMPLETE
    TS_LUA_HOOK_SEND_REQUEST_HDR
    TS_LUA_HOOK_READ_RESPONSE_HDR
    TS_LUA_HOOK_SEND_RESPONSE_HDR
    TS_LUA_REQUEST_TRANSFORM
    TS_LUA_RESPONSE_TRANSFORM
    
These constants are usually used in ts.hook method call.


ts.ctx
------
**syntax**: *ts.ctx[KEY]*

**context**: do_remap or later

**description**: This table can be used to store per-request Lua context data and has a life time identical to the current request.

Here is an example:

    function send_response()
        ts.client_response.header['RR'] = ts.ctx['rhost']
        return 0
    end
    
    function do_remap()
        local req_host = ts.client_request.header.Host
        ts.ctx['rhost'] = string.reverse(req_host)
        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
        return 0
    end


ts.http.get_cache_lookup_status
------
**syntax**: *ts.http.get_cache_lookup_status()*

**context**: function @ TS_LUA_HOOK_CACHE_LOOKUP_COMPLETE hook point

**description**: This function can be used to get cache lookup status.

Here is an example:

    function send_response()
        ts.client_response.header['CStatus'] = ts.ctx['cstatus']
    end
    
    function cache_lookup()
        local cache_status = ts.http.get_cache_lookup_status()
        if cache_status == TS_LUA_CACHE_LOOKUP_HIT_FRESH:
            ts.ctx['cstatus'] = 'hit'
        else
            ts.ctx['cstatus'] = 'not hit'
        end
    end
    
    function do_remap()
        ts.hook(TS_LUA_HOOK_CACHE_LOOKUP_COMPLETE, cache_lookup)
        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
        return 0
    end


Http cache lookup status constants
------
**context**: global

    TS_LUA_CACHE_LOOKUP_MISS (0)
    TS_LUA_CACHE_LOOKUP_HIT_STALE (1)
    TS_LUA_CACHE_LOOKUP_HIT_FRESH (2)
    TS_LUA_CACHE_LOOKUP_SKIPPED (3)


ts.http.set_cache_url
------
**syntax**: *ts.http.set_cache_url(KEY_URL)*

**context**: do_remap

**description**: This function can be used to modify the cache key for the request.

Here is an example:

    function do_remap()
        ts.http.set_cache_url('http://127.0.0.1:8080/abc/')
        return 0
    end


ts.http.resp_cache_transformed
------
**syntax**: *ts.http.resp_cache_transformed(BOOL)*

**context**: do_remap or later

**description**: This function can be used to tell trafficserver whether to cache the transformed data.

Here is an example:

    function upper_transform(data, eos)
        if eos == 1 then
            return string.upper(data)..'S.H.E.\n', eos
        else
            return string.upper(data), eos
        end
    end
    
    function do_remap()
        ts.hook(TS_LUA_RESPONSE_TRANSFORM, upper_transform)
        ts.http.resp_cache_transformed(0)
        ts.http.resp_cache_untransformed(1)
        return 0
    end
    
This function is usually called after we hook TS_LUA_RESPONSE_TRANSFORM.


ts.http.resp_cache_untransformed
------
**syntax**: *ts.http.resp_cache_untransformed(BOOL)*

**context**: do_remap or later

**description**: This function can be used to tell trafficserver whether to cache the untransformed data.

Here is an example:

    function upper_transform(data, eos)
        if eos == 1 then
            return string.upper(data)..'S.H.E.\n', eos
        else
            return string.upper(data), eos
        end
    end
    
    function do_remap()
        ts.hook(TS_LUA_RESPONSE_TRANSFORM, upper_transform)
        ts.http.resp_cache_transformed(0)
        ts.http.resp_cache_untransformed(1)
        return 0
    end
    
This function is usually called after we hook TS_LUA_RESPONSE_TRANSFORM.

ts.http.set_resp
------

ts.http.redirect
------


ts.client_request.client_addr.get_addr
------
**syntax**: *ts.client_request.client_addr.get_addr()*

**context**: do_remap or later

**description**: This function can be used to get socket address of the client.

Here is an example:

    function do_remap
        ip, port, family = ts.client_request.client_addr.get_addr()
        return 0
    end

The ts.client_request.client_addr.get_addr function returns three values, ip is a string, port and family is number.


ts.client_request.get_method
------
**syntax**: *ts.client_request.get_method()*

**context**: do_remap or later

**description**: This function can be used to retrieve the current request's request method name. String like "GET" or 
"POST" is returned.


ts.client_request.set_method
------
**syntax**: *ts.client_request.set_method(METHOD_NAME)*

**context**: do_remap

**description**: This function can be used to override the current request's request method with METHOD_NAME.


ts.client_request.get_url
------
**syntax**: *ts.client_request.get_url()*

**context**: do_remap or later

**description**: This function can be used to retrieve the whole request's url.


ts.client_request.get_uri
------
**syntax**: *ts.client_request.get_uri()*

**context**: do_remap or later

**description**: This function can be used to retrieve the request's path.


ts.client_request.set_uri
------
**syntax**: *ts.client_request.set_uri(PATH)*

**context**: do_remap

**description**: This function can be used to override the request's path.


ts.client_request.get_uri_args
------
**syntax**: *ts.client_request.get_uri_args()*

**context**: do_remap or later

**description**: This function can be used to retrieve the request's query string.


ts.client_request.set_uri_args
------
**syntax**: *ts.client_request.set_uri_args(QUERY_STRING)*

**context**: do_remap

**description**: This function can be used to override the request's query string.


ts.client_request.header.HEADER
------
**syntax**: *ts.client_request.header.HEADER = VALUE*

**syntax**: *ts.client_request.header[HEADER] = VALUE*

**syntax**: *VALUE = ts.client_request.header.HEADER*

**context**: do_remap or later

**description**: Set, add to, clear or get the current request's HEADER.

Here is an example:

    function do_remap()
        local req_host = ts.client_request.header.Host
        ts.client_request.header['Host'] = 'a.tbcdn.cn'
    end

ts.client_request.get_url_host
------
**syntax**: *host = ts.client_request.get_url_host()*

**context**: do_remap or later

**description**: Return the `host` field of the request url.

Here is an example:

    function do_remap()
        local url_host = ts.client_request.get_url_host()
        local url_port = ts.client_request.get_url_port()
        local url_scheme = ts.client_request.get_url_scheme()
        print(url_host)
        print(url_port)
        print(url_scheme)
    end

Request like this:

    GET /liuyurou.txt HTTP/1.1
    User-Agent: curl/7.19.7 (x86_64-redhat-linux-gnu) libcurl/7.19.7
    Host: 192.168.231.129:8080
    Accept: */*
    ...

yields the output:

    192.168.231.129
    8080
    http


ts.client_request.set_url_host
------
**syntax**: *ts.client_request.set_url_host(str)*

**context**: do_remap

**description**: Set `host` field of the request url with `str`. This function is used to change the address of the origin server, and we should return TS_LUA_REMAP_DID_REMAP(_STOP) in do_remap.

Here is an example:

    function do_remap()
        ts.client_request.set_url_host('192.168.231.130')
        ts.client_request.set_url_port('80')
        ts.client_request.set_url_scheme('http')
        return TS_LUA_REMAP_DID_REMAP
    end

remap like this:

    map http://192.168.231.129:8080/ http://192.168.231.129:9999/

client request like this:

    GET /liuyurou.txt HTTP/1.1
    User-Agent: curl/7.19.7 (x86_64-redhat-linux-gnu) libcurl/7.19.7
    Host: 192.168.231.129:8080
    Accept: */*
    ...

server request will connect to 192.168.231.130:80, header like this:

    GET /liuyurou.txt HTTP/1.1
    User-Agent: curl/7.19.7 (x86_64-redhat-linux-gnu) libcurl/7.19.7
    Host: 192.168.231.129:8080
    Accept: */*
    ...

ts.client_request.get_url_port
------
**syntax**: *port = ts.client_request.get_url_port()*

**context**: do_remap or later

**description**: Return the `port` field of the request url.

See the example of ts.client_request.get_url_host


ts.client_request.set_url_port
------
**syntax**: *ts.client_request.set_url_port(str)*

**context**: do_remap

**description**: Set `port` field of the request url with `str`. This function is used to change the address of the origin server, and we should return TS_LUA_REMAP_DID_REMAP(_STOP) in do_remap.

See the example of ts.client_request.set_url_host


ts.client_request.get_url_scheme
------
**syntax**: *scheme = ts.client_request.get_url_scheme()*

**context**: do_remap or later

**description**: Return the `scheme` field of the request url.

See the example of ts.client_request.get_url_host


ts.client_request.set_url_scheme
------
**syntax**: *ts.client_request.set_url_scheme(str)*

**context**: do_remap

**description**: Set `scheme` field of the request url with `str`. This function is used to change the scheme of the server request, and we should return TS_LUA_REMAP_DID_REMAP(_STOP) in do_remap.

See the example of ts.client_request.set_url_host


ts.client_request.get_version
------
**syntax**: *ver = ts.client_request.get_version()*

**context**: do_remap or later

**description**: Return the http version of the client request.

Here is an example:

    function do_remap():
        print(ts.client_request.get_version())
    end

Request like this:

    GET /liuyurou.txt HTTP/1.1
    User-Agent: curl/7.19.7 (x86_64-redhat-linux-gnu) libcurl/7.19.7
    Host: 192.168.231.129:8080
    Accept: */*
    ...

yields the output:

    1.1


ts.client_request.set_version
------
**syntax**: *ts.client_request.set_version(str)*

**context**: do_remap or later

**description**: Return the http version of the client request.

Here is an example:

    function do_remap():
        ts.client_request.set_version('1.0')
    end


ts.server_request.header.HEADER
------
**syntax**: *ts.server_request.header.HEADER = VALUE*

**syntax**: *ts.server_request.header[HEADER] = VALUE*

**syntax**: *VALUE = ts.server_request.header.HEADER*

**context**: TS_LUA_HOOK_SEND_REQUEST_HDR hook point

**description**: Set, add to, clear or get the server request's HEADER.

Here is an example:

    function do_remap()
        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR,
                    function()
                        ts.server_request.header['song'] = 'YuZhouXiaoJie'
                    end)
    end


ts.server_request.get_uri
------
ts.server_request.set_uri
------
ts.server_request.get_uri_args
------
ts.server_request.set_uri_args
------


ts.server_response.header.HEADER
------
**syntax**: *ts.server_response.header.HEADER = VALUE*

**syntax**: *ts.server_response.header[HEADER] = VALUE*

**syntax**: *VALUE = ts.server_response.header.HEADER*

**context**: TS_LUA_HOOK_READ_RESPONSE_HDR hook point

**description**: Set, add to, clear or get the server response's HEADER.

Here is an example:

    function do_remap()
        ts.hook(TS_LUA_HOOK_READ_RESPONSE_HDR,
                    function()
                        ts.server_response.header['Singer'] = 'SHE'
                    end)
    end


ts.server_response.set_status
------
ts.server_response.get_status
------
ts.server_response.set_version
------
ts.server_response.get_version
------


ts.client_response.header.HEADER
------
**syntax**: *ts.client_response.header.HEADER = VALUE*

**syntax**: *ts.client_response.header[HEADER] = VALUE*

**syntax**: *VALUE = ts.client_response.header.HEADER*

**context**: TS_LUA_HOOK_SEND_RESPONSE_HDR hook point

**description**: Set, add to, clear or get the client response's HEADER.

Here is an example:

    function do_remap()
        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR,
                    function()
                        ts.client_response.header['cop'] = 'Fast'
                    end)
    end


ts.client_response.get_status
------
ts.client_response.set_status
------
ts.client_response.get_version
------
ts.client_response.set_version
------
ts.client_response.set_error_resp
------

ts.cached_response.header.HEADER
------
**syntax**: *VALUE = ts.cached_response.header.HEADER*

**context**: TS_LUA_HOOK_CACHE_LOOKUP_COMPLETE hook point

**description**: Get the cached response's HEADER.

Here is an example:

    function do_remap()
        ts.hook(TS_LUA_HOOK_CACHE_LOOKUP_COMPLETE,
                    function()
                        local cache_status = ts.http.get_cache_lookup_status()
                        if cached_status == TS_LUA_CACHE_LOOKUP_HIT_FRESH:
                            print(ts.cached_response.header['Content-Type'])
                    end)
    end


ts.cached_response.get_status
------
ts.cached_response.get_version
------

ts.http.intercept
------

ts.http.server_intercept
------

ts.md5
------
**syntax**: *digest = ts.md5(str)*

**context**: global

**description**: Returns the hexadecimal representation of the MD5 digest of the `str` argument.

Here is an example:

    function do_remap()
        uri = ts.client_request.get_uri()
        print(ts.md5(uri))
    end


ts.md5_bin
------
**syntax**: *digest = ts.md5_bin(str)*

**context**: global

**description**: Returns the binary form of the MD5 digest of the `str` argument.

Here is an example:

    function do_remap()
        uri = ts.client_request.get_uri()
        print(ts.md5_bin(uri))
    end



ts.re.match
------
**syntax**: *captures = ts.re.match(subject, regex, options?)*

**context**: *do_remap or later*

**description**: Matches a compiled regular expression `regex` against a given `subject` string. 
When a match is found, a Lua table `captures` is returned. If no match is found, `nil` will be returned.

Here is an example:

    local captures = ts.re.match('<name>ZhanNaiZhen</name>', '<(.*)>(.*)<(.*)>')
    if captures then
        -- captures[0] == "name"
        -- captures[1] == "ZhanNaiZhen"
        -- captures[2] == "/name"
    end

Specify "options" to control how the match operation will be performed. The following option characters are supported:

    a             anchored mode
    
    i             Do caseless matching
    
    m             multi-line mode (^ and $ match newlines within data)
    
    u             Invert greediness of quantifiers
    
    s             . matches anything including NL
    
    x             Ignore white space and # comments
    
    d             $ not to match newline at end




TODO
=======
Short Term
------
* non-blocking I/O operation
* ts.fetch

Long Term
------
* ts.socket

