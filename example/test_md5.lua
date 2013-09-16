
require 'md5'

function string.tohex(str)
    return (str:gsub('.', function (c)
        return string.format('%02X', string.byte(c))
    end))
end

function send_response()
    mm = md5:new()
    mm:update(ts.ctx['tt'])
    ts.client_response.header['Uri'] = ts.ctx['tt']
    ts.client_response.header['Hex'] = (mm:final()):tohex()
    return 0
end


function do_remap()
    req_host = ts.client_request.header.Host

    if req_host == nil then
        return 0
    end

    ts.ctx['tt'] = ts.client_request.get_uri()
    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response);

    return 0
end

