if (WIN32 OR APPLE)
    cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
else()
    cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
endif()

project(interationcenterdll)

set(BasicLibDir ${PROJECT_SOURCE_DIR}/../../../ccbasic)

include_directories(${BasicLibDir}/src/inc)
include_directories(${BasicLibDir}/3rd/libevent/include)
include_directories(${BasicLibDir}/3rd/lua/src)
include_directories(${BasicLibDir}/3rd/cryptopp)
include_directories(${BasicLibDir}/3rd/rapidjson/include)
include_directories(${PROJECT_SOURCE_DIR}/../..)

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
else()
	set(RunProjectCXX_FLAGS "${RunProjectCXX_FLAGS} --std=c++11")
	add_definitions(-DLINUX -D__LINUX -DBASIC_DLL_IMPORTS)
endif()

#flag
set(CMAKE_CXX_FLAGS ${RunProjectCXX_FLAGS})
set(CMAKE_C_FLAGS ${RunProjectC_FLAGS})
#link
set(RunProjectLinks ${RunProjectLinks} basicdll cryptopp-static kernel)

set(RunProject_SOURCES
	${BasicLibDir}/src/scbasic/net/net.h
	${BasicLibDir}/src/scbasic/net/sendbuffer.h
	${BasicLibDir}/src/scbasic/net/net.cpp
	${BasicLibDir}/src/scbasic/net/sendbuffer.cpp
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_auto.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_define.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_error.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_function.h
    ${PROJECT_SOURCE_DIR}/interationcenter.h
    ${PROJECT_SOURCE_DIR}/interationcenterserversessionctx.h
	${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_auto.cpp
    ${PROJECT_SOURCE_DIR}/interationcenter.cpp
    ${PROJECT_SOURCE_DIR}/interationcenterserversessionctx.cpp
    ${PROJECT_SOURCE_DIR}/main.cpp
		)
set(TotalSourceData ${RunProject_SOURCES})
source_group_by_dir(TotalSourceData)

add_library(interationcenter SHARED ${RunProject_SOURCES})
target_link_libraries(interationcenter ${RunProjectLinks})
if(MSVC)
	add_custom_command(TARGET interationcenter POST_BUILD COMMAND if not exist "\"$(OutDir)conf\"" mkdir "\"$(OutDir)conf\"")
	add_custom_command(TARGET interationcenter POST_BUILD COMMAND xcopy /Y /Q "\"${PROJECT_SOURCE_DIR}\\..\\..\\conf\\interationcenter.lua\"" "\"$(OutDir)conf\"")
	add_custom_command(TARGET interationcenter POST_BUILD COMMAND xcopy /Y /Q "\"${PROJECT_SOURCE_DIR}\\..\\..\\interationcenter.bat\"" "\"$(OutDir)\"" )
	
endif()