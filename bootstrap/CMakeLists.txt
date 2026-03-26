cmake_minimum_required(VERSION 4.2)

project(external C CXX)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(arch32 TRUE)
else()
    set(arch32 FALSE)
endif()


if(MSVC AND (CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64"
		OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64)$"))
	set(platform "msvc_arm")
elseif(MSVC)
	set(platform "msvc")
elseif(APPLE)
	set(platform "macos")
elseif(ANDROID)
	set(platform "android")
elseif(EMSCRIPTEN)
	set(platform "wasm")
elseif(LINUX)
	set(platform "linux")
else()
	message(FATAL_ERROR "not supported")
endif()

if(MSVC)
	set(libprefix "")
else()
	set(libprefix "lib")
endif()

if(arch32)
	set(platform ${platform}_32)
endif()

message(STATUS "platform=${platform}")

if(APPLE)
	enable_language(OBJC)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)
	set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

#-------------------------------------------------------------------------------

set(CMAKE_CONFIGURATION_TYPES Debug Release)
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
message(STATUS "Configuring SDL...")
set(SDL_LIBC ON CACHE BOOL "")
add_subdirectory(
	${CMAKE_CURRENT_BINARY_DIR}/../SDL 
	${CMAKE_CURRENT_BINARY_DIR}/SDL 
	EXCLUDE_FROM_ALL
)

set (SDL3_target SDL3-static)
add_dependencies(${custom} ${SDL3_target})

set_target_properties (${SDL3_target} PROPERTIES 
	OUTPUT_NAME SDL3
	COMPILE_PDB_NAME SDL3
	PROJECT_LABEL SDL3
	DEBUG_POSTFIX ""
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	PREFIX "${libprefix}"
)

