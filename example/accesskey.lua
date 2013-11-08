local md5 = require("md5")

string.split = function(s, p)
    local rt= {}
    string.gsub(s, '[^'..p..']+', function(w) table.insert(rt, w) end )
    return rt
end

function send_response()
    return 0
end


function do_remap()
        local cip = ts.client_request.client_addr.get_ip()
        key= md5.sumhexa(cip)
        --ts.debug(cip)
        --ts.debug(key)
        if cip == nil then
                ts.http.set_resp(403, "Invaild IP\n")
        end

        local uri = ts.client_request.get_uri()
        --ts.debug(uri)
        if uri == nil then
                ts.http.set_resp(403, "Invaild URI\n")
        else
                local list = string.split(uri,'/')
                get_uri_key = list[1]
                realpath= "/"..list[2]
        end
        ts.debug(get_uri_key)
        ts.debug(realpath)
        if get_uri_key == key then
                ts.client_request.set_uri(realpath)
        else
                ts.http.set_resp(410, "URL Gone\n")
        end 
        --ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response);
        return 0
end

