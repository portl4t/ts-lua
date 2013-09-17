

function send_response()
    ts.client_response.header['Now'] = ts.now()
    return 0
end


function do_remap()

    uri = ts.client_request.get_uri()

    pos, len = string.find(uri, '/css/')
    if pos ~= nil then
        ts.http.set_resp(403, "Document access failed :)\n")
        return 0
    end

    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)

    return 0
end

