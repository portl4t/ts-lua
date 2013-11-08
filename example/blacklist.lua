local HOSTNAME = ''

function __init__(argtb)

    if (#argtb) < 1 then
        print(argtb[0], 'hostname parameter required!!')
        return -1
    end

    HOSTNAME = argtb[1]
end
time=ts.now()

body= "Access Deny From " ..  HOSTNAME .. time .."\n"
function send_response()
    ts.client_response.header['BlackList'] = ts.ctx['black']
end


function do_remap()
        local Memcached = require('Memcached')
        local m = Memcached.Connect()
        local req_url = ts.client_request.get_url()
        --ts.debug(req_url)
        local obj = m:get(req_url)
        --ts.debug(obj)
        if obj == nil then

        else
                ts.http.set_resp(403, body)
                ts.ctx['black']= obj
        end
        ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
end
