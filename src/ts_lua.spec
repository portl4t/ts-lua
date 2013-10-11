%define VERSION 1.0
Name:           ts-lua
Version:        %VERSION
Release:        1%{?dist}
Summary:        Embed the Power of Lua into TrafficServer

Group:          tools
License:        GNU
URL:            https://github.com/portl4t/ts-lua
BuildRoot:      /tmp/ts-lua-build-root
Vendor:         YingCai
Prefix:         Yingcai
BuildRequires: trafficserver-devel lua-devel
Requires:      trafficserver
Autoreq:        no
%define _builddir           .
%define _rpmdir             .
%define _srcrpmdir          .
%define _build_name_fmt     %%{NAME}-%%{VERSION}-%%{RELEASE}-%%{ARCH}.rpm
%define __os_install_post   /usr/lib/rpm/brp-compress; echo 'Not stripping.'

%description
ts-lua - Embed the Power of Lua into TrafficServer


%prep

%build
make 


%install
rm -rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT/usr/lib64/trafficserver/plugins/
PREFIX=$RPM_BUILD_ROOT make install


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,nobody,nobody,-)
/usr/lib64/trafficserver/plugins/libtslua.so

* Fri Oct 11 2013 <b13621367396@gmail.com> - 1.0.1
+ First Build
