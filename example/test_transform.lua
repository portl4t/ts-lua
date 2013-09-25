

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

