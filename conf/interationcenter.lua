--指明自己的远程id
--serverid=1
--servername="interationcenterserver1"

--日志文件目录
logpath="$GetModulePath/log/"

--启动的网络线程
NetThreadCount=2

--启动工作线程数
workthread=2

--http监听地址
StartDefaultHttp=1
http_Address="0.0.0.0:19080"
http_iptrust="*"

--加载的动态库
dlllist="kernel.dll;interationcenter.dll"
--替换的动态库
--replacedlllist="skynettest.dll>skynettest_V2.dll"
--默认启动的主模板类
maintemplate="CInterationCenterCtx"

--服务
server_serversessionctx="CInterationCenterServerSessionCtx"
server_Address="0.0.0.0:6666"
server_serverlistentimeout=45
server_serverlisteniptrust="*"
--网关
server_gate="CCoroutineCtx_GateInteration"
--业务ctx
server_gate_serversessionctx="CInterationCenterServerSessionCtx"
--
requesttimeout=5

