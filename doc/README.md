# ATS-LUA 
***
* ts-lua 安装

* ts-lua 使用


# ts-lua 安装

:one: 安装依赖包

	yum install lua-devel trafficserver trafficserver-devel
***
:bulb: trafficserver 是以rpm包的方式安装的不懂的看老赵的[SPEC](http://zymlinux.net/trafficserver-dev.spec)文件

***
:two:  github源码获取
	
	git clone https://github.com/portl4t/ts-lua.git
***

:three:  编译安装

	cd ts-lua/src
	make
    make rpm 
	make install
***

:four:  ats 配置

	record.config 开启ts-lua debug tag 支持
	
	traffic_line -s proxy.config.diags.debug.enabled -v 1
	traffic_line -s proxy.config.diags.debug.tags -v "ts_lua"
	traffic_line -x
	
	remap.config 开启插件支持(ts_lua 为remap类型插件)
	
	map http://www.test.com/  http://www.test.com/ @plugin=libtslua.so @pparam=/etc/trafficserver/test_ret_403.lua

:warning:test_ret_403.lua 脚本一定要存在，否则ats会一直重启。:cry:已经建议阙寒修改


:five: ats流程图

![image](https://raw.github.com/portl4t/ts-lua/phonehold/doc/ats-workflow.png)

# ts-lua 使用

### TS-LUA API

:one:  Client 相关API


### ts.client_request.client_addr.get_port()

**语法:** *ts.client_request.client_addr.get_port()*

**context**: ts.client

**返回值**: int

记录客户端 Socket 连接的端口号

使用举例:

	function send_response()
    	ts.client_response.header['Client_Port'] = ts.ctx['Client_Port']
    	return 0
	end

    function do_remap()
    	local req_client_port = ts.client_request.client_addr.get_port()
        ts.ctx['Client_Port'] = req_client_port
       return 0
    end
***



### ts.client_request.client_addr.get_ip()

**语法:** *ts.client_request.client_addr.get_ip()*

**context**: do_remap

**返回值** :char

记录客户端 Socket 连接的ip

使用举例:

	function send_response()
    	ts.client_response.header['Client_IP'] = ts.ctx['Client_IP']
    	return 0
	end

    function do_remap()
    	local req_client_ip = ts.client_request.client_addr.get_ip()
        ts.ctx['Client_IP] = req_client_ip
       return 0
    end
***


### ts.client_request.client_addr.get_addr()

**语法:** *ts.client_request.client_addr.get_addr()*

**context**: do_remap

**返回值:** ip port family (int,int,char)

记录客户端 Socket 信息

:warning: family 为判定客户端是ipv4 还是ipv6

使用举例:

	function send_response()
    	ts.client_response.header['Client_IP'] = ts.ctx['Client_IP']
    	ts.client_response.header['Client_Port'] = ts.ctx['Client_Port']
    	ts.client_response.header['Client_Family'] = ts.ctx['Client_Family']

    	return 0
	end

    function do_remap()
    	local ip,port,family = ts.client_request.client_addr.get_addr()
        ts.ctx['Client_IP] = ip
        ts.ctx['Client_Port'] =port
        ts.ctx['Client_Family'] = family
       return 0
    end
***

### ts.client_request.get_uri()

**语法:** *ts.client_request.get_uri()*

**context**: do_remap

**返回值** :char

记录客户端请求的 Path

使用举例:

	function send_response()
    	ts.client_response.header['Client_URI'] = ts.ctx['Client_URI']
    	return 0
	end

    function do_remap()
    	local req_client_uri = ts.client_request.get_uri()
        ts.ctx['Client_URI] = req_client_uri
       return 0
    end
***

### ts.client_request.get_uri_args()

**语法:** *ts.client_request.get_uri_args()*

**context**: do_remap

**返回值** :char

记录客户端请求的参数

使用举例:

	function send_response()
    	ts.client_response.header['Client_URI'] = ts.ctx['Client_URI_ARGS']
    	return 0
	end

    function do_remap()
    	local req_client_uri_args = ts.client_request.get_uri_args()
        ts.ctx['Client_URI_ARGS] = req_client_uri_args
       return 0
    end
***


### ts.client_request.header

**语法:** *ts.client_request.header*

**context**: do_remap

**返回值** :lua table

记录客户端请求的Header信息

使用举例:

	function send_response()
    	ts.client_response.header['HOST'] = ts.ctx['HOST']

    	return 0
	end

    function do_remap()
    	local host = ts.client_request.header.Host
        ts.ctx['HOST] = host
       return 0
    end
***

### ts.client_request.header


**语法:** *ts.client_request.header*

**context**: do_remap

**返回值** :lua table

更改客户端请求的Header信息

使用举例:

	function send_response()
    	ts.client_response.header['HOST'] = ts.ctx['HOST']

    	return 0
	end

    function do_remap()
    	ts.client_request.header['Host'] = 'google.com'
    	ts.ctx['HOST']= ts.client_request.header.Host
       return 0
    end
***



### ts.client_request.get_method


**语法:** *ts.client_request.get_method*

**context**: do_remap

**返回值** :char

返回客户端请求的方法

使用举例:

	function send_response()
    	ts.client_response.header['METHOD'] = ts.ctx['METHOD']

    	return 0
	end

    function do_remap()
    	local ts.client_method = ts.client_request.get_method()
    	ts.ctx['METHOD']= ts.client_method
       return 0
    end
***

:warning: set_method set_uri_args set_uri 不支持，如果支持就可以改为请求的方法 以及做url防盗链了。:cry: 

