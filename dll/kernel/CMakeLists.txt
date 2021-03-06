if (WIN32 OR APPLE)
    cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
else()
    cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
endif()

project(kerneldll)

#build basiclib
set(BASICLIB_BUILD_SHARED ON CACHE BOOL "flags")
set(BASICLIB_BUILD_LUA ON CACHE BOOL "flags")
add_subdirectory(${BasicLibPath}/src ${PROJECT_BINARY_DIR}/BasicLib)

#flag
set(RunProjectCXX_FLAGS ${CMAKE_CXX_FLAGS})
set(RunProjectC_FLAGS ${CMAKE_C_FLAGS})
#link
set(RunProjectLinks)

if(MSVC)
	set(RunProjectCXX_FLAGS "${RunProjectCXX_FLAGS} /MP")
	set(RunProjectC_FLAGS "${RunProjectC_FLAGS} /MP")
	add_definitions(-DUNICODE -D_UNICODE -DBASIC_DLL_IMPORTS -DKAGUYA_SUPPORT_MULTIPLE_SHARED_LIBRARY)
	set(RunProjectLinks ${RunProjectLinks})
else()
	set(RunProjectCXX_FLAGS "${RunProjectCXX_FLAGS} --std=c++11")
	add_definitions(-DLINUX -D__LINUX  -DKAGUYA_SUPPORT_MULTIPLE_SHARED_LIBRARY)
	set(RunProjectLinks ${RunProjectLinks})
endif()

#flag
set(CMAKE_CXX_FLAGS ${RunProjectCXX_FLAGS})
set(CMAKE_C_FLAGS ${RunProjectC_FLAGS})
#link
set(RunProjectLinks ${RunProjectLinks} basiclib cryptopp-static sproto)

set(RunProject_SOURCES
    ${BasicLibPath}/src/scbasic/commu/basicclient.h
    ${BasicLibPath}/src/scbasic/commu/ipverify.h
    ${BasicLibPath}/src/scbasic/commu/servertemplate.h
    ${BasicLibPath}/src/scbasic/http/httpdefine.h
    ${BasicLibPath}/src/scbasic/http/httpparser.h
    ${BasicLibPath}/src/scbasic/http/httprequest.h
    ${BasicLibPath}/src/scbasic/http/httpresponse.h
    ${BasicLibPath}/src/scbasic/http/httpsession.h
    ${BasicLibPath}/src/scbasic/mysql/mysqlconnector.h
    ${BasicLibPath}/src/scbasic/redis/read.h
    ${BasicLibPath}/src/scbasic/redis/redisprotocal.h
    ${BasicLibPath}/src/scbasic/redis/sds.h
	${BasicLibPath}/src/scbasic/lua/ccbasiclib_lua.h
	${BasicLibPath}/src/scbasic/lua/ccbasiclib_lua.cpp
	${BasicLibPath}/src/scbasic/encode/rsaencode.h
	${BasicLibPath}/src/scbasic/encode/rsaencode.cpp
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_auto.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_define.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_error.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_function.h
    ${PROJECT_SOURCE_DIR}/ccframe/coroutinedata/coroutinestackdata.h
	${PROJECT_SOURCE_DIR}/ccframe/coroutinedata/coroutinestackdata.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/coroutine_ctx.h
    ${PROJECT_SOURCE_DIR}/ccframe/ctx_handle.h
    ${PROJECT_SOURCE_DIR}/ccframe/ctx_msgqueue.h
    ${PROJECT_SOURCE_DIR}/ccframe/ctx_threadpool.h
    ${PROJECT_SOURCE_DIR}/ccframe/dllmodule.h
    ${PROJECT_SOURCE_DIR}/ccframe/log/ctx_log.h
    ${PROJECT_SOURCE_DIR}/ccframe/net/ccframedefaultfilter.h
    ${PROJECT_SOURCE_DIR}/ccframe/net/ccframeserver.h
    ${PROJECT_SOURCE_DIR}/ccframe/net/ccframeserver_frame.h
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_ccframeserversession.h
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_mysql.h
	${PROJECT_SOURCE_DIR}/ccframe/net/ctx_http.h
	${PROJECT_SOURCE_DIR}/ccframe/net/ctx_http.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_netclient.h
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_netclientsevercommu.h
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_redis.h
    ${PROJECT_SOURCE_DIR}/ccframe/public/kernelrequeststoredata.h
    ${PROJECT_SOURCE_DIR}/ccframe/state/ctx_threadstate.h
    ${PROJECT_SOURCE_DIR}/kerneltolua.h
    ${PROJECT_SOURCE_DIR}/kernel_head.h
    ${PROJECT_SOURCE_DIR}/kernel_server.h
	${BasicLibPath}/src/scbasic/commu/basicclient.cpp
    ${BasicLibPath}/src/scbasic/commu/ipverify.cpp
    ${BasicLibPath}/src/scbasic/commu/servertemplate.cpp
    ${BasicLibPath}/src/scbasic/http/httpparser.cpp
    ${BasicLibPath}/src/scbasic/http/httprequest.cpp
    ${BasicLibPath}/src/scbasic/http/httpresponse.cpp
    ${BasicLibPath}/src/scbasic/http/httpsession.cpp
    ${BasicLibPath}/src/scbasic/mysql/mysqlconnector.cpp
    ${BasicLibPath}/src/scbasic/redis/read.c
    ${BasicLibPath}/src/scbasic/redis/redisprotocal.cpp
    ${BasicLibPath}/src/scbasic/redis/sds.c
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_auto.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/coroutine_ctx.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/ctx_handle.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/ctx_msgqueue.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/ctx_threadpool.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/dllmodule.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/log/ctx_log.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ccframedefaultfilter.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ccframeserver.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ccframeserver_frame.cpp
	${PROJECT_SOURCE_DIR}/ccframe/net/ctx_gate.cpp
	${PROJECT_SOURCE_DIR}/ccframe/net/ctx_gate.h
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_ccframeserversession.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_mysql.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_netclient.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_netclientsevercommu.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_redis.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/public/kernelrequeststoredata.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/state/ctx_threadstate.cpp
	${PROJECT_SOURCE_DIR}/ccframe/paiming/ctx_tiaozhanpaiming.h
	${PROJECT_SOURCE_DIR}/ccframe/paiming/ctx_tiaozhanpaiming.cpp
	${PROJECT_SOURCE_DIR}/ccframe/paiming/ctx_tiaozhanpaiming_record.h
	${PROJECT_SOURCE_DIR}/ccframe/paiming/ctx_tiaozhanpaiming_record.cpp
    ${PROJECT_SOURCE_DIR}/kerneltolua.cpp
    ${PROJECT_SOURCE_DIR}/kernel_server.cpp
    ${PROJECT_SOURCE_DIR}/kernel_server_http.cpp
	${PROJECT_SOURCE_DIR}/lua/loadconfig.lua
    ${PROJECT_SOURCE_DIR}/lua/main.lua
	${PROJECT_SOURCE_DIR}/lua/exec.lua
		)
set(TotalSourceData ${RunProject_SOURCES})
source_group_by_dir(TotalSourceData)

add_library(kernel SHARED ${RunProject_SOURCES})
target_link_libraries(kernel ${RunProjectLinks})
if(MSVC)
	add_custom_command(TARGET kernel POST_BUILD COMMAND if not exist "\"$(OutDir)conf\"" mkdir "\"$(OutDir)conf\"")
	add_custom_command(TARGET kernel POST_BUILD COMMAND if not exist "\"$(OutDir)lua\"" mkdir "\"$(OutDir)lua\"")
	add_custom_command(TARGET kernel POST_BUILD COMMAND xcopy /Y /Q "\"${BasicLibPath}\\src\\scbasic\\sproto\\*.lua\"" "\"$(OutDir)lua\"" )
	add_custom_command(TARGET kernel POST_BUILD COMMAND xcopy /Y /Q "\"${PROJECT_SOURCE_DIR}\\lua\\*.lua\"" "\"$(OutDir)lua\"" )
endif()
