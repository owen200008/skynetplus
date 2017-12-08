if (WIN32 OR APPLE)
    cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
else()
    cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
endif()

project(skynetplusexe)

set(SkynetPlusBasicLibPath ${PROJECT_SOURCE_DIR}/../ccbasic CACHE STRING "BasiclibPath")
set(BasicLibPath ${SkynetPlusBasicLibPath} CACHE PATH "BasicLibPath" FORCE)

if(SkynetPlusOutputPath)
	SET(EXECUTABLE_OUTPUT_PATH ${SkynetPlusOutputPath})
	SET(LIBRARY_OUTPUT_PATH ${SkynetPlusOutputPath})
else()
	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
	SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
endif()

#create default config
if(SkynetPlusProtocalFiles)
	file(WRITE ${PROJECT_SOURCE_DIR}/conf/sprotoencode.lua "--parse protocal file\nsprotofilename=\"${SkynetPlusProtocalFiles}\"\nsprotostoreluafile=\"lua/skynetplus.protocal\"\n")
else()
	file(WRITE ${PROJECT_SOURCE_DIR}/conf/sprotoencode.lua "--parse protocal file\nsprotofilename=\"${PROJECT_SOURCE_DIR}/dll/skynetplusprotocal/skynetplusprotocal.h\"\nsprotostoreluafile=\"lua/skynetplus.protocal\"\n")
endif()

#build kernel
add_subdirectory(${PROJECT_SOURCE_DIR}/dll/kernel ${PROJECT_BINARY_DIR}/kernel)
#build interationcenter
add_subdirectory(${PROJECT_SOURCE_DIR}/dll/interationcenter ${PROJECT_BINARY_DIR}/interationcenter)

include_directories(${SkynetPlusBasicLibPath}/src/inc)

#flag
set(RunProjectCXX_FLAGS ${CMAKE_CXX_FLAGS})
set(RunProjectC_FLAGS ${CMAKE_C_FLAGS})
#link
set(RunProjectLinks_SkynetPlus)

if(MSVC)
	set(RunProjectCXX_FLAGS "${RunProjectCXX_FLAGS} /MP")
	set(RunProjectC_FLAGS "${RunProjectC_FLAGS} /MP")
	add_definitions(-DUNICODE -D_UNICODE -DBASIC_DLL_IMPORTS)
else()
	set(RunProjectCXX_FLAGS "${RunProjectCXX_FLAGS} --std=c++11")
	add_definitions(-DLINUX -D__LINUX)
endif()

#flag
set(CMAKE_CXX_FLAGS ${RunProjectCXX_FLAGS})
set(CMAKE_C_FLAGS ${RunProjectC_FLAGS})
#link
set(RunProjectLinks_SkynetPlus ${RunProjectLinks_SkynetPlus} basiclib)


set(RunProject_SOURCES
    ${PROJECT_SOURCE_DIR}/src/skynetplusmain.cpp
		)
add_executable(skynetplus ${RunProject_SOURCES}) 
target_link_libraries(skynetplus ${RunProjectLinks_SkynetPlus}) 
if(MSVC)
	add_custom_command(TARGET skynetplus POST_BUILD COMMAND if not exist "\"$(OutDir)conf\"" mkdir "\"$(OutDir)conf\"")
	add_custom_command(TARGET skynetplus POST_BUILD COMMAND xcopy /Y /Q "\"${PROJECT_SOURCE_DIR}\\conf\\sprotoencode.lua\"" "\"$(OutDir)conf\"")
	add_custom_command(TARGET skynetplus POST_BUILD COMMAND xcopy /Y /Q "\"${PROJECT_SOURCE_DIR}\\script\\sprotoencode.bat\"" "\"$(OutDir)\"" )
endif()
