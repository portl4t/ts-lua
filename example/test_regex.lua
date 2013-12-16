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


function send_response()
    ts.client_response.header['K-K'] = ts.ctx['kk']
    return 0
end


function do_remap()
    local r = ts.re.compile('<(.*)>(.*)<(.*)>')
--    local r = ts.re.compile('Hello')
    local ret = ts.re.match(r, '<nani>Hello World<sigoyi>')
    ts.re.free(r)

    if ret
    then
        print('match')
--        print(ret[0])
--        print(ret[1])
--        print(ret[2])
    end

    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)

    return 0
end

