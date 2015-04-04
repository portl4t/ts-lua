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

function read_data(c)
    local nt = os.time()..' Zheng.\n'
    local resp =  'HTTP/1.0 200 OK\r\n' ..
                  'Server: ATS/5.2.0\r\n' ..
                  'Content-Type: text/plain\r\n' ..
                  'Content-Length: ' .. string.format('%d', string.len(nt)) .. '\r\n' ..
                  'Last-Modified: ' .. os.date("%a, %d %b %Y %H:%M:%S GMT", os.time()) .. '\r\n' ..
                  'Connection: keep-alive\r\n' ..
                  'Cache-Control: max-age=7200\r\n' ..
                  'Accept-Ranges: bytes\r\n\r\n' ..
                  nt

    if c == 'r' then
        local fd = ts.cache_open('http://soowind.com/goodness.txt', TS_LUA_CACHE_READ)
        if fd.hit then
            d = ts.cache_read(fd)
            ts.cache_close(fd)
            print(d)

        else
            print('miss')
        end

    elseif c == 'w' then
        local rfd = ts.cache_open('http://soowind.com/goodness.txt', TS_LUA_CACHE_READ)
        local hit = rfd.hit
        ts.cache_close(rfd)

        if hit ~= true then
            local fd = ts.cache_open('http://soowind.com/goodness.txt', TS_LUA_CACHE_WRITE)
            local n = ts.cache_write(fd, 'The Last Emperor.')
            print(string.format('write %d bytes', n))
            ts.cache_close(fd)
        end

    else
        ts.cache_remove('http://soowind.com/goodness.txt')
        print('removed')
    end

    ts.say(resp)
end

function do_remap()
    local cc = ts.client_request.header['CC']
    if cc == nil then
        return 0
    end

    ts.http.intercept(read_data, cc)
    return 0
end
