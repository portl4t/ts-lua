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


function corp_transform(data, eos)
    if ts.ctx['cl'] == nil then
        ts.http.resp_transform.set_downstream_bytes(10)
        ts.ctx['cl'] = true
    end

    if data ~= nil then
        ts.ctx['resp'] = ts.ctx['resp'] .. data
    end

    if string.len(ts.ctx['resp']) >= 10 then
        return string.sub(ts.ctx['resp'], 1, 10), 1

    else
        return nil, eos
    end
end


function do_remap()
    ts.hook(TS_LUA_RESPONSE_TRANSFORM, corp_transform)

    ts.http.resp_cache_transformed(0)
    ts.http.resp_cache_untransformed(1)
    ts.ctx['resp'] = ''
    return 0
end

