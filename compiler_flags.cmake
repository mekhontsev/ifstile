set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

if (MSVC)
	add_compile_options(/W4)
	add_compile_options(/utf-8 /Zc:char8_t- /MP /GR- /bigobj /fp:except- /Zm256 /permissive-)
	add_compile_options(/experimental:c11atomics)# TODO: remove when will be supported
	add_compile_options("$<$<CONFIG:Release>:/Oi;/Ot;/Oy;/GT;/Gy;/GF;/GS->")
	#add_compile_options("$<$<CONFIG:Debug>:/fsanitize=address>")
	add_definitions(/D_HAS_STATIC_RTTI=0 -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN)
	#add_definitions(/arch:AVX2)
	add_link_options(/STACK:10000000 /LARGEADDRESSAWARE	/MANIFEST:NO)
	add_link_options("$<$<CONFIG:Release>:/OPT:REF;/OPT:ICF>")
	add_link_options("$<$<CONFIG:Release>:/LTCG>") #https://gitlab.kitware.com/cmake/cmake/-/issues/20484
else()
	add_compile_options(-Wall -Wextra -Wpedantic -fno-math-errno)
	add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fno-char8_t>")
	#add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>")
	add_compile_options("$<$<CONFIG:Release>:-ffunction-sections;-fdata-sections;-fno-stack-protector;-fvisibility=hidden;-U_FORTIFY_SOURCE;-D_FORTIFY_SOURCE=0>")
	add_compile_options("$<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-fvisibility-inlines-hidden>")
endif()

if(APPLE)
	add_compile_options(-stdlib=libc++)
	#set(LLVM_ENABLE_LIBCXX ON)
elseif(ANDROID)
	#set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	#add_definitions(-DSDL_DYNAMIC_API=0)
	add_link_options("$<$<CONFIG:RELEASE>:-Wl,--exclude-libs,ALL -Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/PlatDep/android.map>")
	add_link_options(-static-libstdc++)
elseif(EMSCRIPTEN)
	add_compile_options(-fwasm-exceptions -mnontrapping-fptoint)
	add_compile_options("$<$<CONFIG:Debug>:-g;-gseparate-dwarf>")

	add_compile_options(-pthread -Wno-pthreads-mem-growth)
	add_link_options(-pthread -Wno-pthreads-mem-growth)

	#add_compile_options("$<$<CONFIG:Release>:-fexceptions;-UNDEBUG;-s;-gseparate-dwarf;-g>")#-fno-inline	
	#add_link_options("$<$<CONFIG:Release>:-sGL_DEBUG=1;-sASSERTIONS=1;-sNO_DISABLE_EXCEPTION_CATCHING>")
	#add_link_options("$<$<CONFIG:Debug>:-sGL_DEBUG=1;-sASSERTIONS=1;-sNO_DISABLE_EXCEPTION_CATCHING>")
	
	add_link_options("$<$<CONFIG:Debug>:-sGL_DEBUG=1;-sASSERTIONS=1>")

	add_link_options(
	-lidbfs.js
	-lhtml5
	-lGL
	#-sERROR_ON_UNDEFINED_SYMBOLS=0
	#-sSTRICT=1 
	-sSTACK_SIZE=10Mb
	-sMIN_WEBGL_VERSION=2
	-sMAX_WEBGL_VERSION=2
	-sINITIAL_MEMORY=64MB
	-sMAXIMUM_MEMORY=2gb
	-sALLOW_MEMORY_GROWTH=1
	-sPTHREAD_POOL_SIZE=4
	-sEXPORTED_RUNTIME_METHODS=ccall,cwrap)

elseif(LINUX)
	add_compile_options("$<$<CONFIG:Release>:-ffat-lto-objects>")
	add_link_options("$<$<CONFIG:Debug>:-s;-Wl,--gc-sections>")
	add_link_options(-static-libstdc++ -static-libgcc)
endif()