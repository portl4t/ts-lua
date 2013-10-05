

function post_transform(data, eos)
    local already = ts.ctx['body']
    ts.ctx['body'] = already..string.upper(data)
    return data, eos
end

function send_response()
    ts.client_response.header['Method'] = ts.ctx['method']
    if ts.ctx['body'] ~= nil then
        ts.client_response.header['Body'] = ts.ctx['body']
    end
    return 0
end


function do_remap()
    local req_method = ts.client_request.get_method()

    ts.ctx['method'] = req_method
    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)

    if req_method ~= 'POST' then
        return 0
    end

    ts.ctx['body'] = ''
    ts.hook(TS_LUA_REQUEST_TRANSFORM, post_transform)

    return 0
end

