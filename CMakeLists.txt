cmake_minimum_required(VERSION 4.2)

if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version" FORCE)
	set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

#------------------------------------------------------------------------------
include(version.cmake)
project(IFStile LANGUAGES CXX C VERSION ${PROJECT_VERSION})
include(CTest)

#------------------------------------------------------------------------------
function(prepend var path)
	set(listVar "")
	foreach(f ${${var}})
		list(APPEND listVar ${path}/${f})
	endforeach()
	set(${var} ${listVar} PARENT_SCOPE)
endfunction()

#https://thatonegamedev.com/cpp/cmake/how-to-embed-files-in-your-c-project/
function(generate_embed_header EMBED_HEADERS_LOCATION DST_FILE)

	cmake_parse_arguments(EF "" "" "FILES" ${ARGN})

	set(ARRAY_DECLARATIONS)

	foreach(INPUT_FILE IN LISTS EF_FILES)
		if(NOT EXISTS "${INPUT_FILE}")
			message(FATAL_ERROR "generate_embed_header: file not found: ${INPUT_FILE}")
		endif()
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${INPUT_FILE}")

		cmake_path(GET INPUT_FILE STEM LAST_ONLY FILENAME)

		set(CURRENT_COMMENT "//${FILENAME}")
		list(APPEND ARRAY_DECLARATIONS "${CURRENT_COMMENT}")
		
        string(TOLOWER "${FILENAME}" ARR_NAME)
        string(MAKE_C_IDENTIFIER "${ARR_NAME}" ARR_NAME)

        file(READ "${INPUT_FILE}" BYTES HEX)
        string(REGEX REPLACE "(..)" "0x\\1, " BYTES "${BYTES}")

        set(CURRENT_DECLARATION "constexpr uint8_t ${ARR_NAME}[] = { ${BYTES} }\;")
		list(APPEND ARRAY_DECLARATIONS "${CURRENT_DECLARATION}")
    endforeach()

    string(JOIN "\n" ARRAY_DECLARATIONS ${ARRAY_DECLARATIONS})

    set(BASIC_HEADER "\
#pragma once
namespace embed{
${ARRAY_DECLARATIONS}
}")

    file(CONFIGURE OUTPUT "${EMBED_HEADERS_LOCATION}/${DST_FILE}"
    CONTENT "${BASIC_HEADER}")
endfunction()

#------------------------------------------------------------------------------
#set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(compiler_flags.cmake)

#------------------------------------------------------------------------------
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(arch32 TRUE)
else()
    set(arch32 FALSE)
endif()

set(exe_suffix)

if(MSVC AND (CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64"
		OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64)$"))
	set(platform "msvc_arm")
	set(exe_suffix "ARM")
elseif(MSVC)
	set(platform "msvc")
	if(arch32)
		set(exe_suffix "32")
	endif()
elseif(APPLE)
	set(platform "macos")
elseif(ANDROID)
	set(platform "android")
elseif(EMSCRIPTEN)
	set(platform "wasm")
	set(exe_suffix "_${PROJECT_VERSION}")
elseif(LINUX)
	set(platform "linux")
else()
	message(FATAL_ERROR "not supported")
endif()

if(arch32)
	set(platform ${platform}_32)
endif()

set(exe_name ${PROJECT_NAME}${exe_suffix})

if (MSVC)
	set(win_directx "#define IMS_USE_DX11 1")
endif()

#-------------------------------------------------------------------------------
cmake_path(SET ext NORMALIZE "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(bin ${CMAKE_CURRENT_SOURCE_DIR}/bin)

if(EMSCRIPTEN)	
	set(bin ${bin}/wasm)
elseif (ANDROID)
    set(bin ${bin}/arm64-v8a)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${bin})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${bin})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${bin})

set(GENERATE_HEADER_LOCATION "${CMAKE_BINARY_DIR}/embed/include")
set(EMBED_HEADERS_LOCATION "${GENERATE_HEADER_LOCATION}/embed")

#-------------------------------------------------------------------------------
set (verhdr ${CMAKE_CURRENT_BINARY_DIR}/version_gen.h)

file(CONFIGURE OUTPUT ${verhdr} CONTENT
"#pragma once
#define PROJECT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR}
#define PROJECT_VERSION_MINOR ${PROJECT_VERSION_MINOR}
#define PROJECT_VERSION \"${PROJECT_VERSION}\"
${win_directx}
${dev_version}
")

include(examples.cmake)
prepend(Examples ${CMAKE_CURRENT_SOURCE_DIR}/Examples)
generate_embed_header(${EMBED_HEADERS_LOCATION} examples.h FILES "${Examples}")

generate_embed_header(${EMBED_HEADERS_LOCATION} manual.h FILES ${CMAKE_CURRENT_SOURCE_DIR}/manual.md)
generate_embed_header(${EMBED_HEADERS_LOCATION} font_awesome.h FILES "${CMAKE_CURRENT_SOURCE_DIR}/Res/forkawesome-webfont.ttf")
generate_embed_header(${EMBED_HEADERS_LOCATION} RobotoMono_Regular.h FILES "${CMAKE_CURRENT_SOURCE_DIR}/Res/RobotoMono-Regular.ttf")
generate_embed_header(${EMBED_HEADERS_LOCATION} RobotoMono_Bold.h FILES "${CMAKE_CURRENT_SOURCE_DIR}/Res/RobotoMono-Bold.ttf")
generate_embed_header(${EMBED_HEADERS_LOCATION} RobotoMono_Italic.h FILES "${CMAKE_CURRENT_SOURCE_DIR}/Res/RobotoMono-Italic.ttf")
generate_embed_header(${EMBED_HEADERS_LOCATION} icon256.png.h FILES "${CMAKE_CURRENT_SOURCE_DIR}/Res/ifstile256.png")
#-------------------------------------------------------------------------------
set (SRC_Core
full_search.h full_search.cpp ims_print.h
ims_file.h ims_file.cpp built_in_func.h built_in_func.cpp big_array.h 
parser_state.h parser_state.cpp  spline.h print_dimensions.cpp
matrix_funcs.h matrix_group.h matrix_group.cpp 
ims_keywords.h ims_operator.h ims_operator.cpp block_class.h block_class.cpp 
data_column.cpp data_column.h def_number_types.h ims_random.h ims_random.cpp 
ims_stage.h ims_stage.cpp
ims_info.h ims_info.cpp call_thread.h aifs_printer.h aifs_printer.cpp
search_info.h print_operator.cpp oper_block.h oper_block.cpp
ovr_data.h ovr_data.cpp eval_context.h eval_context.cpp
geom_solver.h dist_solver.h dist_solver.cpp diam_solver.h diam_solver.cpp 
clock_print.h block_info.h block_info.cpp
dfs.h dfs.cpp ims_graph_base.h ims_graph_base.cpp ims_graph.cpp ims_graph.h
conbuf.h conbuf.cpp version.h ims_num_traits.cpp geometry.h
 ifs_data_text.h  ifs_data_text.cpp  variator.h variator.cpp
integer_ims.h integer_ims.cpp error_helper.h error_helper.cpp
ims_num_traits.h ims_val_b.h ims_val.h ims_val.cpp throw_exception.cpp
block_form.h block_form.cpp inter_type.h inter_type.cpp
neighbors_data.h neighbors_data.cpp math_helpers.h mprec.h
poly_funcs.h poly_roots.h combinator.h combinator.cpp
finder.h finder.cpp column_id.h pch_core.h pch.h
columns.h columns.cpp graph_init_data.h graph_init_data.cpp
ifs_list.h ifs_list.cpp
projector.h projector.cpp env_block.h env_block.cpp ims_identifiers.cpp ims_identifiers.h
ims_full_graph.h ims_full_graph.cpp
utf8.h utf8.cpp main_utf8.h ims_chrono.h ims_ini.h ims_ini.cpp
block_converter.h block_converter.cpp graph_poly.h js_engine.h js_engine.cpp
operator_ptr.h operator_ptr.cpp  palette.h palette.cpp ims_visitor.h 
ast_stack.h ast_stack.cpp proj_data.cpp proj_data.h
ims_streambuf.h render_params.h ims_pool.h variable.h
print_ims_val.cpp graph_divider.h graph_divider.cpp
eval_helpers.h eval_helpers.cpp report_params.h derived_ifs.cpp derived_ifs.h
eval_pool.h eval_pool.cpp ims_view.h ast_maps.h ast_maps.cpp
test_alloc_hook.h test_alloc_hook.cpp edge_ball.h edge_ball.cpp
edge_map.h dyn_mat_vec.h ims_number.h cardinality.h
pool_ptr.h pool_ptr.cpp graph_builder.h graph_builder.cpp string_view_hash.h
block_graph.h block_vars.cpp indexed_maps.h indexed_maps.cpp
param_walker.h param_walker.cpp
mat_operations.h matrix_helper.h matrix_helper.cpp
box.h ball3d.h ifs_metrics.h 
affine_dim_calc.h affine_dim_calc.cpp 
affine_bound_calc.h affine_bound_calc.cpp 
affine_point_calc.h affine_point_calc.cpp
affine_subspace.h affine_subspace.cpp samples.h samples.cpp
aifs_load.h aifs_load.cpp
exr.h exr.cpp  base64.h base64.cpp
ims_bitmap.h ims_bitmap.cpp ims_image.h
mesh_primitives.h mesh3d.cpp mesh3d.h point.h voxel_volume.cpp voxel_volume.h voxel.h
octree.cpp octree.h 
lqsort.h builder.h builder.cpp builder3d.h
gbuffer3d.h gbuffer3d.cpp builder3d.cpp builder2d.h builder2d.cpp builder_ext.h builder_ext.cpp
build_data.h build_data.cpp builder_mesh.h builder_mesh.cpp 
standard_vars.h standard_vars.cpp ifs_renderer.h ifs_renderer.cpp
png_support.h png_support.cpp png_impl.c
animator.h animator.cpp create_block.cpp create_ifs.cpp
creator_state.h creator_state.cpp
ims_to_string_ex.h ims_to_string_ex.cpp
flame_calc.h flame_calc.cpp
info_printer.h info_printer.cpp save_type.h  draw_task.h
ims_help.h ims_help.cpp
visible_blocks.h visible_blocks.cpp  block_sets.h block_sets.cpp
)

prepend(SRC_Core SrcCore)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/SrcCore PREFIX "SrcCore" FILES ${SRC_Core})
#------------------------------------------------------------------------------
set (SRC_QuickJS 
	cutils.c cutils.h libregexp.c libregexp.h  libregexp-opcode.h
	libunicode.c  libunicode.h libunicode-table.h quickjs.c quickjs.h list.h
	dtoa.h dtoa.c
)
prepend(SRC_QuickJS quickjs)
#------------------------------------------------------------------------------
set (SRC_Tiny
	cute_png.h tinyexr.h utf8_decoder.h 
	md4c.h md4c.c
	miniz.h miniz.c IconsForkAwesome.h
)
if(MSVC)
	list (APPEND SRC_Tiny dirent/dirent.h)
endif()
prepend(SRC_Tiny Tiny)
#------------------------------------------------------------------------------
set (SRC_GENERAL
ims_settings.h ims_settings.cpp
pch.h main.cpp main_utf8.cpp
gl_helper.h gl_helper.cpp dx_helper.h
js_ifs.cpp ims_rw.h ims_rw.cpp
file_dialog_state.h file_dialog_state.cpp 
thread_priority.cpp platform.h platform.cpp 
platform_windows.cpp platform_macos.cpp platform_android.cpp platform_wasm.cpp
platform_linux.cpp program_state.h program_state.cpp
ui_window.h ui_window.cpp gui.h gui.cpp ims_window_drag.h ims_window_drag.cpp
ui_creator.h ui_creator.cpp ui_animation.h ui_animation.cpp
ui_examples.h ui_examples.cpp ui_colum_info.cpp ui_finder.h ui_finder.cpp
ui_search_session.h ui_search_session.cpp ui_clipboard.h ui_clipboard.cpp
ui_file_dialog.h ui_file_dialog.cpp ui_about.h ui_about.cpp
ui_palette.h ui_palette.cpp ui_location.h ui_location.cpp ui_components.h
ui_components.cpp ui_filter_columns.h ui_filter_columns.cpp
ui_settings.h ui_settings.cpp ui_modal_msg.h ui_modal_msg.cpp ui_source.h
ui_source.cpp ui_help.h ui_help.cpp ui_fonts.h ui_fonts.cpp ui_ifs_list.h
ui_ifs_list.cpp ui_sel_visible.h ui_sel_visible.cpp ui_render_par.h
ui_render_par.cpp ui_console.h ui_console.cpp ui_editor.h ui_editor.cpp
ui_utils.h ui_utils.cpp ims_markdown.h ims_markdown.cpp icons.cpp
font_icons.cpp font_regular.cpp font_bold.cpp font_italic.cpp
ims_val_widget.h ims_val_widget.cpp edit_helper.h edit_helper.cpp
ims_worker.h ims_worker.cpp
)
prepend(SRC_GENERAL Src)

if(MSVC)
	list(APPEND SRC_GENERAL PlatDep/resource.rc PlatDep/cpp.hint)
endif()

list(APPEND SRC_GENERAL 
${CMAKE_CURRENT_SOURCE_DIR}/Tiny/imgui_md.h 
${CMAKE_CURRENT_SOURCE_DIR}/Tiny/imgui_md.cpp)

#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Src PREFIX "Src" FILES ${SRC_GENERAL})
#------------------------------------------------------------------------------
set (SRC_IMGUI 
imgui.h imgui.cpp imconfig.h imgui_internal.h imgui_demo.cpp imgui_draw.cpp
 imgui_widgets.cpp imgui_tables.cpp imstb_truetype.h imstb_rectpack.h  imstb_textedit.h
 backends/imgui_impl_sdl3.h backends/imgui_impl_sdl3.cpp
 misc/cpp/imgui_stdlib.h misc/cpp/imgui_stdlib.cpp
 misc/debuggers/imgui.natvis
)

if(win_directx)
	set (SRC_IMGUI  ${SRC_IMGUI} 
		backends/imgui_impl_dx11.h backends/imgui_impl_dx11.cpp)
elseif(EMSCRIPTEN OR ANDROID) 
	set (SRC_IMGUI  ${SRC_IMGUI} 
		backends/imgui_impl_opengl3.h backends/imgui_impl_opengl3.cpp)
else()
	set (SRC_IMGUI  ${SRC_IMGUI} 
		backends/imgui_impl_opengl2.h backends/imgui_impl_opengl2.cpp)
endif()

prepend(SRC_IMGUI imgui)
source_group(ImGui FILES ${SRC_IMGUI})
#-------------------------------------------------------------------------------
if(MSVC_IDE)
	set (NATVIS_FILES
		boost.natvis boost_Containers.natvis boost_DateTime.natvis boost_Geometry.natvis 
		boost_Gil.natvis boost_IntrusiveContainers.natvis boost_MultiArray.natvis 
		boost_Multiprecision.natvis boost_PointerContainerLibrary.natvis boost_Rational.natvis
		boost_Regex.natvis boost_SmartPointers.natvis boost_uBLAS.natvis boost_Unordered.natvis
		boost_Variant.natvis directx.natvis eigen.natvis)
	prepend(NATVIS_FILES natvis)
	set (NATVIS_FILES ${NATVIS_FILES} imgui/misc/debuggers/imgui.natvis)
	source_group(natvis FILES ${NATVIS_FILES})
endif()

#-------------------------------------------------------------------------------
set(ALL_SRC 
	${SRC_GENERAL} ${SRC_IMGUI} ${NATVIS_FILES}	${verhdr} manual.md)

if(MSVC_IDE)
    list(APPEND ALL_SRC ifstile.natvis)
endif()
#-------------------------------------------------------------------------------

add_library(IFScore STATIC ${SRC_Core})
target_precompile_headers(IFScore PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/SrcCore/pch.h  ${verhdr}
)

target_include_directories(IFScore BEFORE PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(IFScore AFTER PUBLIC SrcCore)#AFTER - because pch.h
target_include_directories(IFScore PUBLIC
	${ext}
	${ext}/fmt/include
	Tiny
)
target_include_directories(IFScore PUBLIC ${GENERATE_HEADER_LOCATION})
#------------------------------------------------------------------------------
add_library(QuickJS STATIC ${SRC_QuickJS})
target_include_directories(QuickJS PUBLIC quickjs)

add_library(TinyLib STATIC ${SRC_Tiny})
target_include_directories(TinyLib PUBLIC Tiny)
#------------------------------------------------------------------------------
#disable warnings for 3rd party code
if(MSVC)
	target_compile_options(QuickJS PRIVATE /W0)
	target_compile_options(TinyLib PRIVATE /W0)
else()
	target_compile_options(QuickJS PRIVATE -w)
	target_compile_options(TinyLib PRIVATE "-w;-Wno-#pragma-messages")
endif()
#------------------------------------------------------------------------------

if(MSVC)
	add_executable(${PROJECT_NAME} WIN32 ${ALL_SRC})

	target_link_options(${PROJECT_NAME} PRIVATE
		/VERSION:${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
	
	add_custom_command(
		TARGET ${PROJECT_NAME}
		POST_BUILD
		COMMAND "mt.exe" -manifest 
			\"${CMAKE_CURRENT_SOURCE_DIR}\\PlatDep\\ifstile.manifest\" 
			\"${CMAKE_CURRENT_SOURCE_DIR}\\PlatDep\\dpiaware.manifest\" 
			-outputresource:\"$<TARGET_FILE:${PROJECT_NAME}>\"\;\#1
		COMMENT "Adding manifests..." 
	)

	set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME} )
elseif(APPLE)

	# NOTE: Don't include the path in MACOSX_BUNDLE_ICON_FILE -- this is
	# the property added to Info.plist
	set(MACOSX_BUNDLE_ICON_FILE Application.icns)

	set( MACOSX_BUNDLE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
	set( MACOSX_BUNDLE_SHORT_VERSION_STRING ${MACOSX_BUNDLE_VERSION})
	set( MACOSX_BUNDLE_BUNDLE_VERSION ${MACOSX_BUNDLE_VERSION})
	set( MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${MACOSX_BUNDLE_VERSION}" )
	set( MACOSX_BUNDLE_COPYRIGHT "Copyright (C) 2016-2026, Dmitry Mekhontsev. All Rights Reserved.")
	set( MACOSX_BUNDLE_GUI_IDENTIFIER "com.ifstile")
#	set( MACOSX_BUNDLE_ADDITIONAL "<key>NSPrincipalClass</key>\n<string>NSApplication</string>") 

	set(resFiles
		"${CMAKE_CURRENT_SOURCE_DIR}/PlatDep/Resources/Application.icns"
	)
	set_property(
	  SOURCE ${resFiles} 
	  PROPERTY MACOSX_PACKAGE_LOCATION Resources
	)

	add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${ALL_SRC} ${resFiles})
	
	#get_dotapp_dir(${APPLICATION_BIN} APPLICATION_APP_LOCATION) 

elseif(ANDROID)

	add_library(${PROJECT_NAME} SHARED ${ALL_SRC})
		
elseif(EMSCRIPTEN)
	#set(CMAKE_EXECUTABLE_SUFFIX ".html")
	add_executable(${PROJECT_NAME} ${ALL_SRC})	
elseif(LINUX)
	add_executable(${PROJECT_NAME} ${ALL_SRC})

	set_target_properties(${PROJECT_NAME} PROPERTIES 
		LINK_SEARCH_START_STATIC 1
		LINK_SEARCH_END_STATIC 1
	)
endif()

#-------------------------------------------------------------------------------
# Setup include

target_include_directories(${PROJECT_NAME} BEFORE PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(${PROJECT_NAME} AFTER PUBLIC SrcCore)#AFTER - because pch.h
target_include_directories(${PROJECT_NAME} PUBLIC
	Tiny 
	Src 
	imgui 
	imgui/misc/cpp 
	imgui/backends
	${ext} 
	${ext}/fmt/include 
	${ext}/SDL/include
)
target_include_directories(${PROJECT_NAME} PUBLIC ${GENERATE_HEADER_LOCATION})

target_precompile_headers(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Src/pch.h
)

if(EMSCRIPTEN)
	file(MAKE_DIRECTORY ${bin})
	set (wasm_files ${CMAKE_CURRENT_SOURCE_DIR}/Setup/wasm)
	set (NAME_VER "${exe_name}")
	configure_file(${wasm_files}/sw.js ${bin})
	configure_file(${wasm_files}/index.html ${bin})
	configure_file(${wasm_files}/manifest.json ${bin} COPYONLY)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/PlatDep/Resources/ifstile192mask.png ${bin} COPYONLY)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/PlatDep/Resources/ifstile512.png ${bin} COPYONLY)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/PlatDep/Resources/favicon.ico ${bin} COPYONLY)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/PlatDep/Resources/apple-touch-icon.png ${bin} COPYONLY)
	
elseif(NOT ANDROID)
	set_target_properties (${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX d)
endif()


set_target_properties (${PROJECT_NAME} PROPERTIES
	OUTPUT_NAME ${exe_name}
#	PREFIX ""
)

#-------------------------------------------------------------------------------
#strip
if(NOT EMSCRIPTEN AND NOT MSVC)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND $<$<CONFIG:Release>:${CMAKE_STRIP}$<SEMICOLON>$<TARGET_FILE:${PROJECT_NAME}>>
	COMMAND_EXPAND_LISTS
	COMMENT "Stripping ${PROJECT_NAME} for Release..."
)
endif()
#-------------------------------------------------------------------------------
#link other
if(MSVC)
	set (linkto SDL3.lib)
else()
	set (linkto libSDL3.a)
endif()

set (lib ${ext}/lib/${platform})
set (LINK_LIST)
foreach(x ${linkto})
	list(APPEND LINK_LIST
		"$<$<CONFIG:Debug>:${lib}/Debug/${x}>"
		"$<$<CONFIG:Release>:${lib}/Release/${x}>")
endforeach()

#-------------------------------------------------------------------------------
if(MSVC)
	list (APPEND LINK_LIST 
		setupapi.lib winmm.lib comctl32.lib version.lib ole32.lib 
		comdlg32.lib shlwapi.lib Imm32.lib)
	if (win_directx)
		list (APPEND LINK_LIST D3D11.lib)
	else()
		list (APPEND LINK_LIST opengl32.lib)
	endif()
elseif(ANDROID)
	list (APPEND LINK_LIST 
		EGL GLESv3 GLESv1_CM dl android log OpenSLES)
elseif(EMSCRIPTEN)
	list (APPEND LINK_LIST 
		EGL)
elseif(APPLE)
	list (APPEND LINK_LIST 
		iconv objc)
	list (APPEND LINK_LIST 
		"-framework audiotoolbox" "-framework Foundation" "-framework Cocoa" 
		"-framework CoreAudio" "-framework audiounit" "-framework CoreVideo" 
		"-framework ForceFeedback" "-framework IOKit" "-framework Carbon" 
		"-framework OpenGL" "-framework CoreHaptics" "-framework GameController"
		"-framework Metal" "-framework QuartzCore" "-framework AVFoundation"
		"-framework CoreMedia" "-framework UniformTypeIdentifiers"
	)
elseif(LINUX)
	list (APPEND LINK_LIST 
		GL dl pthread rt)
endif()	

target_link_libraries(${PROJECT_NAME} PRIVATE IFScore QuickJS TinyLib ${LINK_LIST})


#-----------------------------------Finder--------------------------------------
if(NOT EMSCRIPTEN AND NOT ANDROID)
add_executable(IFSfinder 
	SrcFinder/main.cpp SrcFinder/main_utf8.cpp SrcFinder/pch.h ${verhdr}
)

# Setup include
target_include_directories(IFSfinder BEFORE PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(IFSfinder AFTER PUBLIC SrcCore)#AFTER - because pch.h
target_include_directories(IFSfinder PUBLIC 
	Tiny 
	SrcFinder 
	${ext} 
	${ext}/fmt/include
)

target_precompile_headers(IFSfinder PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/SrcFinder/pch.h
)

target_link_libraries(IFSfinder PRIVATE IFScore QuickJS TinyLib)

if(LINUX)
	target_link_libraries(IFSfinder PRIVATE pthread)
	set_target_properties(IFSfinder PROPERTIES
		LINK_SEARCH_START_STATIC 1
		LINK_SEARCH_END_STATIC 1
	)
endif()	

set_target_properties(IFSfinder PROPERTIES
	DEBUG_POSTFIX d
	OUTPUT_NAME IFSfinder${exe_suffix}
)
endif()

#------------------------------------Tests--------------------------------------
if(NOT EMSCRIPTEN AND NOT ANDROID)
add_executable(Tests 
	SrcTests/main.cpp SrcTests/main_utf8.cpp SrcTests/pch.h
	SrcTests/test.cpp SrcTests/spline.cpp
	SrcTests/aifs_simple.cpp SrcTests/aifs_examples.cpp
	SrcTests/aifs_tester.h SrcTests/aifs_tester.cpp
	SrcTests/render_tester.h SrcTests/render_tester.cpp
	${ext}/googletest/googletest/src/gtest-all.cc
	${ext}/googletest/googlemock/src/gmock-all.cc
	${verhdr})

# Setup include
target_include_directories(Tests BEFORE PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(Tests AFTER PUBLIC SrcCore)#AFTER - because pch.h
target_include_directories(Tests PUBLIC 
	Tiny 
	SrcTests 
	${ext} 
	${ext}/fmt/include
	${ext}/googletest/googletest
	${ext}/googletest/googletest/include
	${ext}/googletest/googlemock
	${ext}/googletest/googlemock/include
)


target_precompile_headers(Tests PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/SrcTests/pch.h
)

target_link_libraries(Tests PRIVATE IFScore QuickJS TinyLib)

if(LINUX)
	target_link_libraries(Tests PRIVATE pthread)
	set_target_properties(Tests PROPERTIES
		LINK_SEARCH_START_STATIC 1
		LINK_SEARCH_END_STATIC 1
	)
endif()

include(GoogleTest)
gtest_discover_tests(Tests)

set_target_properties(Tests PROPERTIES
	DEBUG_POSTFIX d
	OUTPUT_NAME Tests${exe_suffix}
)
endif()
