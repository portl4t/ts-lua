--  Licensed to the Apache Software Foundation (ASF) under one
--  or more contributor license agreements.  See the NOTICE file
--  distributed with this work for additional information
--  regarding copyright ownership.  The ASF licenses this file
--  to you under the Apache License, Version 2.0 (the
--  "License"); you may not use this file except in compliance
--  with the License.  You may obtain a copy of the License at
--
--  http://www.apache.org/licenses/LICENSE-2.0
--
--  Unless required by applicable law or agreed to in writing, software
--  distributed under the License is distributed on an "AS IS" BASIS,
--  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--  See the License for the specific language governing permissions and
--  limitations under the License.

require 'os'

function send_data(a, b, c)
    print(a)
    print(b)
    print(c)

    local res = ts.fetch('http://192.168.131.128:8080/am.txt')
    print(res.body)

    local nt = os.time()..' Zheng.\n'
    local resp =  'HTTP/1.0 200 OK\r\n' ..
                  'Server: ATS/3.2.0\r\n' ..
                  'Content-Type: text/plain\r\n' ..
                  'Content-Length: ' .. string.format('%d', string.len(nt)) .. '\r\n' ..
                  'Last-Modified: ' .. os.date("%a, %d %b %Y %H:%M:%S GMT", os.time()) .. '\r\n' ..
                  'Connection: keep-alive\r\n' ..
                  'NG: hell\r\n' ..
                  'Cache-Control: max-age=7200\r\n' ..
                  'Accept-Ranges: bytes\r\n\r\n' ..
                  nt
    ts.sleep(1)

    local nn = ts.fetch('http://192.168.131.128:8080/naga.txt')
    print(nn.body)
    ts.say(resp)
end

function read_response()
    local ng = ts.server_response.header['NG']
    if ng ~= nil then
        print(ng)
    end
end

function do_remap()
    local inner =  ts.http.is_internal_request()
    if inner ~= 0 then
        return 0
    end

    ts.hook(TS_LUA_HOOK_READ_RESPONSE_HDR, read_response)
    ts.http.intercept(send_data, 1, 2, 3)
    return 0
end

