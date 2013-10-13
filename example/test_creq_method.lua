

function send_response()
    ts.client_response.header['Method'] = ts.ctx['method']
end


function do_remap()
    local method = ts.client_request.get_method()

    if method == nil then
        return 0
    end

    ts.ctx['method'] = method

    if method ~= 'GET' then
        ts.client_request.set_method('GET')
    end

    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)

    return 0
end

