

function send_response()
    ts.client_response.header['Uri'] = ts.ctx['uri']
end


function do_remap()
    local uri = ts.client_request.get_uri()

    if uri == nil then
        return 0
    end

    ts.ctx['uri'] = uri

    if uri == '/st' then
        ts.client_request.set_uri('/echo/123456')
    end

    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)

    return 0
end

