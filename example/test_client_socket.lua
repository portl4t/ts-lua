

function send_response()
    ts.client_response.header['CIP'] = ts.ctx['ip']
    ts.client_response.header['CPort'] = ts.ctx['port']
    ts.client_response.header['Now'] = ts.now()
    return 0
end


function do_remap()
    req_host = ts.client_request.header.Host

    if req_host == nil then
        return 0
    end

    ts.ctx['ip'] = ts.client_request.client_addr.get_ip()
    ts.ctx['port'] = ts.client_request.client_addr.get_port()

    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)

    return 0
end

