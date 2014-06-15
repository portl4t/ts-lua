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


st = 'OVER\n'

function upper_transform(data, eos)
    if eos == 1 then
        return string.upper(data)..st, eos
    else
        return string.upper(data), eos
    end
end

function setcl(inlen)
    return inlen + string.len(st)
end

function send_response()
    ts.client_response.header['SHE'] = 'hebe'
    return 0
end


function do_remap()
    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
    ts.hook(TS_LUA_RESPONSE_TRANSFORM, upper_transform, setcl)

    ts.http.resp_cache_transformed(0)
    ts.http.resp_cache_untransformed(1)
    return 0
end

