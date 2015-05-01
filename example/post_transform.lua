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


function request_transform(data, eos)
    if data ~= nil then
        ts.ctx['body'] = ts.ctx['body'] .. data
    end

    if eos == 1 then
        local hash = ts.sha1(ts.ctx['body'])
        print(ts.ctx['body'])
        print(hash)
    end

    return data, eos
end

function do_remap()
    local method = ts.client_request.get_method()

    if method == 'POST' then
        ts.ctx['body'] = ''
        ts.hook(TS_LUA_REQUEST_TRANSFORM, request_transform)
    end

    return 0
end
