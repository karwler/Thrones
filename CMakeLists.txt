cmake_minimum_required(VERSION 3.12.4)
project(Thrones VERSION 0.5.3 DESCRIPTION "Thrones Board Game" HOMEPAGE_URL "http://thrones4.spdns.org/" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT GENERATOR_IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

option(EXTERNAL "Save settings externally." ON)
option(SERVICE "Build server without keyboard input." OFF)
option(LIBDROID "Download libraries for Android." OFF)
if(NOT EMSCRIPTEN)
	option(OPENVR "Build with VR support." ON)
endif()
if(UNIX AND NOT APPLE)
	option(APPIMAGE "Package as an AppImage." OFF)
endif()
if(APPLE)
	option(SYSLIBS "Use system libraries." OFF)
elseif(NOT WIN32)
	option(SYSLIBS "Use system libraries." ON)
endif()
if(NOT EMSCRIPTEN AND NOT WIN32)
	set(OPENGLES CACHE STRING "Build for OpenGL ES. (always on with Emscripten)")
endif()
if(NOT EMSCRIPTEN AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
	set(ARCH CACHE STRING "CPU type to pass to Clang's or GCC's \"-march\" option")
endif()
set(EMBASE "${CMAKE_BINARY_DIR}" CACHE STRING "Location where to put the assets built by ${EDATA_NAME}")
set(TEXDIV CACHE STRING "Override the default texture size divisors.")
if(EMSCRIPTEN)
	option(UPDATE_CHECK "Build with update check on startup. (causes CORS error)" OFF)
else()
	option(UPDATE_CHECK "Build with update check on startup." ON)
endif()

# target names and source/resource files

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
set(GAMEVR_NAME "ThronesVr")
set(SERVER_NAME "Server")
set(DATA_NAME "assets")
set(ADATA_NAME "${DATA_NAME}_android")
set(EDATA_NAME "${DATA_NAME}_emscripten")
set(OVEN_NAME "oven")
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
	"rsc/objects/grid2.obj"
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
	"rsc/shaders/blurColor.frag"
	"rsc/shaders/blurMono.frag"
	"rsc/shaders/brights.frag"
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
	"rsc/shaders/startup.frag"
	"rsc/shaders/startup.vert"
	"rsc/shaders/ssao.frag"
	"rsc/shaders/ssr.frag"
	"rsc/shaders/ssrColor.frag")
if(OPENVR)
	set(ASSET_SHD
		${ASSET_SHD}
		"rsc/shaders/vrController.frag"
		"rsc/shaders/vrController.vert"
		"rsc/shaders/vrGui.vert"
		"rsc/shaders/vrModel.frag"
		"rsc/shaders/vrModel.vert"
		"rsc/shaders/vrWindow.frag"
		"rsc/shaders/vrWindow.vert")
endif()

set(ASSET_IMG
	"rsc/textures/assault1.png"
	"rsc/textures/city2.png"
	"rsc/textures/cityN.png"
	"rsc/textures/conspire1.png"
	"rsc/textures/deceive1.png"
	"rsc/textures/farm2.png"
	"rsc/textures/farmN.png"
	"rsc/textures/forest3.png"
	"rsc/textures/forestN.png"
	"rsc/textures/fortress2.png"
	"rsc/textures/fortressN.png"
	"rsc/textures/grass2.png"
	"rsc/textures/grassN.png"
	"rsc/textures/grid2.png"
	"rsc/textures/hasten1.png"
	"rsc/textures/metal2.png"
	"rsc/textures/metalN.png"
	"rsc/textures/mountain3.png"
	"rsc/textures/mountainN.png"
	"rsc/textures/plains3.png"
	"rsc/textures/plainsN.png"
	"rsc/textures/reticle2.png"
	"rsc/textures/rock2.png"
	"rsc/textures/rockN.png"
	"rsc/textures/sky40.png"
	"rsc/textures/sky41.png"
	"rsc/textures/sky42.png"
	"rsc/textures/sky43.png"
	"rsc/textures/sky44.png"
	"rsc/textures/sky45.png"
	"rsc/textures/title1.png"
	"rsc/textures/wall2.png"
	"rsc/textures/wallN.png"
	"rsc/textures/water3.png"
	"rsc/textures/waterN.png")

set(ASSET_IMG_EXT
	${ASSET_IMG}
	"rsc/textures/cog1.svg"
	"rsc/textures/wrench1.svg")

set(DATA_SRC ${ASSET_WAV} ${ASSET_DAT} ${ASSET_MTL} ${ASSET_OBJ} ${ASSET_SHD} ${ASSET_IMG_EXT})

set(THRONES_SRC
	"src/engine/audioSys.cpp"
	"src/engine/audioSys.h"
	"src/engine/fileSys.cpp"
	"src/engine/fileSys.h"
	"src/engine/inputSys.cpp"
	"src/engine/inputSys.h"
	"src/engine/scene.cpp"
	"src/engine/scene.h"
	"src/engine/shaders.cpp"
	"src/engine/shaders.h"
	"src/engine/vrSys.cpp"
	"src/engine/vrSys.h"
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
	"src/prog/recorder.cpp"
	"src/prog/recorder.h"
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

if(WIN32)
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
if(WIN32)
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
	"src/engine/audioSys.cpp"
	"src/engine/audioSys.h"
	"src/engine/fileSys.cpp"
	"src/engine/fileSys.h"
	"src/engine/inputSys.cpp"
	"src/engine/inputSys.h"
	"src/engine/scene.cpp"
	"src/engine/scene.h"
	"src/engine/shaders.cpp"
	"src/engine/shaders.h"
	"src/engine/vrSys.cpp"
	"src/engine/vrSys.h"
	"src/engine/windowSys.cpp"
	"src/engine/windowSys.h"
	"src/engine/fileSys.cpp"
	"src/engine/fileSys.h"
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
	"src/prog/recorder.cpp"
	"src/prog/recorder.h"
	"src/prog/types.cpp"
	"src/prog/types.h"
	"src/server/server.cpp"
	"src/server/server.h"
	"src/test/aliasTest.cpp"
	"src/test/fileSysTest.cpp"
	"src/test/serverTest.cpp"
	"src/test/tests.cpp"
	"src/test/tests.h"
	"src/test/textTest.cpp"
	"src/test/utilsTest.cpp"
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

# dependencies

set(VER_SDL "2.26.2")
set(VER_IMG "2.6.2")
set(VER_TTF "2.20.1")
set(VER_GLEW "2.2.0")
set(VER_GLM "0.9.9.8")
set(VER_ASSIMP "5.2.4")
set(VER_CURL "7.83.1")
set(VER_OVR "1.16.8")
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

function(downloadLibOpenVr LIBDIR ARCH_DIR_32 ARCH_DIR_64 ORIG_LIBDIR)
	if(OPENVR)
		downloadLib("https://github.com/ValveSoftware/openvr/archive/refs/tags/v${VER_OVR}.zip" "${LIBDIR}" "${ORIG_LIBDIR}")
		link_directories("${LIBDIR}/lib/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,${ARCH_DIR_64},${ARCH_DIR_32}>")
		include_directories("${LIBDIR}/headers")
	endif()
endfunction()

if(MSVC)
	set(ENV{CL} /MP)
	set(MAKE_CMD "nmake")
else()
	set(MAKE_CMD "make")
	if(MINGW)
		set(MAKE_CMD "mingw32-${MAKE_CMD}")
	endif()

	include(ProcessorCount)
	ProcessorCount(NJOBS)
	if(NJOBS GREATER 1)
		set(MAKE_CMD "${MAKE_CMD}\;-j${NJOBS}")
	endif()
endif()

file(MAKE_DIRECTORY "${DIR_LIB}")
downloadLib("https://github.com/g-truc/glm/releases/download/${VER_GLM}/glm-${VER_GLM}.zip" "${DIR_LIB}/glm" "")
include_directories("${CMAKE_SOURCE_DIR}/src" "${DIR_LIB}/glm")

set(ASSIMP_ARGS "-DCMAKE_BUILD_TYPE=Release\;-DASSIMP_NO_EXPORT=1\;-DASSIMP_BUILD_ASSIMP_TOOLS=0\;-DASSIMP_BUILD_TESTS=0\;-DASSIMP_INSTALL=0\;-DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=0\;-DASSIMP_BUILD_OBJ_IMPORTER=1\;-DASSIMP_WARNINGS_AS_ERRORS=0")
if(EMSCRIPTEN)
	file(COPY "${DIR_RSC}/thrones.html" DESTINATION "${CMAKE_BINARY_DIR}")
elseif(WIN32)
	set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
	set(DATA_DIR "${PBOUT_DIR}/share/${PROJECT_NAME_LOWER}")
	set(DOCS_DIR "${PBOUT_DIR}/doc")
	set(LICN_DIR "${PBOUT_DIR}/licenses")

	if(MSVC)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(VMACHINE "x64")
			set(VMARCH "x64")
		else()
			set(VMACHINE "x86")
			set(VMARCH "Win32")
		endif()
		set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}-msvc")
		set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}-msvc")
		set(DIR_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}-msvc")
		set(DIR_GLEW "${DIR_LIB}/glew-${VER_GLEW}-msvc")
		set(DIR_ASSIMP "${DIR_LIB}/assimp-${VER_ASSIMP}-${VMACHINE}-msvc")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}-${VMACHINE}-msvc")
		set(OUT_CURL "${DIR_CURL}/builds/libcurl-vc-${VMACHINE}-release-dll-ipv6-sspi-schannel")
		set(DIR_OVR "${DIR_LIB}/openvr-${VER_OVR}-general")
		set(BIN_SDL "${DIR_SDL}/lib/${VMACHINE}")
		set(BIN_IMG "${DIR_IMG}/lib/${VMACHINE}")
		set(BIN_TTF "${DIR_TTF}/lib/${VMACHINE}")
		set(BIN_CURL "${OUT_CURL}/bin")
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(BIN_OVR "${DIR_OVR}/bin/win64")
		else()
			set(BIN_OVR "${DIR_OVR}/bin/win32")
		endif()

		downloadLib("https://github.com/libsdl-org/SDL/releases/download/release-${VER_SDL}/SDL2-devel-${VER_SDL}-VC.zip" "${DIR_SDL}" "${DIR_LIB}/SDL2-${VER_SDL}")
		downloadLib("https://github.com/libsdl-org/SDL_image/releases/download/release-${VER_IMG}/SDL2_image-devel-${VER_IMG}-VC.zip" "${DIR_IMG}" "${DIR_LIB}/SDL2_image-${VER_IMG}")
		downloadLib("https://github.com/libsdl-org/SDL_ttf/releases/download/release-${VER_TTF}/SDL2_ttf-devel-${VER_TTF}-VC.zip" "${DIR_TTF}" "${DIR_LIB}/SDL2_ttf-${VER_TTF}")
		downloadLib("https://netix.dl.sourceforge.net/project/glew/glew/${VER_GLEW}/glew-${VER_GLEW}-win32.zip" "${DIR_GLEW}" "${DIR_LIB}/glew-${VER_GLEW}")
		compileLib("https://github.com/assimp/assimp/archive/refs/tags/v${VER_ASSIMP}.zip" "${DIR_ASSIMP}" "cmake\;.\;-G\;NMake Makefiles\;${ASSIMP_ARGS};${MAKE_CMD}\;/f\;Makefile\;assimp" "${DIR_ASSIMP}" "${DIR_LIB}/assimp-${VER_ASSIMP}")
		if(UPDATE_CHECK)
			compileLib("https://curl.se/download/curl-${VER_CURL}.zip" "${DIR_CURL}" "${MAKE_CMD}\;/f\;Makefile.vc\;mode=dll\;MACHINE=${VMACHINE}" "${DIR_CURL}/winbuild" "${DIR_LIB}/curl-${VER_CURL}")
		endif()
		downloadLibOpenVr("${DIR_OVR}" "win32" "win64" "${DIR_LIB}/openvr-${VER_OVR}")
		link_directories("${BIN_SDL}" "${BIN_IMG}" "${BIN_TTF}" "${DIR_GLEW}/lib/Release/${VMARCH}")
		include_directories("${DIR_SDL}/include" "${DIR_IMG}/include" "${DIR_TTF}/include" "${DIR_GLEW}/include" "${DIR_ASSIMP}/include" "$<$<BOOL:${UPDATE_CHECK}>:${OUT_CURL}/include>")
	else()
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(VMACHINE "win64")
			set(VMARCH "x86_64")
		else()
			set(VMACHINE "win32")
			set(VMARCH "i686")
		endif()
		set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}-mingw")
		set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}-mingw")
		set(DIR_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}-mingw")
		set(DIR_GLEW "${DIR_LIB}/glew-${VER_GLEW}-${VMACHINE}-mingw")
		set(DIR_ASSIMP "${DIR_LIB}/assimp-${VER_ASSIMP}-${VMACHINE}-mingw")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}-${VMACHINE}-mingw")
		set(DIR_OVR "${DIR_LIB}/openvr-${VER_OVR}-${VMACHINE}-mingw")
		set(BIN_SDL "${DIR_SDL}/${VMARCH}-w64-mingw32/bin")
		set(BIN_IMG "${DIR_IMG}/${VMARCH}-w64-mingw32/bin")
		set(BIN_TTF "${DIR_TTF}/${VMARCH}-w64-mingw32/bin")
		set(BIN_CURL "${DIR_CURL}/bin")
		set(BIN_OVR "${DIR_OVR}/bin/${VMACHINE}")

		downloadLib("https://github.com/libsdl-org/SDL/releases/download/release-${VER_SDL}/SDL2-devel-${VER_SDL}-mingw.zip" "${DIR_SDL}" "${DIR_LIB}/SDL2-${VER_SDL}")
		downloadLib("https://github.com/libsdl-org/SDL_image/releases/download/release-${VER_IMG}/SDL2_image-devel-${VER_IMG}-mingw.zip" "${DIR_IMG}" "${DIR_LIB}/SDL2_image-${VER_IMG}")
		downloadLib("https://github.com/libsdl-org/SDL_ttf/releases/download/release-${VER_TTF}/SDL2_ttf-devel-${VER_TTF}-mingw.zip" "${DIR_TTF}" "${DIR_LIB}/SDL2_ttf-${VER_TTF}")
		compileLib("https://jztkft.dl.sourceforge.net/project/glew/glew/${VER_GLEW}/glew-${VER_GLEW}.zip" "${DIR_GLEW}" "${MAKE_CMD}\;glew.lib.static" "${DIR_GLEW}" "${DIR_LIB}/glew-${VER_GLEW}")
		compileLib("https://github.com/assimp/assimp/archive/refs/tags/v${VER_ASSIMP}.tar.gz" "${DIR_ASSIMP}" "cmake\;.\;-G\;MinGW Makefiles\;${ASSIMP_ARGS};${MAKE_CMD}\;assimp" "${DIR_ASSIMP}" "${DIR_LIB}/assimp-${VER_ASSIMP}")
		if(UPDATE_CHECK)
			downloadLib("https://curl.se/windows/dl-${VER_CURL}/curl-${VER_CURL}-${VMACHINE}-mingw.zip" "${DIR_CURL}" "")
		endif()
		if(OPENVR)
			compileLib("https://github.com/ValveSoftware/openvr/archive/refs/tags/v${VER_OVR}.zip" "${DIR_OVR}" "sed\;-i\;'s/vrcore/vrcommon/g'\;src/*.{cpp,h}\;src/*/*.{cpp,h};sed\;-i\;-r\;'s/([^_])WIN32/\\1_WIN32/g'\;src/*.{cpp,h}\;src/*/*.{cpp,h};sed\;-i\;-r\;'s/.*assert\\.h.*//g'\;src/*.{cpp,h}\;src/*/*.{cpp,h};sed\;-i\;'s/AssertMsg*//g'\;src/*.{cpp,h}\;src/*/*.{cpp,h};cmake\;.\;-G\;MinGW Makefiles\;-DCMAKE_BUILD_TYPE=Release\;-DBUILD_SHARED=1;${MAKE_CMD}" "${DIR_OVR}" "${DIR_LIB}/openvr-${VER_OVR}")
		endif()
		link_directories("${DIR_SDL}/${VMARCH}-w64-mingw32/lib" "${DIR_IMG}/${VMARCH}-w64-mingw32/lib" "${DIR_TTF}/${VMARCH}-w64-mingw32/lib" "${DIR_GLEW}/lib" "$<$<BOOL:${OPENVR}>:${BIN_OVR}>")
		include_directories("${DIR_SDL}/${VMARCH}-w64-mingw32/include/SDL2" "${DIR_IMG}/${VMARCH}-w64-mingw32/include/SDL2" "${DIR_TTF}/${VMARCH}-w64-mingw32/include/SDL2" "${DIR_GLEW}/include" "${DIR_ASSIMP}/include" "$<$<BOOL:${UPDATE_CHECK}>:${DIR_CURL}/include>" "$<$<BOOL:${OPENVR}>:${DIR_OVR}/headers>")
	endif()
elseif(APPLE)
	set(MACOSX_BUNDLE_INFO_PLIST "rsc/Info.plist")
	set(PBOUT_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents")
	set(DATA_DIR "${PBOUT_DIR}/Resources")
	set(DOCS_DIR "${DATA_DIR}/doc")
	set(LICN_DIR "${DATA_DIR}/licenses")
	set(LIBS_DIR "${PBOUT_DIR}/Frameworks")
	set(FRAM_SDL "SDL2.framework")
	set(FRAM_IMG "SDL2_image.framework")
	set(FRAM_TTF "SDL2_ttf.framework")

	downloadLib("https://github.com/libsdl-org/SDL/releases/download/release-${VER_SDL}/SDL2-${VER_SDL}.dmg" "${DIR_LIB}/${FRAM_SDL}" "")
	downloadLib("https://github.com/libsdl-org/SDL_image/releases/download/release-${VER_IMG}/SDL2_image-${VER_IMG}.dmg" "${DIR_LIB}/${FRAM_IMG}" "")
	downloadLib("https://github.com/libsdl-org/SDL_ttf/releases/download/release-${VER_TTF}/SDL2_ttf-${VER_TTF}.dmg" "${DIR_LIB}/${FRAM_TTF}" "")
	find_library(LIB_SDL "SDL2" HINTS "${DIR_LIB}")
	find_library(LIB_IMG "SDL2_image" HINTS "${DIR_LIB}")
	find_library(LIB_TTF "SDL2_ttf" HINTS "${DIR_LIB}")
	find_library(LIB_OPENGL "OpenGL")
	include_directories("${LIB_SDL}/Headers")	# XCode can't find the SDL2 headers

	if(NOT SYSLIBS)
		set(DIR_ASSIMP "${DIR_LIB}/assimp-${VER_ASSIMP}")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}")
		set(DIR_OVR "${DIR_LIB}/openvr-${VER_OVR}")
		compileLib("https://github.com/assimp/assimp/archive/refs/tags/v${VER_ASSIMP}.tar.gz" "${DIR_ASSIMP}" "cmake\;.\;${ASSIMP_ARGS};${MAKE_CMD}\;assimp" "${DIR_ASSIMP}" "")
		if(UPDATE_CHECK)
			downloadLib("https://curl.se/download/curl-${VER_CURL}.tar.gz" "${DIR_CURL}" "")
		endif()
		downloadLibOpenVr("${DIR_OVR}" "osx32" "osx32" "")
		link_directories("${DIR_ASSIMP}/bin")
		include_directories("${DIR_ASSIMP}/include" "$<$<BOOL:${UPDATE_CHECK}>:${DIR_CURL}/include>")
	endif()
else()
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(VMARCH "x86_64")
	else()
		set(VMARCH "i386")
	endif()
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

	if(NOT OPENGLES)
		set(DIR_GLEW "${DIR_LIB}/glew-${VER_GLEW}")
		compileLib("https://sourceforge.net/projects/glew/files/glew/${VER_GLEW}/glew-${VER_GLEW}.tgz" "${DIR_GLEW}" "${MAKE_CMD}\;glew.lib.static" "${DIR_GLEW}" "")
		link_directories("${DIR_GLEW}/lib")
		include_directories("${DIR_GLEW}/include")
	endif()

	if(NOT SYSLIBS)
		set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}")
		set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}")
		set(DIR_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}")
		set(DIR_ASSIMP "${DIR_LIB}/assimp-${VER_ASSIMP}")
		set(DIR_CURL "${DIR_LIB}/curl-${VER_CURL}")
		set(DIR_OVR "${DIR_LIB}/openvr-${VER_OVR}")
		set(DIR_TMP "${DIR_LIB}/tmp")
		set(ENV{PKG_CONFIG_PATH} "${DIR_TMP}/lib/pkgconfig")

		compileLib("https://github.com/libsdl-org/SDL/releases/download/release-${VER_SDL}/SDL2-${VER_SDL}.tar.gz" "${DIR_SDL}" "./configure\;--prefix=${DIR_TMP};${MAKE_CMD};make\;install" "${DIR_SDL}" "")
		compileLib("https://github.com/libsdl-org/SDL_image/releases/download/release-${VER_IMG}/SDL2_image-${VER_IMG}.tar.gz" "${DIR_IMG}" "./configure\;--prefix=${DIR_TMP};${MAKE_CMD};make\;install" "${DIR_IMG}" "")
		compileLib("https://github.com/libsdl-org/SDL_ttf/releases/download/release-${VER_TTF}/SDL2_ttf-${VER_TTF}.tar.gz" "${DIR_TTF}" "./configure\;--prefix=${DIR_TMP};${MAKE_CMD};make\;install" "${DIR_TTF}" "")
		compileLib("https://github.com/assimp/assimp/archive/refs/tags/v${VER_ASSIMP}.tar.gz" "${DIR_ASSIMP}" "cmake\;.\;${ASSIMP_ARGS};${MAKE_CMD}\;assimp" "${DIR_ASSIMP}" "")
		if(UPDATE_CHECK)
			downloadLib("https://curl.se/download/curl-${VER_CURL}.tar.gz" "${DIR_CURL}" "")
		endif()
		downloadLibOpenVr("${DIR_OVR}" "linux32" "linux64" "")
		include_directories("${DIR_TMP}/include" "${DIR_ASSIMP}/include" "$<$<BOOL:${UPDATE_CHECK}>:${DIR_CURL}/include>")
		link_directories("${DIR_TMP}/lib" "${DIR_ASSIMP}/bin")
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

	downloadLib("https://github.com/libsdl-org/SDL/releases/download/release-${VER_SDL}/SDL2-${VER_SDL}.tar.gz" "${DIR_ASDL}" "")
	downloadLib("https://github.com/libsdl-org/SDL_image/releases/download/release-${VER_IMG}/SDL2_image-${VER_IMG}.tar.gz" "${DIR_AIMG}" "")
	downloadLib("https://github.com/libsdl-org/SDL_ttf/releases/download/release-${VER_TTF}/SDL2_ttf-${VER_TTF}.tar.gz" "${DIR_ATTF}" "")
	renameLibdir("${DIR_ALIB}/SDL2-${VER_SDL}" "${DIR_ASDL}")
	renameLibdir("${DIR_ALIB}/SDL2_image-${VER_IMG}" "${DIR_AIMG}")
	renameLibdir("${DIR_ALIB}/SDL2_ttf-${VER_TTF}" "${DIR_ATTF}")
	file(COPY "${DIR_LIB}/glm" DESTINATION "${DIR_ALIB}")
	file(COPY "${DIR_APRJ}/gradle" "${DIR_APRJ}/build.gradle" "${DIR_APRJ}/gradle.properties" "${DIR_APRJ}/gradlew" "${DIR_APRJ}/gradlew.bat" "${DIR_APRJ}/settings.gradle" DESTINATION "${DIR_PAND}")
	file(COPY "${DIR_APRJ}/app/src/main/java/org/libsdl" DESTINATION "${DIR_PAND}/app/src/main/java/org")
	copyRename("${DIR_ASDL}/LICENSE.txt" "${DIR_ALIC}/LICENSE.SDL.txt")
	copyRename("${DIR_AIMG}/LICENSE.txt" "${DIR_ALIC}/LICENSE.SDL_image.txt")
	copyRename("${DIR_ATTF}/LICENSE.txt" "${DIR_ALIC}/LICENSE.SDL_ttf.txt")
	copyRename("${DIR_ALIB}/glm/copying.txt" "${DIR_ALIC}/COPYING.glm.txt")
	file(COPY "${DIR_RSC}/data/Copyrights.Kurinto.txt" "${DIR_RSC}/data/LICENSE.Merriweather.txt" DESTINATION "${DIR_ALIC}")

	file(READ "${DIR_AIMG}/Android.mk" MTXT)
	string(REPLACE "SUPPORT_WEBP ?= true" "SUPPORT_WEBP ?= false" MTXT "${MTXT}")
	file(WRITE "${DIR_AIMG}/Android.mk" "${MTXT}")
endif()

# compiler/linker flags

add_compile_definitions($<$<BOOL:${EXTERNAL}>:EXTERNAL> $<$<BOOL:${SERVICE}>:SERVICE> $<$<BOOL:${UPDATE_CHECK}>:UPDATE_CHECK> $<$<BOOL:${SYSLIBS}>:SYSLIBS>
						$<$<OR:$<BOOL:${OPENGLES}>,$<BOOL:${EMSCRIPTEN}>>:$<IF:$<STREQUAL:${OPENGLES},32>,OPENGLES=32,OPENGLES=1>>
						$<$<BOOL:${APPLE}>:GL_SILENCE_DEPRECATION>
						"$<$<BOOL:${WIN32}>:UNICODE;_UNICODE;_CRT_SECURE_NO_WARNINGS;NOMINMAX;$<$<NOT:$<BOOL:${MSVC}>>:_WIN32_WINNT=0x600>>"
						$<$<CONFIG:Debug>:MALLOC_CHECK_=2> $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>,$<CONFIG:RelWithDebInfo>>:NDEBUG>)	# NDEBUG doesn't exist on some platforms (macOS)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	add_compile_options(-Wall -Wextra -pedantic-errors -Wfloat-conversion -Wformat=2 -Wmissing-declarations -Wold-style-cast -Wshadow -Wunreachable-code -Wno-implicit-fallthrough -Wno-old-style-cast -Wno-switch $<$<NOT:$<BOOL:${MINGW}>>:-Wdouble-promotion> $<$<BOOL:${OPENGLES}>:-Wno-unused-parameter>)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-Wcast-align=strict -Wduplicated-cond -Wno-maybe-uninitialized -Wno-volatile)
	else()
		add_compile_options(-Wcast-align -Wimplicit-int-float-conversion -Wnull-dereference -Wover-aligned -Wshadow-all -Wno-final-dtor-non-final-class -Wno-unused-private-field)
	endif()

	if(EMSCRIPTEN)
		add_compile_options(-sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS=[jpg,png] -sUSE_SDL_TTF=2 -sDISABLE_EXCEPTION_CATCHING=0 -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-parameter -Wno-unused-variable $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-flto>)
		add_link_options(-sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS=[jpg,png] -sUSE_SDL_TTF=2 -sDISABLE_EXCEPTION_CATCHING=0 $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-flto>)
	else()
		if(ARCH)
			add_compile_options(-march=${ARCH})
		elseif(CMAKE_SYSTEM_PROCESSOR MATCHES ".86.64")
			add_compile_options(-march=x86-64)
		endif()
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			add_compile_options("$<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-flto;-fuse-linker-plugin>")
			add_link_options("$<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-flto;-fuse-linker-plugin;-s>")
		endif()
	endif()
elseif(MSVC)
	add_compile_options(/W4 /wd4201 /wd4244 /wd4267 /wd4706 $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:/GL>)
endif()

# main target

if(NOT EMSCRIPTEN AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
	find_program(STRIPPER "strip")
	if(NOT STRIPPER)
		message(WARNING "Can't strip executables: Failed to find stip")
	endif()
endif()

function(setMainTargetProperties TARGET_NAME OUT_DIR)
	set_target_properties(${TARGET_NAME} PROPERTIES
							RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}"
							RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUT_DIR}"
							RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUT_DIR}"
							RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUT_DIR}"
							RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${OUT_DIR}")
	if(NOT WIN32)
		set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME $<LOWER_CASE:${TARGET_NAME}>)
	endif()
	if(STRIPPER)
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
							COMMAND "$<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:${STRIPPER};-s;$<TARGET_FILE:${TARGET_NAME}>>"
							COMMAND_EXPAND_LISTS)
	endif()
endfunction()

function(setMainTargetLinks TARGET_NAME)
	if(WIN32)
		target_link_libraries(${TARGET_NAME} SDL2 SDL2_image SDL2_ttf $<IF:$<BOOL:${MSVC}>,glew32s,glew32> opengl32 ws2_32)
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_SDL}/SDL2.dll" "${BIN_IMG}/SDL2_image.dll" "${BIN_TTF}/SDL2_ttf.dll" "$<TARGET_FILE_DIR:${TARGET_NAME}>"
							COMMAND "$<$<BOOL:${UPDATE_CHECK}>:${CMAKE_COMMAND};-E;copy_if_different;${BIN_CURL}/$<IF:$<OR:$<BOOL:${MSVC}>,$<NOT:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>>>,libcurl.dll,libcurl-x64.dll>;$<TARGET_FILE_DIR:${TARGET_NAME}>>"
							COMMAND_EXPAND_LISTS)
	elseif(APPLE)
		target_link_libraries(${TARGET_NAME} "${LIB_SDL}" "${LIB_IMG}" "${LIB_TTF}" "${LIB_OPENGL}")
	else()
		target_link_libraries(${TARGET_NAME} SDL2 SDL2_image SDL2_ttf $<IF:$<BOOL:${OPENGLES}>,GLESv2,GLEW;GL> $<$<BOOL:${UPDATE_CHECK}>:dl>)
		if(NOT SYSLIBS AND NOT APPIMAGE)
			file(GLOB SOLIB_FILES "${DIR_TMP}/lib/*.so")
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
								COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${SOLIB_FILES} "$<TARGET_FILE_DIR:${TARGET_NAME}>")
		endif()
	endif()
endfunction()

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${THRONES_SRC})
if(EMSCRIPTEN)
	setMainTargetProperties(${PROJECT_NAME} "${CMAKE_BINARY_DIR}")
	target_link_libraries(${PROJECT_NAME} "--preload-file ${CMAKE_BINARY_DIR}/share@/" "-s TOTAL_MEMORY=536870912" "-s WASM=1" "-s USE_WEBGL2=1" "-s FETCH=1" "-s DYNAMIC_EXECUTION=1" "idbfs.js"
							"$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-gsource-map;--source-map-base http://localhost:8080/;-s ASSERTIONS=2;-s STACK_OVERFLOW_CHECK=2;-s SAFE_HEAP=1;-s DEMANGLE_SUPPORT=1>")
	return()
endif()
setMainTargetProperties(${PROJECT_NAME} "$<IF:$<BOOL:${WIN32}>,${PBOUT_DIR},$<IF:$<OR:$<BOOL:${APPLE}>,$<BOOL:${APPIMAGE}>>,${PBOUT_DIR}/MacOS,${TBIN_DIR}>>")
setMainTargetLinks(${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} ${DATA_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DPI_AWARE "PerMonitor")
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

if(OPENVR)
	add_executable(${GAMEVR_NAME} WIN32 MACOSX_BUNDLE ${THRONES_SRC})
	setMainTargetProperties(${GAMEVR_NAME} "$<IF:$<BOOL:${WIN32}>,${PBOUT_DIR},$<IF:$<OR:$<BOOL:${APPLE}>,$<BOOL:${APPIMAGE}>>,${PBOUT_DIR}/MacOS,${TBIN_DIR}>>")
	setMainTargetLinks(${GAMEVR_NAME})
	target_compile_definitions(${GAMEVR_NAME} PUBLIC OPENVR)
	add_dependencies(${GAMEVR_NAME} ${DATA_NAME})
	set_property(TARGET ${GAMEVR_NAME} PROPERTY VS_DPI_AWARE "PerMonitor")

	if(WIN32)
		target_link_libraries(${GAMEVR_NAME} $<IF:$<BOOL:${MSVC}>,openvr_api,openvr_api$<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:64>.dll>)
		add_custom_command(TARGET ${GAMEVR_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_OVR}/$<IF:$<BOOL:${MSVC}>,openvr_api.dll,libopenvr_api$<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:64>.dll>" "$<TARGET_FILE_DIR:${GAMEVR_NAME}>")
	else()
		target_link_libraries(${GAMEVR_NAME} openvr_api)
	endif()
endif()

if(APPLE)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBS_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_SDL}" "${LIBS_DIR}/${FRAM_SDL}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_IMG}" "${LIBS_DIR}/${FRAM_IMG}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_TTF}" "${LIBS_DIR}/${FRAM_TTF}")
elseif(APPIMAGE)
	find_program(LINUXDEPLOY "linuxdeploy")
	if(NOT LINUXDEPLOY)
		set(LDPEXE_NAME "linuxdeploy-${VMARCH}.AppImage")
		set(LINUXDEPLOY "${DIR_LIB}/${LDPEXE_NAME}")
		if(EXISTS "${LINUXDEPLOY}")
			message(STATUS "Found ${LINUXDEPLOY}")
		else()
			message(STATUS "Downloading ${LINUXDEPLOY}")
			file(DOWNLOAD "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/${LDPEXE_NAME}" "${LINUXDEPLOY}")
			file(CHMOD "${LINUXDEPLOY}" FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
		endif()
	endif()
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E create_symlink "usr/bin/${PROJECT_NAME_LOWER}" "${PBOUT_DIR}/AppRun"
						COMMAND "${CMAKE_COMMAND}" -E create_symlink "usr/share/${PROJECT_NAME_LOWER}/${PROJECT_NAME_LOWER}.png" "${PBOUT_DIR}/${PROJECT_NAME_LOWER}.png"
						COMMAND bash -c "VERSION='${PROJECT_VERSION}' '${LINUXDEPLOY}' --appdir '${PBOUT_DIR}' --output appimage")
endif()

# server program target

add_executable(${SERVER_NAME} ${SERVER_SRC})
setMainTargetProperties(${SERVER_NAME} "$<IF:$<BOOL:${WIN32}>,${PBOUT_DIR},$<IF:$<OR:$<BOOL:${APPLE}>,$<BOOL:${APPIMAGE}>>,${CMAKE_BINARY_DIR},${TBIN_DIR}>>")
target_link_libraries(${SERVER_NAME} $<$<BOOL:${WIN32}>:ws2_32>)

# asset building program target

add_executable(${OVEN_NAME} ${OVEN_SRC})
if(WIN32)
	if(MSVC)
		file(GLOB ASSIMP_DLL "${DIR_ASSIMP}/bin/assimp*.dll")
		file(GLOB ASSIMP_LIB "${DIR_ASSIMP}/lib/assimp*.lib")
	else()
		file(GLOB ASSIMP_DLL "${DIR_ASSIMP}/bin/libassimp*.dll")
		file(GLOB ASSIMP_LIB "${DIR_ASSIMP}/lib/libassimp*.a")
	endif()
	add_custom_command(TARGET ${OVEN_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_SDL}/SDL2.dll" "${BIN_IMG}/SDL2_image.dll" "${ASSIMP_DLL}" "$<TARGET_FILE_DIR:${OVEN_NAME}>")
endif()
target_link_libraries(${OVEN_NAME} "$<IF:$<BOOL:${APPLE}>,${LIB_SDL};${LIB_IMG},SDL2;SDL2_image>" "$<IF:$<BOOL:${WIN32}>,${ASSIMP_LIB},assimp>")

# game data copier targets

function(addDataCopierTarget TARGET_NAME TEXTRA DST_DATA OPT_SHD ASS_IMG IMG_SCALE)
	if(TEXDIV)
		set(IMG_SCALE ${TEXDIV})
	endif()
	set(DST_FONT "${DST_DATA}/fonts")
	set(DST_AUDIO "${DST_DATA}/audio")
	set(DST_TEXS "${DST_DATA}/textures")
	add_custom_target(${TARGET_NAME} ${TEXTRA} SOURCES ${DATA_SRC}
						COMMAND "${CMAKE_COMMAND}" -E make_directory "${DST_FONT}" "${DST_AUDIO}" "${DST_TEXS}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/kurinto.ttf" "${DIR_RSC}/data/merriweather.otf" "${DIR_RSC}/data/romanesque.ttf" "${DST_FONT}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${ASSET_WAV} "${DST_AUDIO}"
						COMMAND "$<TARGET_FILE:${OVEN_NAME}>" -m "${DST_DATA}/materials.dat" ${ASSET_MTL}
						COMMAND "$<TARGET_FILE:${OVEN_NAME}>" -o "${DST_DATA}/objects/" ${ASSET_OBJ}
						COMMAND "$<TARGET_FILE:${OVEN_NAME}>" ${OPT_SHD} "${DST_DATA}/shaders/" ${ASSET_SHD}
						WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	if(IMG_SCALE GREATER 1)
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
							COMMAND "$<TARGET_FILE:${OVEN_NAME}>" -t "${DST_TEXS}/" -d ${IMG_SCALE} ${ASS_IMG}
							WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	else()
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${ASS_IMG} "${DST_TEXS}"
							WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	endif()
	add_dependencies(${TARGET_NAME} ${OVEN_NAME})
endfunction()

addDataCopierTarget(${DATA_NAME} "ALL" "${DATA_DIR}" $<IF:$<BOOL:${OPENGLES}>,-S,-s> "${ASSET_IMG_EXT}" $<IF:$<BOOL:${OPENGLES}>,2,0>)
add_custom_command(TARGET ${DATA_NAME} POST_BUILD
					COMMAND "${CMAKE_COMMAND}" -E make_directory "${LICN_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/doc" "${DOCS_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/${PROJECT_NAME_LOWER}.png" "${DIR_RSC}/data/cursor.png" "${DATA_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/Copyrights.Kurinto.txt" "${DIR_RSC}/data/LICENSE.Merriweather.txt" "${LICN_DIR}"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_LIB}/glm/copying.txt" "${LICN_DIR}/COPYING.glm.txt")
if(WIN32 OR (NOT APPLE AND NOT SYSLIBS))
	add_custom_command(TARGET ${DATA_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_SDL}/$<IF:$<BOOL:${WIN32}>,COPYING,LICENSE>.txt" "${LICN_DIR}/$<IF:$<BOOL:${WIN32}>,COPYING,LICENSE>.SDL.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_IMG}/LICENSE.txt" "${LICN_DIR}/LICENSE.SDL_image.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_TTF}/LICENSE.txt" "${LICN_DIR}/LICENSE.SDL_ttf.txt")
endif()
if(WIN32)
	add_custom_command(TARGET ${DATA_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_GLEW}/LICENSE.txt" "${LICN_DIR}/LICENSE.glew.txt"
						COMMAND "$<$<BOOL:${UPDATE_CHECK}>:${CMAKE_COMMAND};-E;copy_if_different;${DIR_CURL}/$<IF:$<BOOL:${MSVC}>,COPYING,COPYING.txt>;${LICN_DIR}/COPYING.curl.txt>"
						COMMAND "$<$<BOOL:${MSVC}>:${CMAKE_COMMAND};-E;copy_if_different;${BIN_TTF}/LICENSE.freetype.txt;${BIN_TTF}/LICENSE.harfbuzz.txt;${BIN_TTF}/LICENSE.zlib.txt;${LICN_DIR}>"
						COMMAND "$<$<BOOL:${UPDATE_CHECK}>:$<$<NOT:$<BOOL:${MSVC}>>:${CMAKE_COMMAND};-E;copy_if_different;${DIR_CURL}/COPYING-brotli.txt;${DIR_CURL}/COPYING-libidn2.txt;${DIR_CURL}/COPYING-libssh2.txt;${DIR_CURL}/COPYING-nghttp2.txt;${DIR_CURL}/COPYING-openssl.txt;${DIR_CURL}/COPYING-zlib.txt;${LICN_DIR}>>"
						COMMAND_EXPAND_LISTS)
elseif(APPLE)
	add_custom_command(TARGET ${DATA_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/${PROJECT_NAME_LOWER}.icns" "${DATA_DIR}"
						COMMAND "$<$<BOOL:${UPDATE_CHECK}>:${CMAKE_COMMAND};-E;copy_if_different;${DIR_CURL}/COPYING;${LICN_DIR}/COPYING.curl.txt>")
else()
	add_custom_command(TARGET ${DATA_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/${PROJECT_NAME_LOWER}.desktop" "${PBOUT_DIR}"
						COMMAND "$<$<NOT:$<BOOL:${OPENGLES}>>:${CMAKE_COMMAND};-E;copy_if_different;${DIR_GLEW}/LICENSE.txt;${LICN_DIR}/LICENSE.glew.txt>"
						COMMAND_EXPAND_LISTS)
	if(NOT SYSLIBS)
		add_custom_command(TARGET ${DATA_NAME} POST_BUILD
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_TTF}/external/freetype/LICENSE.TXT" "${LICN_DIR}/LICENSE.freetype.txt"
							COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_TTF}/external/harfbuzz/COPYING" "${LICN_DIR}/COPYING.harfbuzz.txt")
	endif()
endif()
if(OPENVR)
	add_custom_command(TARGET ${DATA_NAME} POST_BUILD
						COMMAND "$<$<OR:$<BOOL:${WIN32}>,$<NOT:$<BOOL:${SYSLIBS}>>>:${CMAKE_COMMAND};-E;copy_if_different;${DIR_OVR}/LICENSE;${LICN_DIR}/LICENSE.openvr.txt>"
						COMMAND "$<TARGET_FILE:${OVEN_NAME}>" -j "${DATA_DIR}/" "${DIR_RSC}/data/vr_actions.json" "${DIR_RSC}/data/vr_bindings_generic.json" "${DIR_RSC}/data/vr_bindings_vive.json"
						COMMAND_EXPAND_LISTS)
endif()
addDataCopierTarget(${ADATA_NAME} "" "${DIR_PAND}/app/src/main/assets" "-S" "${ASSET_IMG}" "2")

get_filename_component(EMS_DIR "${EMBASE}" ABSOLUTE)
addDataCopierTarget(${EDATA_NAME} "" "${EMS_DIR}/share" "-S" "${ASSET_IMG}" "2")
add_custom_command(TARGET ${EDATA_NAME} POST_BUILD
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/doc" "${EMS_DIR}/doc"
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/${PROJECT_NAME_LOWER}.png" "${EMS_DIR}")

# tests

enable_testing()
add_executable(${TESTS_NAME} EXCLUDE_FROM_ALL ${TESTS_SRC})
setMainTargetLinks(${TESTS_NAME})
add_test(NAME ${TESTS_NAME} COMMAND ${TESTS_NAME})

# prettyfiers

set(ALL_SRC ${THRONES_SRC} ${DATA_SRC} ${SERVER_SRC} ${OVEN_SRC} ${TESTS_SRC})
foreach(FSRC IN LISTS ALL_SRC)
	get_filename_component(FGRP "${FSRC}" DIRECTORY)
	string(REPLACE "/" ";" FGRP "${FGRP}")
	list(REMOVE_AT FGRP 0)
	string(REPLACE ";" "/" FGRP "${FGRP}")
	source_group("${FGRP}" FILES "${FSRC}")
endforeach()
