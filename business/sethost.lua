
HOSTNAME = 'img01.tbcdn.cn'


function do_remap()
    req_host = ts.client_request.header.Host

    if req_host == nil then
        return 0
    end

    ts.client_request.header['Host'] = HOSTNAME

    return 0
end

