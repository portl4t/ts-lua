local HOSTNAME = ''

function __init__(argtb)

    if (#argtb) < 1 then
        print(argtb[0], 'hostname parameter required!!')
        return -1
    end

    HOSTNAME = argtb[1]
end

function send_response()
    ts.client_response.header['X-Cache'] = ts.ctx['lookup']
end


function cache_lookup()
    local cache_status = ts.http.get_cache_lookup_status()
    --ts.http.set_resp(400, "params invalid\n")
     if cache_status == 0 then
        status= "MISS "
    elseif cache_status == 1 then
        status = "HIT_STALE "
    elseif cache_status == 2 then
        status = "HIT "
    elseif cache_status == 3 then
        status = "SKIP "
    else
        status = "Unknow "
    end
    local return_status = status .. HOSTNAME
    ts.ctx['lookup'] = return_status
end

function do_remap()
    ts.hook(TS_LUA_HOOK_SEND_RESPONSE_HDR, send_response)
    ts.hook(TS_LUA_HOOK_CACHE_LOOKUP_COMPLETE, cache_lookup)
end
