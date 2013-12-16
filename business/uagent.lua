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


local st = 'UCWEB|Opera Mobi|MAUI|BlackBerry|Symbian|SEMC|Palm|webOS|MEIZU|NetFront|Obigo|MIDP-2|CLDC-1.1|CLDC1.1'
local ua_re = nil

function __init__(argtb)
    ua_re = ts.re.compile(st)
    print(ua_re)
end

function __clean__()
    if ua_re ~= nil then
        print(ua_re)
        ts.re.free(ua_re)
    end
end

function send_response()
    ts.client_response.header['CR'] = ts.ctx['result']
    return 0
end

function do_remap()

    local nk = ts.client_request.header.NK

    if nk == nil or ua_re == nil
    then
        return 0
    end

    local res = ts.re.match(ua_re, nk)

    if res ~= nil
    then
        ts.ctx['result'] = 'match'
    else
        ts.ctx['result'] = 'not match'
    end

    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)

    return 0
end

