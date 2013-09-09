
require 'md5'

function string.tohex(str)
    return (str:gsub('.', function (c)
        return string.format('%02X', string.byte(c))
    end))
end

function do_send_response()
    mm = md5:new()
    mm:update(ts.ctx['tt'])
    ts.client_response.header['Uri'] = ts.ctx['tt']
    ts.client_response.header['Hex'] = (mm:final()):tohex()
    return 1
end


function do_remap()
    req_host = ts.client_request.header.Host

    if req_host == nil then
        return 0
    end

    ts.ctx['tt'] = ts.client_request.get_uri()

    return 1
end

