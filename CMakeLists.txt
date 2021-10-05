cmake_minimum_required(VERSION 3.10.2)
project(Thrones LANGUAGES CXX)
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(ProcessorCount)
ProcessorCount(NJOBS)

# target names and source/resource files

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
set(SERVER_NAME "Server")
set(DATA_NAME "assets")
set(ADATA_NAME "${DATA_NAME}_android")
set(EDATA_NAME "${DATA_NAME}_emscripten")
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
	"rsc/objects/breached4.obj"
	"rsc/objects/catapult.obj"
	"rsc/objects/crossbowmen.obj"
	"rsc/objects/dragon.obj"
	"rsc/objects/elephant.obj"
	"rsc/objects/fortress4.obj"
	"rsc/objects/ground0.obj"
	"rsc/objects/lancer.obj"
	"rsc/objects/outline6.obj"
	"rsc/objects/plane5.obj"
	"rsc/objects/rangers.obj"
	"rsc/objects/screen3.obj"
	"rsc/objects/spearmen.obj"
	"rsc/objects/table1.obj"
	"rsc/objects/throne.obj"
	"rsc/objects/tile4.obj"
	"rsc/objects/trebuchet.obj"
	"rsc/objects/warhorse.obj")

set(ASSET_SHD
	"rsc/shaders/blur.frag"
	"rsc/shaders/depth.frag"
	"rsc/shaders/depth.vert"
	"rsc/shaders/final.frag"
	"rsc/shaders/frame.vert"
	"rsc/shaders/gauss.frag"
	"rsc/shaders/geom.frag"
	"rsc/shaders/geom.vert"
	"rsc/shaders/gui.frag"
	"rsc/shaders/gui.vert"
	"rsc/shaders/light.frag"
	"rsc/shaders/light.vert"
	"rsc/shaders/skybox.frag"
	"rsc/shaders/skybox.vert"
	"rsc/shaders/ssao.frag")

set(ASSET_IMG
	"rsc/textures/assault1.png"
	"rsc/textures/catapult1.png"
	"rsc/textures/city2.png"
	"rsc/textures/conspire1.png"
	"rsc/textures/crossbowmen1.png"
	"rsc/textures/deceive1.png"
	"rsc/textures/dragon1.png"
	"rsc/textures/elephant1.png"
	"rsc/textures/farm2.png"
	"rsc/textures/forest3.png"
	"rsc/textures/fortress2.png"
	"rsc/textures/grass2.png"
	"rsc/textures/hasten1.png"
	"rsc/textures/lancer1.png"
	"rsc/textures/metal2.png"
	"rsc/textures/mountain3.png"
	"rsc/textures/plains3.png"
	"rsc/textures/rangers1.png"
	"rsc/textures/reticle2.png"
	"rsc/textures/rock2.png"
	"rsc/textures/sky40.png"
	"rsc/textures/spearmen1.png"
	"rsc/textures/throne1.png"
	"rsc/textures/title1.png"
	"rsc/textures/trebuchet1.png"
	"rsc/textures/wall2.png"
	"rsc/textures/warhorse1.png"
	"rsc/textures/water3.png")

foreach(IMG IN LISTS ASSET_IMG)
	list(APPEND DATA_SRC "${IMG}")
	if(IMG MATCHES "[a-zA-Z/]+[23]\.[a-zA-Z]+")
		string(REGEX REPLACE "[23]" "N" NIMG "${IMG}")
		list(APPEND DATA_SRC "${NIMG}")
	elseif(IMG MATCHES "[a-zA-Z/]+40\.[a-zA-Z]+")
		foreach(INUM RANGE 1 5)
			string(REPLACE "40" "4${INUM}" NIMG "${IMG}")
			list(APPEND DATA_SRC "${NIMG}")
		endforeach()
	endif()
endforeach()
list(APPEND DATA_SRC ${ASSET_WAV} ${ASSET_DAT} ${ASSET_MTL} ${ASSET_OBJ} ${ASSET_SHD})

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
	"src/test/server.cpp"
	"src/test/tests.cpp"
	"src/test/tests.h"
	"src/test/text.cpp"
	"src/test/utils.cpp")

# dependencies

option(EXTERNAL "Save settings externally." ON)
option(OPENGLES "Build for OpenGL ES. (always on with Emscripten)" OFF)
option(SERVICE "Build server without keyboard input." OFF)
option(LIBDROID "Download libraries for Android." OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	option(NATIVE "Build for the current CPU." OFF)
endif()
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	option(APPIMAGE "Package as an AppImage." OFF)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	option(SYSLIBS "Use system libraries." OFF)
elseif(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
	option(SYSLIBS "Use system libraries." ON)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	set(ARCH CACHE STRING "CPU type to pass to Clang's or GCC's \"-march\" option")
endif()
set(EMBASE "${CMAKE_BINARY_DIR}" CACHE STRING "Location where to put the assets built by ${EDATA_NAME}")
set(VER_SDL "2.0.18" CACHE STRING "SDL2 version.")
set(VER_IMG "2.0.5" CACHE STRING "SDL2_image version.")
set(VER_TTF "2.0.15" CACHE STRING "SDL2_ttf version.")
set(VER_GLEW "2.2.0" CACHE STRING "GLEW version.")
set(VER_GLM "0.9.9.8" CACHE STRING "glm version.")
set(VER_ASSIMP "5.1.4" CACHE STRING "assimp version.")
set(VER_CURL "7.80.0" CACHE STRING "libcurl version.")
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
	set(DATA_DIR "${PBOUT_DIR}/share/${PROJECT_NAME_LOWER}")
	set(DOCS_DIR "${PBOUT_DIR}/doc")
	set(LICN_DIR "${PBOUT_DIR}/licenses")

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
		set(DIR_ASSIMP "${DIR_LIB}/assimp-${VER_ASSIMP}-msvc-${VMACHINE}")
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
		downloadLib("https://netix.dl.sourceforge.net/project/glew/glew/${VER_GLEW}/glew-${VER_GLEW}-win32.zip" "${DIR_GLEW}" "${DIR_LIB}/glew-${VER_GLEW}")
		compileLib("https://github.com/assimp/assimp/archive/refs/tags/v${VER_ASSIMP}.zip" "${DIR_ASSIMP}" "cmake\;.\;-G\;NMake Makefiles\;-DASSIMP_NO_EXPORT=1\;-DASSIMP_BUILD_ASSIMP_TOOLS=0\;-DASSIMP_BUILD_TESTS=0;nmake\;/f\;Makefile\;assimp" "${DIR_ASSIMP}" "${DIR_LIB}/assimp-${VER_ASSIMP}")
		compileLib("https://curl.se/download/curl-${VER_CURL}.zip" "${DIR_CURL}" "nmake\;/f\;Makefile.vc\;mode=dll\;MACHINE=${VMACHINE}" "${DIR_CURL}/winbuild" "${DIR_LIB}/curl-${VER_CURL}")
		link_directories("${BIN_SDL}" "${BIN_IMG}" "${BIN_TTF}" "${LIB_GLEW}")
		include_directories("${DIR_SDL}/include" "${DIR_IMG}/include" "${DIR_TTF}/include" "${DIR_GLEW}/include" "${DIR_ASSIMP}/include" "${OUT_CURL}/include")
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
		set(DIR_ASSIMP "${DIR_LIB}/assimp-${VER_ASSIMP}-${VMACHINE}-mingw")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}-${VMACHINE}-mingw")
		set(BIN_SDL "${DIR_SDL}/bin")
		set(BIN_IMG "${DIR_IMG}/bin")
		set(BIN_TTF "${DIR_TTF}/bin")
		set(BIN_CURL "${DIR_CURL}/bin")

		if(N LESS_EQUAL 1)
			set(MAKE_CMD "mingw32-make")
		else()
			set(MAKE_CMD "mingw32-make\;-j${NJOBS}")
		endif()

		downloadLib("https://www.libsdl.org/release/SDL2-devel-${VER_SDL}-mingw.tar.gz" "${LIC_SDL}" "${DIR_LIB}/SDL2-${VER_SDL}")
		downloadLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-${VER_IMG}-mingw.tar.gz" "${LIC_IMG}" "${DIR_LIB}/SDL2_image-${VER_IMG}")
		downloadLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-${VER_TTF}-mingw.tar.gz" "${LIC_TTF}" "${DIR_LIB}/SDL2_ttf-${VER_TTF}")
		compileLib("https://jztkft.dl.sourceforge.net/project/glew/glew/${VER_GLEW}/glew-${VER_GLEW}.zip" "${DIR_GLEW}" "${MAKE_CMD}\;glew.lib.static" "${DIR_GLEW}" "${DIR_LIB}/glew-${VER_GLEW}")
		compileLib("https://github.com/assimp/assimp/archive/refs/tags/v${VER_ASSIMP}.tar.gz" "${DIR_ASSIMP}" "cmake\;.\;-G\;MinGW Makefiles\;-DASSIMP_NO_EXPORT=1\;-DASSIMP_BUILD_ASSIMP_TOOLS=0\;-DASSIMP_BUILD_TESTS=0;${MAKE_CMD}\;assimp" "${DIR_ASSIMP}" "${DIR_LIB}/assimp-${VER_ASSIMP}")
		downloadLib("https://curl.se/windows/dl-${VER_CURL}/curl-${VER_CURL}-${VMACHINE}-mingw.zip" "${DIR_CURL}" "")
		link_directories("${DIR_SDL}/lib" "${DIR_IMG}/lib" "${DIR_TTF}/lib" "${DIR_GLEW}/lib")
		include_directories("${DIR_SDL}/include/SDL2" "${DIR_IMG}/include/SDL2" "${DIR_TTF}/include/SDL2" "${DIR_GLEW}/include" "${DIR_ASSIMP}/include" "${DIR_CURL}/include")
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents")
	set(DATA_DIR "${PBOUT_DIR}/Resources")
	set(DOCS_DIR "${DATA_DIR}/doc")
	set(LICN_DIR "${DATA_DIR}/licenses")
	set(LIBS_DIR "${PBOUT_DIR}/Frameworks")
	set(FRAM_SDL "SDL2.framework")
	set(FRAM_IMG "SDL2_image.framework")
	set(FRAM_TTF "SDL2_ttf.framework")

	if(N LESS_EQUAL 1)
		set(MAKE_CMD "make")
	else()
		set(MAKE_CMD "make\;-j${NJOBS}")
	endif()

	downloadLib("https://www.libsdl.org/release/SDL2-${VER_SDL}.dmg" "${DIR_LIB}/${FRAM_SDL}" "")
	downloadLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-${VER_IMG}.dmg" "${DIR_LIB}/${FRAM_IMG}" "")
	downloadLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-${VER_TTF}.dmg" "${DIR_LIB}/${FRAM_TTF}" "")
	find_library(LIB_SDL "SDL2" HINTS "${DIR_LIB}")
	find_library(LIB_IMG "SDL2_image" HINTS "${DIR_LIB}")
	find_library(LIB_TTF "SDL2_ttf" HINTS "${DIR_LIB}")
	find_library(LIB_OPENGL "OpenGL")
	include_directories("${LIB_SDL}/Headers")	# XCode can't find the SDL2 headers

	if(NOT SYSLIBS)
		set(DIR_ASSIMP "${DIR_LIB}/assimp-${VER_ASSIMP}")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}")
		compileLib("https://github.com/assimp/assimp/archive/refs/tags/v${VER_ASSIMP}.tar.gz" "${DIR_ASSIMP}" "cmake\;.\;-DBUILD_SHARED_LIBS=0\;-DASSIMP_NO_EXPORT=1\;-DASSIMP_BUILD_ASSIMP_TOOLS=0\;-DASSIMP_BUILD_TESTS=0;${MAKE_CMD}\;assimp" "${DIR_ASSIMP}" "")
		downloadLib("https://curl.se/download/curl-${VER_CURL}.tar.gz" "${DIR_CURL}" "")
		link_directories("${DIR_ASSIMP}/bin")
		include_directories("${DIR_ASSIMP}/include" "${DIR_CURL}/include")
	endif()
else()
	if(APPIMAGE)
		set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.AppDir")
		set(TBIN_DIR "${PBOUT_DIR}/usr/bin")
		set(DATA_DIR "${PBOUT_DIR}/usr/share/${PROJECT_NAME_LOWER}")
	else()
		set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
		set(TBIN_DIR "${PBOUT_DIR}/bin")
		set(DATA_DIR "${PBOUT_DIR}/share/${PROJECT_NAME_LOWER}")
	endif()
	set(DOCS_DIR "${DATA_DIR}/doc")
	set(LICN_DIR "${DATA_DIR}/licenses")

	if(N LESS_EQUAL 1)
		set(MAKE_CMD "make")
	else()
		set(MAKE_CMD "make\;-j${NJOBS}")
	endif()

	set(DIR_GLEW "${DIR_LIB}/glew-${VER_GLEW}")
	if(NOT OPENGLES)
		compileLib("https://sourceforge.net/projects/glew/files/glew/${VER_GLEW}/glew-${VER_GLEW}.tgz" "${DIR_GLEW}" "${MAKE_CMD}\;glew.lib.static" "${DIR_GLEW}" "")
		link_directories("${DIR_GLEW}/lib")
		include_directories("${DIR_GLEW}/include")
	endif()

	if(NOT SYSLIBS)
		set(DIR_ASSIMP "${DIR_LIB}/assimp-${VER_ASSIMP}")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}")
		compileLib("https://github.com/assimp/assimp/archive/refs/tags/v${VER_ASSIMP}.tar.gz" "${DIR_ASSIMP}" "cmake\;.\;-DBUILD_SHARED_LIBS=0\;-DASSIMP_NO_EXPORT=1\;-DASSIMP_BUILD_ASSIMP_TOOLS=0\;-DASSIMP_BUILD_TESTS=0;${MAKE_CMD}\;assimp" "${DIR_ASSIMP}" "")
		downloadLib("https://curl.se/download/curl-${VER_CURL}.tar.gz" "${DIR_CURL}" "")
		link_directories("${DIR_ASSIMP}/bin")
		include_directories("${DIR_ASSIMP}/include" "${DIR_CURL}/include")
	endif()
endif()

set(DIR_PAND "${CMAKE_SOURCE_DIR}/android")
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
	copyRename("${DIR_ASDL}/LICENSE.txt" "${DIR_ALIC}/LICENSE.SDL.txt")
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
	if(GLES32)
		add_definitions(-DOPENGLES=32)
	else()
		add_definitions(-DOPENGLES=1)
	endif()
endif()
if(SERVICE)
	add_definitions(-DSERVICE)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_definitions(-D_UNICODE -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
	if(NOT MSVC)
		add_definitions(-D_WIN32_WINNT=0x600)
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DMALLOC_CHECK_=2")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")	# doesn't exist on some platforms (macOS)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DMALLOC_CHECK_=2)
else()
	add_definitions(-DNDEBUG)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	set(CXX_FLG_ANY "-Wall -Wextra -pedantic-errors -Wdouble-promotion -Wfloat-conversion -Wformat=2 -Wmissing-declarations -Wold-style-cast -Wshadow -Wunreachable-code -Wno-implicit-fallthrough -Wno-old-style-cast -Wno-switch")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(CXX_FLG_ANY "${CXX_FLG_ANY} -Wcast-align=strict -Wduplicated-cond -Wno-maybe-uninitialized")
	else()
		set(CXX_FLG_ANY "${CXX_FLG_ANY} -Wcast-align -Wimplicit-int-float-conversion -Wnull-dereference -Wover-aligned -Wshadow-all -Wno-final-dtor-non-final-class -Wno-unused-private-field")
	endif()
	if(OPENGLES)
		set(CXX_FLG_ANY "${CXX_FLG_ANY} -Wno-unused-parameter")
	endif()

	if(EMSCRIPTEN)
		set(CXX_FLG_ANY "${CXX_FLG_ANY} -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='[\"jpg\",\"png\"]' -s USE_SDL_TTF=2 -s DISABLE_EXCEPTION_CATCHING=0 -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-parameter -Wno-unused-variable")
		set(FLG_DEBUG "-g")
		set(FLG_RELEASE "-O3 -flto")
	else()
		if(ARCH)
			set(FLG_ANY "${FLG_ANY} -march=${ARCH}")
		elseif(CMAKE_SYSTEM_PROCESSOR MATCHES ".86.64")
			set(FLG_ANY "${FLG_ANY} -march=x86-64")
		endif()

		set(FLG_DEBUG "-g")
		set(FLG_RELEASE "-O3")
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			set(FLG_RELEASE "${FLG_RELEASE} -flto -fuse-linker-plugin -s")
		endif()
	endif()
elseif(MSVC)
	set(CXX_FLG_ANY "/W4 /wd4201 /wd4244 /wd4267 /wd4706")
	set(FLG_RELEASE "/O2 /GL")
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

if(NOT EMSCRIPTEN AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang") AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	find_program(STRIPPER "strip")
	if(NOT STRIPPER)
		message(WARNING "Can't strip executables: Failed to find stip")
	endif()
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
	target_link_libraries(${PROJECT_NAME} "--preload-file ${CMAKE_BINARY_DIR}/share@/" "-s TOTAL_MEMORY=536870912" "-s WASM=1" "-s USE_WEBGL2=1" "-s FETCH=1" "-s DYNAMIC_EXECUTION=1" idbfs.js)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_link_libraries(${PROJECT_NAME} "-gsource-map" "--source-map-base http://localhost:8080/" "-s ASSERTIONS=2" "-s STACK_OVERFLOW_CHECK=2" "-s SAFE_HEAP=1" "-s DEMANGLE_SUPPORT=1")
	endif()
	setCommonTargetProperties(${PROJECT_NAME} "${CMAKE_BINARY_DIR}")
	return()
endif()
add_dependencies(${PROJECT_NAME} ${DATA_NAME})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_libraries(${PROJECT_NAME} "${LIB_SDL}" "${LIB_IMG}" "${LIB_TTF}" "${LIB_OPENGL}")
	setCommonTargetProperties(${PROJECT_NAME} "${PBOUT_DIR}/MacOS" "")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBS_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_SDL}" "${LIBS_DIR}/${FRAM_SDL}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_IMG}" "${LIBS_DIR}/${FRAM_IMG}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_TTF}" "${LIBS_DIR}/${FRAM_TTF}")
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

		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_SDL}/SDL2.dll" "${BIN_IMG}/SDL2_image.dll" "${BIN_TTF}/SDL2_ttf.dll" "${BIN_IMG}/libpng16-16.dll" "${BIN_TTF}/libfreetype-6.dll" "${BIN_TTF}/zlib1.dll" "${PBOUT_DIR}")
		if(MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_CURL}/libcurl.dll" "${PBOUT_DIR}")
		else()
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_CURL}/libcurl-x64.dll" "${PBOUT_DIR}/libcurl.dll")
		endif()
	else()
		if(OPENGLES)
			target_link_libraries(${PROJECT_NAME} GLESv2 dl)
		else()
			target_link_libraries(${PROJECT_NAME} GLEW GL dl)
		endif()
		setCommonTargetProperties(${PROJECT_NAME} "${TBIN_DIR}")

		if(APPIMAGE)
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${CMAKE_COMMAND}" -E create_symlink "usr/bin/${PROJECT_NAME_LOWER}" "${PBOUT_DIR}/AppRun"
								COMMAND "${CMAKE_COMMAND}" -E create_symlink "usr/share/${PROJECT_NAME_LOWER}/${PROJECT_NAME_LOWER}.png" "${PBOUT_DIR}/${PROJECT_NAME_LOWER}.png")

			find_program(LINUXDEPLOY "linuxdeploy")
			if(NOT LINUXDEPLOY)
				set(NAME "linuxdeploy-x86_64.AppImage")
				set(URL "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/${NAME}")
				set(LINUXDEPLOY "${DIR_LIB}/${NAME}")

				if(NOT EXISTS "${LINUXDEPLOY}")
					file(DOWNLOAD "${URL}" "${LINUXDEPLOY}")
					execute_process(COMMAND chmod 775 "${LINUXDEPLOY}")
				endif()
			endif()
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
								COMMAND "${LINUXDEPLOY}" --appdir "${PBOUT_DIR}" --output appimage)
		endif()
	endif()
endif()

# game data copier targets

function(addDataCopierTarget TARGET_NAME TEXTRA DST_DATA OPT_SHD OPT_IMG)
	add_custom_target(${TARGET_NAME} ${TEXTRA} SOURCES ${DATA_SRC}
						COMMAND "${CMAKE_COMMAND}" -E make_directory "${DST_DATA}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/merriweather.otf" "${DIR_RSC}/data/romanesque.ttf" "${DST_DATA}"
						COMMAND "${CMAKE_BINARY_DIR}/${OVEN_NAME}" -a "${DST_DATA}/audio.dat" ${ASSET_WAV}
						COMMAND "${CMAKE_BINARY_DIR}/${OVEN_NAME}" -m "${DST_DATA}/materials.dat" ${ASSET_MTL}
						COMMAND "${CMAKE_BINARY_DIR}/${OVEN_NAME}" -o "${DST_DATA}/objects.dat" ${ASSET_OBJ}
						COMMAND "${CMAKE_BINARY_DIR}/${OVEN_NAME}" ${OPT_SHD} "${DST_DATA}/shaders.dat" ${ASSET_SHD}
						COMMAND "${CMAKE_BINARY_DIR}/${OVEN_NAME}" ${OPT_IMG} "${DST_DATA}/textures.dat" ${ASSET_IMG}
						WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	add_dependencies(${TARGET_NAME} ${OVEN_NAME})
endfunction()

if(OPENGLES)
	addDataCopierTarget(${DATA_NAME} "ALL" "${DATA_DIR}" "-S" "-T")
else()
	addDataCopierTarget(${DATA_NAME} "ALL" "${DATA_DIR}" "-s" "-t")
endif()
add_custom_command(TARGET ${DATA_NAME} POST_BUILD
					COMMAND "${CMAKE_COMMAND}" -E make_directory "${LICN_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/doc" "${DOCS_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/${PROJECT_NAME_LOWER}.png" "${DIR_RSC}/data/cursor.png" "${DATA_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/LICENSE.Merriweather.txt" "${LICN_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_LIB}/glm/copying.txt" "${LICN_DIR}/COPYING.glm.txt")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_custom_command(TARGET ${DATA_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_GLEW}/LICENSE.txt" "${LICN_DIR}/LICENSE.glew.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${LIC_SDL}/COPYING.txt" "${LICN_DIR}/LICENSE.SDL.txt"
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
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/${PROJECT_NAME_LOWER}.icns" "${DATA_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_CURL}/COPYING" "${LICN_DIR}/COPYING.curl.txt")
else()
	add_custom_command(TARGET ${DATA_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/${PROJECT_NAME_LOWER}.desktop" "${PBOUT_DIR}")

	if(NOT OPENGLES)
		add_custom_command(TARGET ${DATA_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_GLEW}/LICENSE.txt" "${LICN_DIR}/LICENSE.glew.txt")
	endif()
endif()
addDataCopierTarget(${ADATA_NAME} "" "${DIR_PAND}/app/src/main/assets" "-S" "-T")

get_filename_component(EMS_DIR "${EMBASE}" ABSOLUTE)
addDataCopierTarget(${EDATA_NAME} "" "${EMS_DIR}/share" "-S" "-T")
add_custom_command(TARGET ${EDATA_NAME} POST_BUILD
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/doc" "${EMS_DIR}/doc"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/${PROJECT_NAME_LOWER}.png" "${EMS_DIR}")

# server program target

add_executable(${SERVER_NAME} ${SERVER_SRC})
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_link_libraries(${SERVER_NAME} ws2_32)
	setCommonTargetProperties(${SERVER_NAME} "${PBOUT_DIR}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR APPIMAGE)
	setCommonTargetProperties(${SERVER_NAME} "${CMAKE_BINARY_DIR}")
else()
	setCommonTargetProperties(${SERVER_NAME} "${TBIN_DIR}")
endif()

# asset building program target

add_executable(${OVEN_NAME} ${OVEN_SRC})
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if(MSVC)
		file(GLOB ASSIMP_LIB "${DIR_ASSIMP}/lib/assimp*.lib")
	else()
		file(GLOB ASSIMP_LIB "${DIR_ASSIMP}/lib/libassimp*.a")
	endif()
	target_link_libraries(${OVEN_NAME} SDL2 SDL2_image "${ASSIMP_LIB}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_libraries(${OVEN_NAME} "${LIB_SDL}" "${LIB_IMG}" assimp)
else()
	target_link_libraries(${OVEN_NAME} SDL2 SDL2_image assimp)
endif()
setCommonTargetProperties(${OVEN_NAME} "${CMAKE_BINARY_DIR}")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if(MSVC)
		file(GLOB ASSIMP_DLL "${DIR_ASSIMP}/bin/assimp*.dll")
	else()
		file(GLOB ASSIMP_DLL "${DIR_ASSIMP}/bin/libassimp*.dll")
	endif()
	add_custom_command(TARGET ${OVEN_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_SDL}/SDL2.dll" "${BIN_IMG}/SDL2_image.dll" "${BIN_IMG}/libpng16-16.dll" "${BIN_TTF}/zlib1.dll" "${ASSIMP_DLL}" "${CMAKE_BINARY_DIR}")
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
