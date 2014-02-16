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

function send_request()
    print(ts.server_request.header['Host'])
end

function do_remap()
    ts.client_request.set_url_host('192.168.231.129')
    ts.client_request.set_url_port('9090')
    ts.hook(TS_LUA_HOOK_SEND_REQUEST_HDR, send_request)
    return TS_LUA_REMAP_DID_REMAP
end

