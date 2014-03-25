

local ft = ts.shared.DICT('fruit')

function do_remap()
    ft:set('apple', 5)
    ft:set('banana', 9)
    ft:set('cherry', 78)
    kt = ft:get_keys()

    for k, v in pairs(kt) do
        print(k)
        print(v)
    end
end

