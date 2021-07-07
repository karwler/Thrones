cmake_minimum_required(VERSION 3.10.2)
project(Thrones LANGUAGES CXX)
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# target names and source/resource files

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
set(SERVER_NAME "Server")
set(DATA_NAME "assets")
set(ADATA_NAME "${DATA_NAME}_android")
set(OVEN_NAME "oven")
set(TLIB_NAME "tlib")
set(TESTS_NAME "tests")

set(ASSET_WAV
	"rsc/audio/ammo.wav"
	"rsc/audio/move.wav")

set(ASSET_DAT
	"rsc/data/cursor.png"
	"rsc/data/merriweather.otf"
	"rsc/data/romanesque.ttf"
	"rsc/data/thrones.png")

set(ASSET_MTL
	"rsc/materials/misc.mtl"
	"rsc/materials/pieces.mtl"
	"rsc/materials/tiles.mtl")

set(ASSET_OBJ
	"rsc/objects/breached.obj"
	"rsc/objects/catapult.obj"
	"rsc/objects/crossbowmen.obj"
	"rsc/objects/dragon.obj"
	"rsc/objects/elephant.obj"
	"rsc/objects/fortress.obj"
	"rsc/objects/ground.obj"
	"rsc/objects/lancer.obj"
	"rsc/objects/outline.obj"
	"rsc/objects/plane.obj"
	"rsc/objects/rangers.obj"
	"rsc/objects/screen.obj"
	"rsc/objects/spearmen.obj"
	"rsc/objects/table.obj"
	"rsc/objects/throne.obj"
	"rsc/objects/tile.obj"
	"rsc/objects/trebuchet.obj"
	"rsc/objects/warhorse.obj")

set(ASSET_SHD
	"rsc/shaders/depth.frag"
	"rsc/shaders/depth.geom"
	"rsc/shaders/depth.vert"
	"rsc/shaders/geometry.frag"
	"rsc/shaders/geometry.vert"
	"rsc/shaders/gui.frag"
	"rsc/shaders/gui.vert")

set(ASSET_IMG
	"rsc/textures/assault.png"
	"rsc/textures/catapult.png"
	"rsc/textures/city.png"
	"rsc/textures/conspire.png"
	"rsc/textures/crossbowmen.png"
	"rsc/textures/deceive.png"
	"rsc/textures/dragon.png"
	"rsc/textures/elephant.png"
	"rsc/textures/farm.png"
	"rsc/textures/forest.png"
	"rsc/textures/fortress.png"
	"rsc/textures/grass.png"
	"rsc/textures/hasten.png"
	"rsc/textures/lancer.png"
	"rsc/textures/spearmen.png"
	"rsc/textures/mountain.png"
	"rsc/textures/plains.png"
	"rsc/textures/rangers.png"
	"rsc/textures/reticle.png"
	"rsc/textures/rock.png"
	"rsc/textures/throne.png"
	"rsc/textures/title.png"
	"rsc/textures/trebuchet.png"
	"rsc/textures/wall.png"
	"rsc/textures/warhorse.png"
	"rsc/textures/water.png")

set(DATA_SRC ${ASSET_WAV} ${ASSET_DAT} ${ASSET_MTL} ${ASSET_OBJ} ${ASSET_SHD} ${ASSET_IMG})

set(THRONES_SRC
	"src/engine/audioSys.cpp"
	"src/engine/audioSys.h"
	"src/engine/fileSys.cpp"
	"src/engine/fileSys.h"
	"src/engine/inputSys.cpp"
	"src/engine/inputSys.h"
	"src/engine/scene.cpp"
	"src/engine/scene.h"
	"src/engine/windowSys.cpp"
	"src/engine/windowSys.h"
	"src/engine/world.cpp"
	"src/engine/world.h"
	"src/oven/oven.cpp"
	"src/oven/oven.h"
	"src/prog/board.cpp"
	"src/prog/board.h"
	"src/prog/game.cpp"
	"src/prog/game.h"
	"src/prog/guiGen.cpp"
	"src/prog/guiGen.h"
	"src/prog/netcp.cpp"
	"src/prog/netcp.h"
	"src/prog/program.cpp"
	"src/prog/program.h"
	"src/prog/progs.cpp"
	"src/prog/progs.h"
	"src/prog/types.cpp"
	"src/prog/types.h"
	"src/server/server.cpp"
	"src/server/server.h"
	"src/utils/alias.h"
	"src/utils/context.cpp"
	"src/utils/context.h"
	"src/utils/layouts.cpp"
	"src/utils/layouts.h"
	"src/utils/objects.cpp"
	"src/utils/objects.h"
	"src/utils/settings.cpp"
	"src/utils/settings.h"
	"src/utils/text.cpp"
	"src/utils/text.h"
	"src/utils/utils.cpp"
	"src/utils/utils.h"
	"src/utils/widgets.cpp"
	"src/utils/widgets.h")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	list(APPEND THRONES_SRC "rsc/thrones.rc")
endif()
list(APPEND THRONES_SRC ${ASSET_SHD})

set(SERVER_SRC
	"src/server/log.cpp"
	"src/server/log.h"
	"src/server/server.cpp"
	"src/server/server.h"
	"src/server/serverProg.cpp"
	"src/utils/alias.h"
	"src/utils/text.cpp"
	"src/utils/text.h")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	list(APPEND SERVER_SRC "rsc/server.rc")
endif()

set(OVEN_SRC
	"src/oven/oven.cpp"
	"src/oven/oven.h"
	"src/oven/ovenProg.cpp"
	"src/utils/alias.h"
	"src/utils/text.cpp"
	"src/utils/text.h")

set(TESTS_SRC
	"src/test/alias.cpp"
	"src/test/fileSys.cpp"
	"src/test/oven.cpp"
	"src/test/server.cpp"
	"src/test/tests.cpp"
	"src/test/tests.h"
	"src/test/text.cpp"
	"src/test/utils.cpp")

# dependencies

option(EXTERNAL "Save settings externally." ON)
option(OPENGLES "Build for OpenGL ES. (always on with Emscripten)" OFF)
option(SERVICE "Build server without keyboard input." OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	option(NATIVE "Build for the current CPU." OFF)
endif()
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	option(APPIMAGE "Package as an AppImage." OFF)
endif()

set(VER_SDL "2.0.14" CACHE STRING "SDL2 version.")
set(VER_IMG "2.0.5" CACHE STRING "SDL2_image version.")
set(VER_TTF "2.0.15" CACHE STRING "SDL2_ttf version.")
set(VER_GLEW "2.1.0" CACHE STRING "GLEW version.")
set(VER_GLM "0.9.9.8" CACHE STRING "glm version.")
set(VER_CURL "7.74.0" CACHE STRING "libcurl version.")
set(DIR_LIB "${CMAKE_SOURCE_DIR}/lib")
set(DIR_RSC "${CMAKE_SOURCE_DIR}/rsc")

function(downloadLib URL LIBDIR ORIG_LIBDIR)
	if(EXISTS "${LIBDIR}")
		message(STATUS "Found ${LIBDIR}")
		return()
	endif()

	get_filename_component(FILE "${URL}" NAME)
	get_filename_component(DST "${LIBDIR}" DIRECTORY)
	if(EXISTS "${DST}/${FILE}")
		message(STATUS "Found ${DST}/${FILE}")
	else()
		message(STATUS "Downloading ${URL}")
		file(DOWNLOAD "${URL}" "${DST}/${FILE}")
	endif()

	if(FILE MATCHES "\.(tar\.gz|tgz|zip)$")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xzf "${FILE}" WORKING_DIRECTORY "${DST}")
		if(ORIG_LIBDIR AND NOT LIBDIR STREQUAL ORIG_LIBDIR)
			file(RENAME "${ORIG_LIBDIR}" "${LIBDIR}")
		endif()
	elseif(FILE MATCHES "\.dmg$")
		execute_process(COMMAND hdiutil attach "${DST}/${FILE}")
		string(FIND "${FILE}" "-" FID)
		string(SUBSTRING "${FILE}" 0 ${FID} FNAME)
		file(COPY "/Volumes/${FNAME}/${FNAME}.framework" DESTINATION "${DST}")
	endif()
endfunction()

function(compileLib URL LIBDIR CMDV WDIR ORIG_LIBDIR)
	if(EXISTS "${LIBDIR}")
		message(STATUS "Found ${LIBDIR}")
	else()
		downloadLib("${URL}" "${LIBDIR}" "${ORIG_LIBDIR}")
		foreach(CMD IN LISTS CMDV)
			execute_process(COMMAND ${CMD} WORKING_DIRECTORY "${WDIR}")
		endforeach()
	endif()
endfunction()

function(renameLibdir SRC DST)
	if(EXISTS "${SRC}")
		file(RENAME "${SRC}" "${DST}")
	endif()
endfunction()

function(copyRename SRC DST)
	get_filename_component(SFIL "${SRC}" NAME)
	get_filename_component(DDIR "${DST}" DIRECTORY)
	file(COPY "${SRC}" DESTINATION "${DDIR}")
	file(RENAME "${DDIR}/${SFIL}" "${DST}")
endfunction()

file(MAKE_DIRECTORY "${DIR_LIB}")
downloadLib("https://github.com/g-truc/glm/releases/download/${VER_GLM}/glm-${VER_GLM}.zip" "${DIR_LIB}/glm" "")
include_directories("${CMAKE_SOURCE_DIR}/src" "${DIR_LIB}/glm")
if(EMSCRIPTEN)
	file(COPY "${DIR_RSC}/thrones.html" DESTINATION "${CMAKE_BINARY_DIR}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
	set(OVEN_DIR "${PBOUT_DIR}")
	set(DATA_DIR "${PBOUT_DIR}/share")
	set(FONTS_DIR "${DATA_DIR}/fonts")
	set(LICN_DIR "${PBOUT_DIR}/licenses")
	set(DOCS_DIR "${PBOUT_DIR}/doc")
	set(LIBS_DIR "${PBOUT_DIR}")

	if(MSVC)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(VMACHINE "x64")
		else()
			set(VMACHINE "x86")
		endif()
		set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}-msvc")
		set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}-msvc")
		set(DIR_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}-msvc")
		set(DIR_GLEW "${DIR_LIB}/glew-${VER_GLEW}-msvc")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}-${VMACHINE}-msvc")
		set(OUT_CURL "${DIR_CURL}/builds/libcurl-vc-${VMACHINE}-release-dll-ipv6-sspi-schannel")
		set(BIN_SDL "${DIR_SDL}/lib/${VMACHINE}")
		set(BIN_IMG "${DIR_IMG}/lib/${VMACHINE}")
		set(BIN_TTF "${DIR_TTF}/lib/${VMACHINE}")
		set(BIN_CURL "${OUT_CURL}/bin")
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(LIB_GLEW "${DIR_GLEW}/lib/Release/x64")
		else()
			set(LIB_GLEW "${DIR_GLEW}/lib/Release/Win32")
		endif()
		set(LIC_SDL "${DIR_SDL}")
		set(LIC_IMG "${DIR_IMG}")
		set(LIC_TTF "${DIR_TTF}")

		downloadLib("https://www.libsdl.org/release/SDL2-devel-${VER_SDL}-VC.zip" "${DIR_SDL}" "${DIR_LIB}/SDL2-${VER_SDL}")
		downloadLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-${VER_IMG}-VC.zip" "${DIR_IMG}" "${DIR_LIB}/SDL2_image-${VER_IMG}")
		downloadLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-${VER_TTF}-VC.zip" "${DIR_TTF}" "${DIR_LIB}/SDL2_ttf-${VER_TTF}")
		downloadLib("https://netix.dl.sourceforge.net/project/glew/glew/2.1.0/glew-${VER_GLEW}-win32.zip" "${DIR_GLEW}" "${DIR_LIB}/glew-${VER_GLEW}")
		compileLib("https://curl.haxx.se/download/curl-${VER_CURL}.zip" "${DIR_CURL}" "nmake\;/f\;Makefile.vc\;mode=dll\;MACHINE=${VMACHINE}" "${DIR_CURL}/winbuild" "${DIR_LIB}/curl-${VER_CURL}")
		link_directories("${BIN_SDL}" "${BIN_IMG}" "${BIN_TTF}" "${LIB_GLEW}")
		include_directories("${DIR_SDL}/include" "${DIR_IMG}/include" "${DIR_TTF}/include" "${DIR_GLEW}/include" "${OUT_CURL}/include")
	else()
		set(LIC_SDL "${DIR_LIB}/SDL2-${VER_SDL}-mingw")
		set(LIC_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}-mingw")
		set(LIC_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}-mingw")
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(VMACHINE "win64")
			set(DIR_SDL "${LIC_SDL}/x86_64-w64-mingw32")
			set(DIR_IMG "${LIC_IMG}/x86_64-w64-mingw32")
			set(DIR_TTF "${LIC_TTF}/x86_64-w64-mingw32")
		else()
			set(VMACHINE "win32")
			set(DIR_SDL "${LIC_SDL}/i686-w64-mingw32")
			set(DIR_IMG "${LIC_IMG}/i686-w64-mingw32")
			set(DIR_TTF "${LIC_TTF}/i686-w64-mingw32")
		endif()
		set(DIR_GLEW "${DIR_LIB}/glew-${VER_GLEW}-${VMACHINE}-mingw")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}-${VMACHINE}-mingw")
		set(BIN_SDL "${DIR_SDL}/bin")
		set(BIN_IMG "${DIR_IMG}/bin")
		set(BIN_TTF "${DIR_TTF}/bin")
		set(BIN_CURL "${DIR_CURL}/bin")

		downloadLib("https://www.libsdl.org/release/SDL2-devel-${VER_SDL}-mingw.tar.gz" "${LIC_SDL}" "${DIR_LIB}/SDL2-${VER_SDL}")
		downloadLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-${VER_IMG}-mingw.tar.gz" "${LIC_IMG}" "${DIR_LIB}/SDL2_image-${VER_IMG}")
		downloadLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-${VER_TTF}-mingw.tar.gz" "${LIC_TTF}" "${DIR_LIB}/SDL2_ttf-${VER_TTF}")
		compileLib("https://jztkft.dl.sourceforge.net/project/glew/glew/${VER_GLEW}/glew-${VER_GLEW}.zip" "${DIR_GLEW}" "mingw32-make\;glew.lib.static" "${DIR_GLEW}" "${DIR_LIB}/glew-${VER_GLEW}")
		downloadLib("https://curl.haxx.se/windows/dl-${VER_CURL}/curl-${VER_CURL}-${VMACHINE}-mingw.zip" "${DIR_CURL}" "")
		link_directories("${DIR_SDL}/lib" "${DIR_IMG}/lib" "${DIR_TTF}/lib" "${DIR_GLEW}/lib")
		include_directories("${DIR_SDL}/include/SDL2" "${DIR_IMG}/include/SDL2" "${DIR_TTF}/include/SDL2" "${DIR_GLEW}/include" "${DIR_CURL}/include")
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents")
	set(OVEN_DIR "${CMAKE_BINARY_DIR}/bin")
	set(DATA_DIR "${PBOUT_DIR}/Resources")
	set(FONTS_DIR "${DATA_DIR}/fonts")
	set(LICN_DIR "${DATA_DIR}/licenses")
	set(DOCS_DIR "${DATA_DIR}/doc")
	set(LIBS_DIR "${PBOUT_DIR}/Frameworks")
	set(FRAM_SDL "SDL2.framework")
	set(FRAM_IMG "SDL2_image.framework")
	set(FRAM_TTF "SDL2_ttf.framework")

	downloadLib("https://www.libsdl.org/release/SDL2-${VER_SDL}.dmg" "${DIR_LIB}/${FRAM_SDL}" "")
	downloadLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-${VER_IMG}.dmg" "${DIR_LIB}/${FRAM_IMG}" "")
	downloadLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-${VER_TTF}.dmg" "${DIR_LIB}/${FRAM_TTF}" "")
	downloadLib("https://curl.haxx.se/download/curl-${VER_CURL}.tar.gz" "${DIR_CURL}" "")
	find_library(LIB_SDL "SDL2" HINTS "${DIR_LIB}")
	find_library(LIB_IMG "SDL2_image" HINTS "${DIR_LIB}")
	find_library(LIB_TTF "SDL2_ttf" HINTS "${DIR_LIB}")
	find_library(LIB_OPENGL "OpenGL")
	include_directories("${LIB_SDL}/Headers" "${DIR_CURL}/include")	# XCode can't find the SDL2 headers
else()
	if(APPIMAGE)
		set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.AppDir")
	else()
		set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
	endif()
	set(OVEN_DIR "${CMAKE_BINARY_DIR}/bin")
	set(DATA_DIR "${PBOUT_DIR}/share")
	set(FONTS_DIR "${DATA_DIR}/fonts")
	set(LICN_DIR "${DATA_DIR}/licenses")
	set(DOCS_DIR "${DATA_DIR}/doc")
	set(LIBS_DIR "${PBOUT_DIR}/lib")

	set(MAKE_CMD "make")
	find_program(NPROC "nproc")
	if(NPROC)
		execute_process(COMMAND "${NPROC}" RESULT_VARIABLE NPROC_RESULT OUTPUT_VARIABLE NPROC_OUTPUT)
		string(STRIP "${NPROC_OUTPUT}" NPROC_OUTPUT)
		if(NPROC_RESULT EQUAL 0 AND NPROC_OUTPUT GREATER 1)
			string(APPEND MAKE_CMD "\;-j${NPROC_OUTPUT}")
		endif()
	endif()

	set(DIR_GLEW "${DIR_LIB}/glew-${VER_GLEW}")
	set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}")
	if(APPIMAGE)
		set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}")
		set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}")
		set(DIR_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}")

		compileLib("https://www.libsdl.org/release/SDL2-${VER_SDL}.tar.gz" "${DIR_SDL}" "./configure;${MAKE_CMD}" "${DIR_SDL}" "")
		compileLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-${VER_IMG}.tar.gz" "${DIR_IMG}" "./configure;${MAKE_CMD}" "${DIR_IMG}" "")
		compileLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-${VER_TTF}.tar.gz" "${DIR_TTF}" "./configure;${MAKE_CMD}" "${DIR_TTF}" "")
		link_directories("${LIBS_DIR}")
		include_directories("${DIR_SDL}/include" "${DIR_IMG}" "${DIR_TTF}")
	endif()

	if(NOT OPENGLES)
		compileLib("https://sourceforge.net/projects/glew/files/glew/${VER_GLEW}/glew-${VER_GLEW}.tgz" "${DIR_GLEW}" "${MAKE_CMD}\;glew.lib.static" "${DIR_GLEW}" "")
		link_directories("${DIR_GLEW}/lib")
		include_directories("${DIR_GLEW}/include")
	endif()
	downloadLib("https://curl.haxx.se/download/curl-${VER_CURL}.tar.gz" "${DIR_CURL}" "")
	include_directories("${DIR_CURL}/include")
endif()

set(DIR_PAND "${CMAKE_SOURCE_DIR}/android")
set(DIR_PASS "${DIR_PAND}/app/src/main/assets")
if(LIBDROID)
	set(DIR_ALIB "${DIR_PAND}/app/jni")
	set(DIR_ASDL "${DIR_ALIB}/SDL")
	set(DIR_AIMG "${DIR_ALIB}/SDL_image")
	set(DIR_ATTF "${DIR_ALIB}/SDL_ttf")
	set(DIR_APRJ "${DIR_ASDL}/android-project")
	set(DIR_ALIC "${DIR_PAND}/app/licenses")

	downloadLib("https://www.libsdl.org/release/SDL2-${VER_SDL}.tar.gz" "${DIR_ASDL}" "")
	downloadLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-${VER_IMG}.tar.gz" "${DIR_AIMG}" "")
	downloadLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-${VER_TTF}.tar.gz" "${DIR_ATTF}" "")
	renameLibdir("${DIR_ALIB}/SDL2-${VER_SDL}" "${DIR_ASDL}")
	renameLibdir("${DIR_ALIB}/SDL2_image-${VER_IMG}" "${DIR_AIMG}")
	renameLibdir("${DIR_ALIB}/SDL2_ttf-${VER_TTF}" "${DIR_ATTF}")
	file(COPY "${DIR_LIB}/glm" DESTINATION "${DIR_ALIB}")
	file(COPY "${DIR_APRJ}/gradle" "${DIR_APRJ}/build.gradle" "${DIR_APRJ}/gradle.properties" "${DIR_APRJ}/gradlew" "${DIR_APRJ}/gradlew.bat" "${DIR_APRJ}/settings.gradle" DESTINATION "${DIR_PAND}")
	file(COPY "${DIR_APRJ}/app/src/main/java/org/libsdl" DESTINATION "${DIR_PAND}/app/src/main/java/org")
	copyRename("${DIR_ASDL}/COPYING.txt" "${DIR_ALIC}/COPYING.SDL.txt")
	copyRename("${DIR_AIMG}/COPYING.txt" "${DIR_ALIC}/COPYING.SDL_image.txt")
	copyRename("${DIR_ATTF}/COPYING.txt" "${DIR_ALIC}/COPYING.SDL_ttf.txt")
	copyRename("${DIR_ALIB}/glm/copying.txt" "${DIR_ALIC}/COPYING.glm.txt")
	file(COPY "${DIR_RSC}/data/LICENSE.Merriweather.txt" DESTINATION "${DIR_ALIC}")

	set(MKF_AIMG "${DIR_AIMG}/Android.mk")
	file(READ "${MKF_AIMG}" MTXT)
	string(REPLACE "SUPPORT_WEBP ?= true" "SUPPORT_WEBP ?= false" MTXT "${MTXT}")
	file(WRITE "${MKF_AIMG}" "${MTXT}")
endif()

# compiler/linker flags

if(EXTERNAL)
	add_definitions(-DEXTERNAL)
endif()
if(OPENGLES OR EMSCRIPTEN)
	add_definitions(-DOPENGLES)
endif()
if(SERVICE)
	add_definitions(-DSERVICE)
endif()
if(APPIMAGE)
	add_definitions(-DAPPIMAGE)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_definitions(-D_UNICODE -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
	if(NOT MSVC)
		add_definitions(-D_WIN32_WINNT=0x600)
	endif()
endif()

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG -D_DEBUG)
else()
	add_definitions(-DNDEBUG)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CXX_FLG_ANY "-Wall -Wextra -pedantic-errors -Wdouble-promotion -Wformat=2 -Wmissing-declarations -Wold-style-cast -Wshadow -Wunreachable-code -Wno-implicit-fallthrough -Wno-old-style-cast -Wno-switch")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(CXX_FLG_ANY "${CXX_FLG_ANY} -Wcast-align=strict -Wduplicated-cond -Wno-maybe-uninitialized")
	else()
		set(CXX_FLG_ANY "${CXX_FLG_ANY} -Wcast-align -Wnull-dereference -Wover-aligned -Wshadow-all -Wno-final-dtor-non-final-class -Wno-unused-private-field")
	endif()
	if(OPENGLES)
		set(CXX_FLG_ANY "${CXX_FLG_ANY} -Wno-unused-parameter")
	endif()

	if(EMSCRIPTEN)
		set(CXX_FLG_ANY "${CXX_FLG_ANY} --preload-file ${CMAKE_BINARY_DIR}/share@/ -s TOTAL_MEMORY=536870912 -s WASM=1 -s USE_WEBGL2=1 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='[\"jpg\",\"png\"]' -s USE_SDL_TTF=2 -s FETCH=1 -s DISABLE_EXCEPTION_CATCHING=0 -s DYNAMIC_EXECUTION=1 -lidbfs.js -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-parameter -Wno-unused-variable")
		set(FLG_DEBUG "-g4 -s ASSERTIONS=2 -s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=1 -s DEMANGLE_SUPPORT=1")
		set(FLG_RELEASE "-O3")
	else()
		if(NATIVE)
			set(CXX_FLG_ANY "${CXX_FLG_ANY} -march=native")
		elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(CXX_FLG_ANY "${CXX_FLG_ANY} -mmmx -msse -msse2")
		endif()
		set(FLG_DEBUG "-g")
		set(FLG_RELEASE "-O3")
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			set(FLG_RELEASE "${FLG_RELEASE} -flto -fuse-linker-plugin -s")
		endif()
	endif()
elseif(MSVC)
	set(CXX_FLG_ANY "/W4 /wd4100 /wd4201 /wd4244 /wd4458 /wd4706 /wd4805")
	set(FLG_DEBUG "/Od /Ob0")
	set(FLG_RELEASE "/O2 /Ob2 /Oi /Ot /GL")
endif()

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_FLG_ANY} ${FLG_DEBUG}")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_FLG_ANY} ${FLG_RELEASE}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLG_ANY} ${FLG_DEBUG}")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLG_ANY} ${FLG_RELEASE}")
endif()

# main target

if(NOT EMSCRIPTEN AND CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	find_program(STRIPPER "strip")
endif()

function(setCommonTargetProperties TARGET_NAME OUT_DIR)
	set_target_properties(${TARGET_NAME} PROPERTIES
							RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}"
							RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUT_DIR}"
							RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUT_DIR}")

	if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
		set(TEXE_NAME ${TARGET_NAME})
	else()
		string(TOLOWER ${TARGET_NAME} TEXE_NAME)
		set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${TEXE_NAME})
	endif()

	if(STRIPPER)
		if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
								COMMAND "${STRIPPER}" "${OUT_DIR}/${TEXE_NAME}")
		else()
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
								COMMAND "${STRIPPER}" -s "${OUT_DIR}/${TEXE_NAME}")
		endif()
	endif()
endfunction()

add_executable(${PROJECT_NAME} WIN32 ${THRONES_SRC})
if(EMSCRIPTEN)
	setCommonTargetProperties(${PROJECT_NAME} "${CMAKE_BINARY_DIR}")
	return()
endif()
add_dependencies(${PROJECT_NAME} ${DATA_NAME})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_libraries(${PROJECT_NAME} "${LIB_SDL}" "${LIB_IMG}" "${LIB_TTF}" "${LIB_OPENGL}")
	setCommonTargetProperties(${PROJECT_NAME} "${PBOUT_DIR}/MacOS" "")
else()
	target_link_libraries(${PROJECT_NAME} SDL2 SDL2_image SDL2_ttf)
	if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
		if(MSVC)
			target_link_libraries(${PROJECT_NAME} glew32s)
		else()
			target_link_libraries(${PROJECT_NAME} glew32)
		endif()
		target_link_libraries(${PROJECT_NAME} opengl32 ws2_32)
		setCommonTargetProperties(${PROJECT_NAME} "${PBOUT_DIR}")
	else()
		if(OPENGLES)
			target_link_libraries(${PROJECT_NAME} GLESv2 dl)
		else()
			target_link_libraries(${PROJECT_NAME} GLEW GL dl)
		endif()
		setCommonTargetProperties(${PROJECT_NAME} "${PBOUT_DIR}/bin")

		if(APPIMAGE)
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${CMAKE_COMMAND}" -E create_symlink "bin/${PROJECT_NAME_LOWER}" "${PBOUT_DIR}/AppRun")
			find_program(APPIMAGETOOL "appimagetool")
			if(APPIMAGETOOL)
				add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
									COMMAND "${APPIMAGETOOL}" "${PBOUT_DIR}")
			endif()
		endif()
	endif()
endif()

# game data copier targets

function(addDataCopierTarget TARGET_NAME TEXTRA DST_DATA DST_FONTS OPT_SHD OPT_IMG)
	add_custom_target(${TARGET_NAME} ${TEXTRA} SOURCES ${DATA_SRC}
						COMMAND "${CMAKE_COMMAND}" -E make_directory "${DST_DATA}" "${DST_FONTS}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/merriweather.otf" "${DIR_RSC}/data/romanesque.ttf" "${DST_FONTS}"
						COMMAND "${OVEN_DIR}/${OVEN_NAME}" -a "${DST_DATA}/audio.dat" ${ASSET_WAV}
						COMMAND "${OVEN_DIR}/${OVEN_NAME}" -m "${DST_DATA}/materials.dat" ${ASSET_MTL}
						COMMAND "${OVEN_DIR}/${OVEN_NAME}" -o "${DST_DATA}/objects.dat" ${ASSET_OBJ}
						COMMAND "${OVEN_DIR}/${OVEN_NAME}" ${OPT_SHD} "${DST_DATA}/shaders.dat" ${ASSET_SHD}
						COMMAND "${OVEN_DIR}/${OVEN_NAME}" ${OPT_IMG} "${DST_DATA}/textures.dat" ${ASSET_IMG}
						WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	add_dependencies(${TARGET_NAME} ${OVEN_NAME})
endfunction()

if(OPENGLES)
	addDataCopierTarget(${DATA_NAME} "ALL" "${DATA_DIR}" "${FONTS_DIR}" "-S" "-T")
else()
	addDataCopierTarget(${DATA_NAME} "ALL" "${DATA_DIR}" "${FONTS_DIR}" "-s" "-t")
endif()
add_custom_command(TARGET ${DATA_NAME} POST_BUILD
					COMMAND "${CMAKE_COMMAND}" -E make_directory "${LICN_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/doc" "${DOCS_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/cursor.png" "${DATA_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/LICENSE.Merriweather.txt" "${LICN_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_LIB}/glm/copying.txt" "${LICN_DIR}/COPYING.glm.txt")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_custom_command(TARGET ${DATA_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/thrones.png" "${DATA_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_GLEW}/LICENSE.txt" "${LICN_DIR}/LICENSE.glew.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${LIC_SDL}/COPYING.txt" "${LICN_DIR}/COPYING.SDL.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${LIC_IMG}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_image.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${LIC_TTF}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_ttf.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_IMG}/LICENSE.png.txt" "${BIN_TTF}/LICENSE.freetype.txt" "${BIN_TTF}/LICENSE.zlib.txt" "${LICN_DIR}")
	if(MSVC)
		add_custom_command(TARGET ${DATA_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_CURL}/COPYING" "${LICN_DIR}/COPYING.curl.txt")
	else()
		add_custom_command(TARGET ${DATA_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_CURL}/COPYING.txt" "${LICN_DIR}/COPYING.curl.txt"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_CURL}/COPYING-brotli.txt" "${DIR_CURL}/COPYING-libssh2.txt" "${DIR_CURL}/COPYING-nghttp2.txt" "${DIR_CURL}/COPYING-openssl.txt" "${DIR_CURL}/COPYING-zstd.txt" "${LICN_DIR}")
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_custom_command(TARGET ${DATA_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/Info.plist" "${PBOUT_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/thrones.icns" "${DIR_RSC}/data/thrones.png" "${DATA_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_CURL}/COPYING" "${LICN_DIR}/COPYING.curl.txt")
else()
	add_custom_command(TARGET ${DATA_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/thrones.desktop" "${PBOUT_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_CURL}/COPYING" "${LICN_DIR}/COPYING.curl.txt")
	if(APPIMAGE)
		add_custom_command(TARGET ${DATA_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/thrones.png" "${PBOUT_DIR}"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_SDL}/COPYING.txt" "${LICN_DIR}/COPYING.SDL.txt"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_IMG}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_image.txt"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_TTF}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_ttf.txt")
	else()
		add_custom_command(TARGET ${DATA_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/thrones.png" "${DATA_DIR}")
	endif()
	if(NOT OPENGLES)
		add_custom_command(TARGET ${DATA_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_GLEW}/LICENSE.txt" "${LICN_DIR}/LICENSE.glew.txt")
	endif()
endif()
addDataCopierTarget(${ADATA_NAME} "" "${DIR_PASS}" "${DIR_PASS}" "-S" "-T")

# server program target

add_executable(${SERVER_NAME} ${SERVER_SRC})
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_link_libraries(${SERVER_NAME} ws2_32)
	setCommonTargetProperties(${SERVER_NAME} "${PBOUT_DIR}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR APPIMAGE)
	setCommonTargetProperties(${SERVER_NAME} "${CMAKE_BINARY_DIR}")
else()
	setCommonTargetProperties(${SERVER_NAME} "${PBOUT_DIR}/bin")
endif()

# asset building program target

add_executable(${OVEN_NAME} ${OVEN_SRC})
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_libraries(${OVEN_NAME} "${LIB_SDL}" "${LIB_IMG}")
else()
	target_link_libraries(${OVEN_NAME} SDL2 SDL2_image)
endif()
setCommonTargetProperties(${OVEN_NAME} "${OVEN_DIR}")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_custom_command(TARGET ${OVEN_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBS_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_SDL}/SDL2.dll" "${BIN_IMG}/SDL2_image.dll" "${BIN_TTF}/SDL2_ttf.dll" "${BIN_IMG}/libjpeg-9.dll" "${BIN_IMG}/libpng16-16.dll" "${BIN_TTF}/libfreetype-6.dll" "${BIN_TTF}/zlib1.dll" "${PBOUT_DIR}")
	if(MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
		add_custom_command(TARGET ${OVEN_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_CURL}/libcurl.dll" "${PBOUT_DIR}")
	else()
		add_custom_command(TARGET ${OVEN_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_CURL}/libcurl-x64.dll" "${PBOUT_DIR}/libcurl.dll")
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_custom_command(TARGET ${OVEN_NAME} PRE_BUILD
						COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBS_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_SDL}" "${LIBS_DIR}/${FRAM_SDL}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_IMG}" "${LIBS_DIR}/${FRAM_IMG}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_TTF}" "${LIBS_DIR}/${FRAM_TTF}")
elseif(APPIMAGE)
	add_custom_command(TARGET ${OVEN_NAME} PRE_BUILD
						COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBS_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_SDL}/build/.libs/libSDL2*.so*" "${DIR_IMG}/.libs/libSDL2_image*.so*" "${DIR_TTF}/.libs/libSDL2_ttf*.so*" "${LIBS_DIR}")
endif()

# tests

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_library(${TLIB_NAME} STATIC EXCLUDE_FROM_ALL ${THRONES_SRC})
	target_compile_definitions(${TLIB_NAME} PUBLIC IS_TEST_LIBRARY)
	target_link_libraries(${TLIB_NAME} SDL2 SDL2_image SDL2_ttf GLEW GL curl)

	enable_testing()
	add_executable(${TESTS_NAME} EXCLUDE_FROM_ALL ${TESTS_SRC})
	target_link_libraries(${TESTS_NAME} ${TLIB_NAME})
	add_dependencies(${TESTS_NAME} ${TLIB_NAME})
	add_test(NAME ${TESTS_NAME} COMMAND ${TESTS_NAME})
endif()

# prettyfiers

set(ALL_SRC ${THRONES_SRC} ${DATA_SRC} ${SERVER_SRC} ${OVEN_SRC} ${TESTS_SRC})
foreach(FSRC IN LISTS ALL_SRC)
	get_filename_component(FGRP "${FSRC}" DIRECTORY)
	string(REPLACE "/" ";" FGRP "${FGRP}")
	list(REMOVE_AT FGRP 0)
	string(REPLACE ";" "/" FGRP "${FGRP}")
	source_group("${FGRP}" FILES "${FSRC}")
endforeach()
