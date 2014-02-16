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

function send_data()

    local res = ts.fetch('http://a.tbcdn.cn/echo/naizhen', 
                    {
                        ['header'] = {
                                        ['Host'] = 'a.tbcdn.cn',
                                        ['Accept'] = '*/*',
                                        ['User-Agent'] = 'libfetcher'
                                     }
                    }, 's')


    local resp = string.format('HTTP/1.1 %d OK\r\n', res.status)

    for key, value in pairs(res.header) do
        resp = resp .. key..': '..value .. '\r\n'
    end

    resp = resp .. '\r\n'

    ts.say(resp)
    ts.flush()

    repeat
        body, eos, err = ts.fetch_read(res)

        if err then
            print('error!!!')
            break

        else
            if body then
                ts.say(body)
                ts.flush()
            end

            if eos then
                break
            end
        end

    until false
end


function do_remap()
    ts.http.intercept(send_data)
    return 0
end

