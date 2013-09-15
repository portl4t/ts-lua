

function do_send_response()
    ts.client_response.header['Rhost'] = ts.ctx['rhost']
    return 0
end


function do_remap()
    req_host = ts.client_request.header.Host

    if req_host == nil then
        return 0
    end

    ts.ctx['rhost'] = string.reverse(req_host)

    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, do_send_response)

    return 0
end

