function send_response()

  ts.client_response.header['Cache-Control'] = 'max-age=10'
  ts.client_response.header['Server'] = 'ATS/5.1.0'
  return 0
end

function read_response()
  http_status = ts.server_response.header.get_status()
  ts.debug(string.format('server_response status = %d', http_status))
  if http_status >= 400 then
    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
  end
  return 0
end

function do_remap()
  ts.hook(TS_LUA_HOOK_READ_RESPONSE_HDR, read_response)
  return 0
end
