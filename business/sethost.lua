
HOSTNAME = ''

function __init__(argtb)

    if (#argtb) < 1 then
        print(argtb[0], 'hostname parameter required!!')
        return -1
    end

    HOSTNAME = argtb[1]
end

function do_remap()
    req_host = ts.client_request.header.Host

    if req_host == nil then
        return 0
    end

    ts.client_request.header['Host'] = HOSTNAME

    return 0
end

