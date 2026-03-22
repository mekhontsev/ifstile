cmake_minimum_required(VERSION 3.27)

cmake_policy(SET CMP0117 NEW)

project(external C CXX)

set(arch32 ${CMAKE_SIZEOF_VOID_P} EQUAL 4)
set(arch64 ${CMAKE_SIZEOF_VOID_P} EQUAL 8)

if(MSVC AND CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
	set(platform "msvc_arm")
	set(libxt lib)
	set(libprefix "")
elseif(MSVC)
	set(platform "msvc")
	set(libxt lib)
	set(libprefix "")
elseif(APPLE)
	enable_language(OBJC)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)
	set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
	set(platform "macos")
	set(libxt a)
	set(libprefix "lib")
elseif(ANDROID)
	set(platform "android")
	set(libxt a)
	set(libprefix "lib")
elseif(EMSCRIPTEN)
	set(platform "wasm")
	set(libxt a)
	set(libprefix "lib")	
elseif(LINUX)
	set(platform "linux")
	set(libxt a)
	set(libprefix "lib")
else()
	message(FATAL_ERROR "not supported")
endif()

if(${arch32})
	set(platform ${platform}_32)
endif()

message(STATUS "platform=${platform}")

#-------------------------------------------------------------------------------

set(CMAKE_CONFIGURATION_TYPES Debug Release)
set(CMAKE_USE_RELATIVE_PATHS ON)
set(SDL_CAMERA OFF CACHE BOOL "")
set(SDL_AUDIO OFF CACHE BOOL "")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "")

set(out_dir ${CMAKE_CURRENT_BINARY_DIR}/../lib/${platform})

set(con $<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${out_dir}/${con})

#-------------------------------------------------------------------------------
include(../compiler_flags.cmake)
#-------------------------------------------------------------------------------
set (custom "custom")
add_custom_target(${custom} ALL)

#-------------------------------------------------------------------------------
message("Configuring SDL...")
set(OPT_DEF_LIBC ON)
add_subdirectory(
	${CMAKE_CURRENT_BINARY_DIR}/../SDL 
	${CMAKE_CURRENT_BINARY_DIR}/SDL 
	EXCLUDE_FROM_ALL
)

if(MSVC)
set(RUNTIME_LIB "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set (SDL3_target SDL3-static)
add_dependencies(${custom} ${SDL3_target})
set_target_properties (${SDL3_target} PROPERTIES 
	OUTPUT_NAME SDL3
	COMPILE_PDB_NAME SDL3
	PROJECT_LABEL SDL3
	DEBUG_POSTFIX ""
	MSVC_RUNTIME_LIBRARY "${RUNTIME_LIB}"
	PREFIX "${libprefix}"
)

