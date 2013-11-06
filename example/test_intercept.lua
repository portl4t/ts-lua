
require 'os'

function send_data()
    local nt = os.time()..' Zheng.\n'

    local resp =  'HTTP/1.1 200 OK\r\n' ..
        'Server: ATS/3.2.0\r\n' ..
        'Content-Type: text/plain\r\n' ..
        'Content-Length: ' .. string.len(nt) .. '\r\n' ..
        'Last-Modified: ' .. os.date("%a, %d %b %Y %H:%M:%S GMT", os.time()) .. '\r\n' ..
        'Connection: keep-alive\r\n' ..
        'Cache-Control: max-age=7200\r\n' ..
        'Accept-Ranges: bytes\r\n\r\n' ..
        nt

    ts.sleep(1)
    return resp
end


function do_remap()
    ts.http.intercept(send_data)
    return 0
end

