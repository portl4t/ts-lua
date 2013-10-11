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
