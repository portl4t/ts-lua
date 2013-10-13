

function send_response()
    ts.client_response.header['Args'] = ts.ctx['args']
end


function do_remap()
    local args = ts.client_request.get_uri_args()

    if args ~= nil then
        ts.ctx['args'] = args
        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
        return 0
    end

    ts.client_request.set_uri_args('hero=am&skill=burning')

    return 0
end

