
resp =  'HTTP/1.1 200 OK\r\n' ..
        'Server: ATS/3.2.0\r\n' ..
        'Date: Sat, 26 Oct 2013 18:31:31 GMT\r\n' ..
        'Content-Type: text/plain\r\n' ..
        'Content-Length: 15\r\n' ..
        'Last-Modified: Mon, 19 Aug 2013 21:30:41 GMT\r\n' ..
        'Connection: keep-alive\r\n' ..
        'Cache-Control: no-cache\r\n' ..
        'Accept-Ranges: bytes\r\n\r\n' ..
        'am sl lk ha gi\n'


function send_data()
    ts.sleep(1)
    return resp
end


function do_remap()
    ts.http.intercept(send_data)
    return 0
end

