

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

