if (WIN32 OR APPLE)
    cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
else()
    cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
endif()

project(kerneldll)

set(BasicLibDir ${PROJECT_SOURCE_DIR}/../../../ccbasic)
#build basiclib
set(BASICLIB_BUILD_STATIC OFF CACHE BOOL "flags")
add_subdirectory(${BasicLibDir}/src ${PROJECT_BINARY_DIR}/BasicLib)
#build cryptopp
set(BUILD_TESTING OFF CACHE BOOL "flags")
set(BUILD_SHARED OFF CACHE BOOL "flags")
set(USE_INTERMEDIATE_OBJECTS_TARGET OFF CACHE BOOL "flags")
add_subdirectory(${BasicLibDir}/3rd/cryptopp ${PROJECT_BINARY_DIR}/Cryptopp)
#build sproto
add_subdirectory(${BasicLibDir}/src/scbasic/sproto ${PROJECT_BINARY_DIR}/Sproto)

include_directories(${BasicLibDir}/src/inc)
include_directories(${BasicLibDir}/3rd/libevent/include)
include_directories(${BasicLibDir}/3rd/lua/src)
include_directories(${BasicLibDir}/3rd/cryptopp)
include_directories(${BasicLibDir}/3rd/rapidjson/include)

#flag
set(RunProjectCXX_FLAGS ${CMAKE_CXX_FLAGS})
set(RunProjectC_FLAGS ${CMAKE_C_FLAGS})
#link
set(RunProjectLinks)

if(MSVC)
	include_directories(${BasicLibDir}/3rd/libevent/build/include)
	set(RunProjectCXX_FLAGS "${RunProjectCXX_FLAGS} /MP")
	set(RunProjectC_FLAGS "${RunProjectC_FLAGS} /MP")
	add_definitions(-DUNICODE -D_UNICODE -D__BASICWINDOWS -DBASIC_DLL_IMPORTS)
	set(RunProjectLinks ${RunProjectLinks} ${BasicLibDir}/libs/FLzlib13.lib 
		${BasicLibDir}/3rd/libevent/build/lib/Debug/event.lib
		${BasicLibDir}/libs/lua53.lib)
else()
	set(RunProjectCXX_FLAGS "${RunProjectCXX_FLAGS} --std=c++11")
	add_definitions(-DLINUX -D__LINUX -DBASIC_DLL_IMPORTS)
	set(RunProjectLinks ${BasicLibDir}/libs/liblua.a
		${BasicLibDir}/3rd/libevent/.libs/libevent.a
		)
endif()

#flag
set(CMAKE_CXX_FLAGS ${RunProjectCXX_FLAGS})
set(CMAKE_C_FLAGS ${RunProjectC_FLAGS})
#link
set(RunProjectLinks ${RunProjectLinks} basicdll cryptopp-static sproto)

set(RunProject_SOURCES
    ${BasicLibDir}/src/scbasic/commu/basicclient.h
    ${BasicLibDir}/src/scbasic/commu/ipverify.h
    ${BasicLibDir}/src/scbasic/commu/servertemplate.h
    ${BasicLibDir}/src/scbasic/http/httpdefine.h
    ${BasicLibDir}/src/scbasic/http/httpparser.h
    ${BasicLibDir}/src/scbasic/http/httprequest.h
    ${BasicLibDir}/src/scbasic/http/httpresponse.h
    ${BasicLibDir}/src/scbasic/http/httpsession.h
    ${BasicLibDir}/src/scbasic/mysql/mysqlconnector.h
    ${BasicLibDir}/src/scbasic/redis/read.h
    ${BasicLibDir}/src/scbasic/redis/redisprotocal.h
    ${BasicLibDir}/src/scbasic/redis/sds.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_auto.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_define.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_error.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_function.h
    ${PROJECT_SOURCE_DIR}/ccframe/coroutinedata/coroutinestackdata.h
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
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_netclient.h
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_netclientsevercommu.h
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_redis.h
    ${PROJECT_SOURCE_DIR}/ccframe/public/kernelrequeststoredata.h
    ${PROJECT_SOURCE_DIR}/ccframe/state/ctx_threadstate.h
    ${PROJECT_SOURCE_DIR}/kerneltolua.h
    ${PROJECT_SOURCE_DIR}/kernel_head.h
    ${PROJECT_SOURCE_DIR}/kernel_server.h
	${BasicLibDir}/src/scbasic/net/net.h
	${BasicLibDir}/src/scbasic/net/sendbuffer.h
	${BasicLibDir}/src/scbasic/net/net.cpp
	${BasicLibDir}/src/scbasic/net/sendbuffer.cpp
	${BasicLibDir}/src/scbasic/commu/basicclient.cpp
    ${BasicLibDir}/src/scbasic/commu/ipverify.cpp
    ${BasicLibDir}/src/scbasic/commu/servertemplate.cpp
    ${BasicLibDir}/src/scbasic/http/httpparser.cpp
    ${BasicLibDir}/src/scbasic/http/httprequest.cpp
    ${BasicLibDir}/src/scbasic/http/httpresponse.cpp
    ${BasicLibDir}/src/scbasic/http/httpsession.cpp
    ${BasicLibDir}/src/scbasic/mysql/mysqlconnector.cpp
    ${BasicLibDir}/src/scbasic/redis/read.c
    ${BasicLibDir}/src/scbasic/redis/redisprotocal.cpp
    ${BasicLibDir}/src/scbasic/redis/sds.c
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
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_ccframeserversession.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_mysql.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_netclient.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_netclientsevercommu.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/net/ctx_redis.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/public/kernelrequeststoredata.cpp
    ${PROJECT_SOURCE_DIR}/ccframe/state/ctx_threadstate.cpp
    ${PROJECT_SOURCE_DIR}/kerneltolua.cpp
    ${PROJECT_SOURCE_DIR}/kernel_server.cpp
    ${PROJECT_SOURCE_DIR}/kernel_server_http.cpp
	${PROJECT_SOURCE_DIR}/lua/loadconfig.lua
    ${PROJECT_SOURCE_DIR}/lua/main.lua
		)
set(TotalSourceData ${RunProject_SOURCES})
source_group_by_dir(TotalSourceData)

add_library(kernel SHARED ${RunProject_SOURCES})
target_link_libraries(kernel ${RunProjectLinks})
if(MSVC)
	add_custom_command(TARGET kernel POST_BUILD COMMAND if not exist "\"$(OutDir)conf\"" mkdir "\"$(OutDir)conf\"")
	add_custom_command(TARGET kernel POST_BUILD COMMAND if not exist "\"$(OutDir)lua\"" mkdir "\"$(OutDir)lua\"")
	add_custom_command(TARGET kernel POST_BUILD COMMAND xcopy /Y /Q "\"${BasicLibDir}\\libs\\FLzlib13.dll\"" "\"$(OutDir)\"")
	add_custom_command(TARGET kernel POST_BUILD COMMAND xcopy /Y /Q "\"${BasicLibDir}\\3rd\\libevent\\build\\bin\\Debug\\event.dll\"" "\"$(OutDir)\"")
	add_custom_command(TARGET kernel POST_BUILD COMMAND xcopy /Y /Q "\"${BasicLibDir}\\libs\\lua53.dll\"" "\"$(OutDir)\"")
	add_custom_command(TARGET kernel POST_BUILD COMMAND xcopy /Y /Q "\"${BasicLibDir}\\libs\\lpeg.dll\"" "\"$(OutDir)\"")
	add_custom_command(TARGET kernel POST_BUILD COMMAND xcopy /Y /Q "\"${BasicLibDir}\\src\\scbasic\\sproto\\*.lua\"" "\"$(OutDir)lua\"" )
	add_custom_command(TARGET kernel POST_BUILD COMMAND xcopy /Y /Q "\"${PROJECT_SOURCE_DIR}\\lua\\*.lua\"" "\"$(OutDir)lua\"" )
endif()
