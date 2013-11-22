math.randomseed(os.time())
--生成1024-2048 之间字节的body
local randomlen= math.random(1024,2048)
local body=string.rep("a", randomlen)
len=string.len(body)
resp =  'HTTP/1.1 200 OK\r\n' ..
        'Server: ATS/3.2.0\r\n' ..
        'Date: Sat, 26 Oct 2013 18:31:31 GMT\r\n' ..
        'Content-Type: text/html\r\n' ..
        'Content-Length:' .. len ..  '\r\n' ..
        'Last-Modified: Mon, 19 Aug 2013 21:30:41 GMT\r\n' ..
        'Connection: keep-alive\r\n' ..
        'Cache-Control: no-cache\r\n' ..
        'Accept-Ranges: bytes\r\n\r\n' ..
        body ..'\n'


function send_data()
    return resp
end


function do_remap()

    ts.http.intercept(send_data)
