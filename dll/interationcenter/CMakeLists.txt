if (WIN32 OR APPLE)
    cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
else()
    cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
endif()

project(interationcenterdll)

include_directories(${PROJECT_SOURCE_DIR}/../..)

#flag
set(RunProjectCXX_FLAGS ${CMAKE_CXX_FLAGS})
set(RunProjectC_FLAGS ${CMAKE_C_FLAGS})
#link
set(RunProjectLinks)

if(MSVC)
	include_directories(${BasicLibPath}/3rd/libevent/build/include)
	set(RunProjectCXX_FLAGS "${RunProjectCXX_FLAGS} /MP")
	set(RunProjectC_FLAGS "${RunProjectC_FLAGS} /MP")
	add_definitions(-DUNICODE -D_UNICODE -DBASIC_DLL_IMPORTS  -DKAGUYA_SUPPORT_MULTIPLE_SHARED_LIBRARY)
else()
	set(RunProjectCXX_FLAGS "${RunProjectCXX_FLAGS} --std=c++11")
	add_definitions(-DLINUX -D__LINUX -DKAGUYA_SUPPORT_MULTIPLE_SHARED_LIBRARY)
endif()

#flag
set(CMAKE_CXX_FLAGS ${RunProjectCXX_FLAGS})
set(CMAKE_C_FLAGS ${RunProjectC_FLAGS})
#link
set(RunProjectLinks ${RunProjectLinks} basiclib cryptopp-static kernel)

set(RunProject_SOURCES
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal.h
	${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal.cpp
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_auto.h
	${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_auto.cpp
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_define.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_error.h
    ${PROJECT_SOURCE_DIR}/../skynetplusprotocal/skynetplusprotocal_function.h
    ${PROJECT_SOURCE_DIR}/interationcenter.h
    ${PROJECT_SOURCE_DIR}/interationcenterserversessionctx.h
    ${PROJECT_SOURCE_DIR}/interationcenter.cpp
    ${PROJECT_SOURCE_DIR}/interationcenterserversessionctx.cpp
    ${PROJECT_SOURCE_DIR}/main.cpp
	${PROJECT_SOURCE_DIR}/ctx_gateinteration.cpp
	${PROJECT_SOURCE_DIR}/ctx_gateinteration.h
		)
set(TotalSourceData ${RunProject_SOURCES})
source_group_by_dir(TotalSourceData)

add_library(interationcenter SHARED ${RunProject_SOURCES})
target_link_libraries(interationcenter ${RunProjectLinks})
if(MSVC)
	add_custom_command(TARGET interationcenter POST_BUILD COMMAND if not exist "\"$(OutDir)conf\"" mkdir "\"$(OutDir)conf\"")
	add_custom_command(TARGET interationcenter POST_BUILD COMMAND xcopy /Y /Q "\"${PROJECT_SOURCE_DIR}\\..\\..\\conf\\interationcenter.lua\"" "\"$(OutDir)conf\"")
	add_custom_command(TARGET interationcenter POST_BUILD COMMAND xcopy /Y /Q "\"${PROJECT_SOURCE_DIR}\\..\\..\\script\\interationcenter.bat\"" "\"$(OutDir)\"" )
endif()