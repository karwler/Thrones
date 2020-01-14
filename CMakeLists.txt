cmake_minimum_required(VERSION 3.10.2)
project(Thrones LANGUAGES CXX)
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")
endif()

# target names and source/resource files

set(SERVER_NAME "Server")
set(DATA_NAME "assets")
set(ADATA_NAME "${DATA_NAME}_android")
set(OVEN_NAME "oven")

set(ASSET_WAV
	"rsc/audio/ammo.wav"
	"rsc/audio/move.wav")

set(ASSET_DAT
	"rsc/data/cursor.png"
	"rsc/data/merriweather.otf"
	"rsc/data/romanesque.ttf"
	"rsc/data/thrones.png")

set(ASSET_MTL
	"rsc/materials/regular.mtl")

set(ASSET_OBJ
	"rsc/objects/breached.obj"
	"rsc/objects/catapult.obj"
	"rsc/objects/crossbowman.obj"
	"rsc/objects/dragon.obj"
	"rsc/objects/elephant.obj"
	"rsc/objects/fortress.obj"
	"rsc/objects/ground.obj"
	"rsc/objects/lancer.obj"
	"rsc/objects/outline.obj"
	"rsc/objects/plane.obj"
	"rsc/objects/ranger.obj"
	"rsc/objects/screen.obj"
	"rsc/objects/spearman.obj"
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
	"rsc/textures/catapult.png"
	"rsc/textures/crossbowman.png"
	"rsc/textures/crosshair.png"
	"rsc/textures/dragon.png"
	"rsc/textures/elephant.png"
	"rsc/textures/forest.png"
	"rsc/textures/fortress.png"
	"rsc/textures/grass.png"
	"rsc/textures/lancer.png"
	"rsc/textures/mountain.png"
	"rsc/textures/plains.png"
	"rsc/textures/ranger.png"
	"rsc/textures/rock.png"
	"rsc/textures/spearman.png"
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
	"src/engine/scene.cpp"
	"src/engine/scene.h"
	"src/engine/windowSys.cpp"
	"src/engine/windowSys.h"
	"src/engine/world.cpp"
	"src/engine/world.h"
	"src/oven/oven.cpp"
	"src/oven/oven.h"
	"src/prog/game.cpp"
	"src/prog/game.h"
	"src/prog/netcp.cpp"
	"src/prog/netcp.h"
	"src/prog/program.cpp"
	"src/prog/program.h"
	"src/prog/progs.cpp"
	"src/prog/progs.h"
	"src/server/server.cpp"
	"src/server/server.h"
	"src/utils/layouts.cpp"
	"src/utils/layouts.h"
	"src/utils/objects.cpp"
	"src/utils/objects.h"
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
	"src/server/server.cpp"
	"src/server/server.h"
	"src/server/serverProg.cpp"
	"src/utils/text.cpp"
	"src/utils/text.h")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	list(APPEND SERVER_SRC "rsc/server.rc")
endif()

set(OVEN_SRC
	"src/oven/oven.cpp"
	"src/oven/oven.h"
	"src/oven/ovenProg.cpp"
	"src/utils/text.cpp"
	"src/utils/text.h")

# dependencies

if(NOT VER_SDL)
	set(VER_SDL "2.0.10")
endif()
if(NOT VER_IMG)
	set(VER_IMG "2.0.5")
endif()
if(NOT VER_NET)
	set(VER_NET "2.0.1")
endif()
if(NOT VER_TTF)
	set(VER_TTF "2.0.15")
endif()
if(NOT VER_GLEW)
	set(VER_GLEW "2.1.0")
endif()
if(NOT VER_GLM)
	set(VER_GLM "0.9.9.7")
endif()
set(DIR_LIB "${CMAKE_SOURCE_DIR}/lib")
set(DIR_RSC "${CMAKE_SOURCE_DIR}/rsc")

function(downloadLib URL DST LIBDIR)
	get_filename_component(LIBDIR "${LIBDIR}" NAME)
	if(EXISTS "${DST}/${LIBDIR}")
		message(STATUS "Found ${DST}/${LIBDIR}")
		return()
	endif()

	get_filename_component(FILE "${URL}" NAME)
	if(EXISTS "${DST}/${FILE}")
		message(STATUS "Found ${DST}/${FILE}")
	else()
		message(STATUS "Downloading ${URL}")
		file(DOWNLOAD "${URL}" "${DST}/${FILE}")
	endif()

	if(FILE MATCHES ".zip$")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf "${FILE}" WORKING_DIRECTORY "${DST}")
	elseif(FILE MATCHES ".tar.gz$")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xzf "${FILE}" WORKING_DIRECTORY "${DST}")
	elseif(FILE MATCHES ".dmg$")
		execute_process(COMMAND hdiutil attach "${DST}/${FILE}")
		string(FIND "${FILE}" "-" FID)
		string(SUBSTRING "${FILE}" 0 ${FID} FNAME)
		file(COPY "/Volumes/${FNAME}/${FNAME}.framework" DESTINATION "${DST}")
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
downloadLib("https://github.com/g-truc/glm/releases/download/${VER_GLM}/glm-${VER_GLM}.zip" "${DIR_LIB}" "glm")
include_directories("${CMAKE_SOURCE_DIR}/src" "${DIR_LIB}/glm")
if(EMSCRIPTEN)
	file(COPY "${DIR_RSC}/thrones.html" DESTINATION "${CMAKE_BINARY_DIR}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}")
		set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}")
		set(DIR_NET "${DIR_LIB}/SDL2_net-${VER_NET}")
		set(DIR_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}")
		set(DIR_GLEW "${DIR_LIB}/glew-${VER_GLEW}")
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(BIN_SDL "${DIR_SDL}/lib/x64")
			set(BIN_IMG "${DIR_IMG}/lib/x64")
			set(BIN_NET "${DIR_NET}/lib/x64")
			set(BIN_TTF "${DIR_TTF}/lib/x64")
		else()
			set(BIN_SDL "${DIR_SDL}/lib/x86")
			set(BIN_IMG "${DIR_IMG}/lib/x86")
			set(BIN_NET "${DIR_NET}/lib/x86")
			set(BIN_TTF "${DIR_TTF}/lib/x86")
		endif()
		set(LIB_SDL "${BIN_SDL}")
		set(LIB_IMG "${BIN_IMG}")
		set(LIB_NET "${BIN_NET}")
		set(LIB_TTF "${BIN_TTF}")
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(LIB_GLEW "${DIR_GLEW}/lib/Release/x64")
		else()
			set(LIB_GLEW "${DIR_GLEW}/lib/Release/Win32")
		endif()
		set(INC_SDL "${DIR_SDL}/include")
		set(INC_IMG "${DIR_IMG}/include")
		set(INC_NET "${DIR_NET}/include")
		set(INC_TTF "${DIR_TTF}/include")
		set(URL_SDL "https://www.libsdl.org/release/SDL2-devel-${VER_SDL}-VC.zip")
		set(URL_IMG "https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-${VER_IMG}-VC.zip")
		set(URL_NET "https://www.libsdl.org/projects/SDL_net/release/SDL2_net-devel-${VER_NET}-VC.zip")
		set(URL_TTF "https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-${VER_TTF}-VC.zip")
		set(URL_GLEW "https://netix.dl.sourceforge.net/project/glew/glew/2.1.0/glew-${VER_GLEW}-win32.zip")
	else()
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}/x86_64-w64-mingw32")
			set(DIR_IMG "${DIR_LIB}/SDL2_image-${VER_IMG}/x86_64-w64-mingw32")
			set(DIR_NET "${DIR_LIB}/SDL2_net-${VER_NET}/x86_64-w64-mingw32")
			set(DIR_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}/x86_64-w64-mingw32")
		else()
			set(DIR_SDL "${DIR_LIB}/SDL2-${VER_SDL}/i686-w64-mingw32")
			set(DIR_IMG "${DIR_LIB}/SDL2-image-${VER_IMG}/i686-w64-mingw32")
			set(DIR_NET "${DIR_LIB}/SDL2_net-${VER_NET}/i686-w64-mingw32")
			set(DIR_TTF "${DIR_LIB}/SDL2_ttf-${VER_TTF}/i686-w64-mingw32")
		endif()
		set(DIR_GLEW "${DIR_LIB}/glew-${VER_GLEW}")
		set(BIN_SDL "${DIR_SDL}/bin")
		set(BIN_IMG "${DIR_IMG}/bin")
		set(BIN_NET "${DIR_NET}/bin")
		set(BIN_TTF "${DIR_TTF}/bin")
		set(LIB_SDL "${DIR_SDL}/lib")
		set(LIB_IMG "${DIR_IMG}/lib")
		set(LIB_NET "${DIR_NET}/lib")
		set(LIB_TTF "${DIR_TTF}/lib")
		set(LIB_GLEW "${DIR_GLEW}/lib")
		set(INC_SDL "${DIR_SDL}/include/SDL2")
		set(INC_IMG "${DIR_IMG}/include/SDL2")
		set(INC_NET "${DIR_NET}/include/SDL2")
		set(INC_TTF "${DIR_TTF}/include/SDL2")
		set(URL_SDL "https://www.libsdl.org/release/SDL2-devel-${VER_SDL}-mingw.tar.gz")
		set(URL_IMG "https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-${VER_IMG}-mingw.tar.gz")
		set(URL_NET "https://www.libsdl.org/projects/SDL_net/release/SDL2_net-devel-${VER_NET}-mingw.tar.gz")
		set(URL_TTF "https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-${VER_TTF}-mingw.tar.gz")
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(URL_GLEW "http://www.grhmedia.com/glew/glew-${VER_GLEW}-mingw-w64.zip")
		else()
			set(URL_GLEW "http://www.grhmedia.com/glew/glew-${VER_GLEW}-mingw.zip")
		endif()
	endif()
	downloadLib("${URL_SDL}" "${DIR_LIB}" "${DIR_SDL}")
	downloadLib("${URL_IMG}" "${DIR_LIB}" "${DIR_IMG}")
	downloadLib("${URL_NET}" "${DIR_LIB}" "${DIR_NET}")
	downloadLib("${URL_TTF}" "${DIR_LIB}" "${DIR_TTF}")
	downloadLib("${URL_GLEW}" "${DIR_LIB}" "${DIR_GLEW}")

	link_directories("${LIB_SDL}" "${LIB_IMG}" "${LIB_NET}" "${LIB_TTF}" "${LIB_GLEW}")
	include_directories("${INC_SDL}" "${INC_IMG}" "${INC_NET}" "${INC_TTF}" "${DIR_GLEW}/include")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	downloadLib("https://www.libsdl.org/release/SDL2-${VER_SDL}.dmg" "${DIR_LIB}" "SDL2.framework")
	downloadLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-${VER_IMG}.dmg" "${DIR_LIB}" "SDL2_image.framework")
	downloadLib("https://www.libsdl.org/projects/SDL_net/release/SDL2_net-${VER_NET}.dmg" "${DIR_LIB}" "SDL2_net.framework")
	downloadLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-${VER_TTF}.dmg" "${DIR_LIB}" "SDL2_ttf.framework")

	find_library(LIB_SDL "SDL2" HINTS "${DIR_LIB}")
	find_library(LIB_IMG "SDL2_image" HINTS "${DIR_LIB}")
	find_library(LIB_NET "SDL2_net" HINTS "${DIR_LIB}")
	find_library(LIB_TTF "SDL2_ttf" HINTS "${DIR_LIB}")
	find_library(LIB_OPENGL "OpenGL")
endif()

set(DIR_PAND "${CMAKE_SOURCE_DIR}/android")
if(LIBDROID)
	set(DIR_ALIB "${DIR_PAND}/app/jni")
	set(DIR_ASDL "${DIR_ALIB}/SDL")
	set(DIR_AIMG "${DIR_ALIB}/SDL_image")
	set(DIR_ANET "${DIR_ALIB}/SDL_net")
	set(DIR_ATTF "${DIR_ALIB}/SDL_ttf")
	set(DIR_APRJ "${DIR_ASDL}/android-project")
	set(DIR_ALIC "${DIR_PAND}/app/licenses")

	downloadLib("https://www.libsdl.org/release/SDL2-${VER_SDL}.tar.gz" "${DIR_ALIB}" "${DIR_ASDL}")
	downloadLib("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-${VER_IMG}.tar.gz" "${DIR_ALIB}" "${DIR_AIMG}")
	downloadLib("https://www.libsdl.org/projects/SDL_net/release/SDL2_net-${VER_NET}.tar.gz" "${DIR_ALIB}" "${DIR_ANET}")
	downloadLib("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-${VER_TTF}.tar.gz" "${DIR_ALIB}" "${DIR_ATTF}")
	renameLibdir("${DIR_ALIB}/SDL2-${VER_SDL}" "${DIR_ASDL}")
	renameLibdir("${DIR_ALIB}/SDL2_image-${VER_IMG}" "${DIR_AIMG}")
	renameLibdir("${DIR_ALIB}/SDL2_net-${VER_NET}" "${DIR_ANET}")
	renameLibdir("${DIR_ALIB}/SDL2_ttf-${VER_TTF}" "${DIR_ATTF}")
	file(COPY "${DIR_LIB}/glm" DESTINATION "${DIR_ALIB}")
	file(COPY "${DIR_APRJ}/gradle" "${DIR_APRJ}/build.gradle" "${DIR_APRJ}/gradle.properties" "${DIR_APRJ}/gradlew" "${DIR_APRJ}/gradlew.bat" "${DIR_APRJ}/settings.gradle" DESTINATION "${DIR_PAND}")
	file(COPY "${DIR_APRJ}/app/src/main/java/org/libsdl" DESTINATION "${DIR_PAND}/app/src/main/java/org")
	copyRename("${DIR_ASDL}/COPYING.txt" "${DIR_ALIC}/COPYING.SDL.txt")
	copyRename("${DIR_AIMG}/COPYING.txt" "${DIR_ALIC}/COPYING.SDL_image.txt")
	copyRename("${DIR_ANET}/COPYING.txt" "${DIR_ALIC}/COPYING.SDL_net.txt")
	copyRename("${DIR_ATTF}/COPYING.txt" "${DIR_ALIC}/COPYING.SDL_ttf.txt")
	copyRename("${DIR_ALIB}/glm/copying.txt" "${DIR_ALIC}/COPYING.glm.txt")
	file(COPY "${DIR_RSC}/data/LICENSE.Merriweather.txt" DESTINATION "${DIR_ALIC}")
endif()

# compiler/linker flags

if(EXTERNAL)
	add_definitions(-DEXTERNAL)
endif()
if(OPENGLES OR EMSCRIPTEN)
	add_definitions(-DOPENGLES)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_definitions(-D_UNICODE -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG -D_DEBUG)
else()
	add_definitions(-DNDEBUG)
endif()

set(CXX_COMMON_WARNINGS "-Wno-implicit-fallthrough -Wno-float-equal -Wno-parentheses -Wno-shadow -Wno-shadow-field -Wno-switch -Wno-switch-enum -Wno-unused-private-field -Wno-unused-result")
if(EMSCRIPTEN)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --preload-file ${CMAKE_BINARY_DIR}/data@/ -s TOTAL_MEMORY=536870912 -s WASM=1 -s USE_WEBGL2=1 -s DISABLE_EXCEPTION_CATCHING=0 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='[\"jpg\",\"png\"]' -s USE_SDL_NET=2 -s USE_SDL_TTF=2 -Wall -Wextra -Wpedantic -Wno-unused-parameter ${CXX_COMMON_WARNINGS}")
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g4 -s DISABLE_EXCEPTION_CATCHING=0")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
	endif()
elseif((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	set(FLG_ANY "-fno-pie -no-pie -fno-stack-protector ${CXX_COMMON_WARNINGS}")
	set(FLG_DEBUG "-g")
	set(FLG_RELEASE "-O3 -s -Wno-padded")
	if(OPENGLES)
		set(FLG_ANY "${FLG_ANY} -Wno-unused-parameter")
	endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(FLG_ANY "${FLG_ANY} -Wall -Wextra -Wpedantic")
	else()
		set(FLG_ANY "${FLG_ANY} -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-covered-switch-default -Wno-exit-time-destructors -Wno-global-constructors -Wno-unused-command-line-argument")
	endif()
	if(ARCH)
		set(FLG_ANY "${FLG_ANY} -m${ARCH}")
	endif()

	if(CMAKE_CONFIGURATION_TYPES)
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${FLG_ANY} ${FLG_DEBUG}")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${FLG_ANY} ${FLG_RELEASE}")
	elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLG_ANY} ${FLG_DEBUG}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLG_ANY} ${FLG_RELEASE}")
	endif()
endif()

# main target

function(setCommonTargetProperties TARGET_NAME OUT_DIR)
	set_target_properties(${TARGET_NAME} PROPERTIES
							RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}"
							RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUT_DIR}"
							RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUT_DIR}")
	if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
		string(TOLOWER ${TARGET_NAME} TARGET_NAME_LOWER)
		set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${TARGET_NAME_LOWER})
	endif()
endfunction()

add_executable(${PROJECT_NAME} WIN32 ${THRONES_SRC})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
if(EMSCRIPTEN)
	setCommonTargetProperties(${PROJECT_NAME} "${CMAKE_BINARY_DIR}")
	return()
endif()
add_dependencies(${PROJECT_NAME} ${DATA_NAME})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

set(EXE_DIR "${CMAKE_BINARY_DIR}/bin")
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(APP_DIR "${EXE_DIR}/${PROJECT_NAME}.app/Contents")
	set(DATA_DIR "${APP_DIR}/Resources")
	set(FRAM_DIR "${APP_DIR}/Frameworks")
	set(FRAM_SDL "SDL2.framework")
	set(FRAM_IMG "SDL2_image.framework")
	set(FRAM_NET "SDL2_net.framework")
	set(FRAM_TTF "SDL2_ttf.framework")
	set(LICN_DIR "${DATA_DIR}/licenses")

	target_link_libraries(${PROJECT_NAME} "${LIB_SDL}" "${LIB_IMG}" "${LIB_NET}" "${LIB_TTF}" "${LIB_OPENGL}")
	setCommonTargetProperties(${PROJECT_NAME} "${APP_DIR}/MacOS" "")
else()
	set(DATA_DIR "${EXE_DIR}/data")
	set(LICN_DIR "${EXE_DIR}/licenses")

	target_link_libraries(${PROJECT_NAME} SDL2 SDL2_image SDL2_net SDL2_ttf)
	if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
		target_link_libraries(${PROJECT_NAME} glew32s glu32 opengl32)
	elseif(OPENGLES)
		target_link_libraries(${PROJECT_NAME} GLESv2)
	else()
		target_link_libraries(${PROJECT_NAME} GLEW GLU GL)
	endif()
	setCommonTargetProperties(${PROJECT_NAME} "${EXE_DIR}")
endif()

# game data copier targets

function(addDataCopierTarget TARGET_NAME TEXTRA DST OPT_SHD OPT_IMG SINGLE_FILES)
	add_custom_target(${TARGET_NAME} ${TEXTRA} SOURCES ${DATA_SRC}
						COMMAND "${CMAKE_COMMAND}" -E make_directory "${DST}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${SINGLE_FILES} "${DST}"
						COMMAND "${EXE_DIR}/${OVEN_NAME}" -a "${DST}/audio.dat" ${ASSET_WAV}
						COMMAND "${EXE_DIR}/${OVEN_NAME}" -m "${DST}/materials.dat" ${ASSET_MTL}
						COMMAND "${EXE_DIR}/${OVEN_NAME}" -o "${DST}/objects.dat" ${ASSET_OBJ}
						COMMAND "${EXE_DIR}/${OVEN_NAME}" ${OPT_SHD} "${DST}/shaders.dat" ${ASSET_SHD}
						COMMAND "${EXE_DIR}/${OVEN_NAME}" ${OPT_IMG} "${DST}/textures.dat" ${ASSET_IMG}
						WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	add_dependencies(${TARGET_NAME} ${OVEN_NAME})
endfunction()

if(OPENGLES)
	addDataCopierTarget(${DATA_NAME} "ALL" "${DATA_DIR}" "-S" "-T" "${ASSET_DAT}")
else()
	addDataCopierTarget(${DATA_NAME} "ALL" "${DATA_DIR}" "-s" "-t" "${ASSET_DAT}")
endif()
addDataCopierTarget(${ADATA_NAME} "" "${DIR_PAND}/app/src/main/assets" "-S" "-T" "rsc/data/romanesque.ttf;rsc/data/merriweather.otf")

# server program target

add_executable(${SERVER_NAME} ${SERVER_SRC})
target_compile_features(${SERVER_NAME} PUBLIC cxx_std_17)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_libraries(${SERVER_NAME} "${LIB_SDL}" "${LIB_NET}")
else()
	target_link_libraries(${SERVER_NAME} SDL2 SDL2_net)
endif()
setCommonTargetProperties(${SERVER_NAME} "${EXE_DIR}")

# asset building program target

add_executable(${OVEN_NAME} ${OVEN_SRC})
target_compile_features(${OVEN_NAME} PUBLIC cxx_std_17)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_libraries(${OVEN_NAME} "${LIB_SDL}" "${LIB_IMG}")
else()
	target_link_libraries(${OVEN_NAME} SDL2 SDL2_image)
endif()
setCommonTargetProperties(${OVEN_NAME} "${EXE_DIR}")

add_custom_command(TARGET ${OVEN_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E make_directory "${LICN_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/data/LICENSE.Merriweather.txt" "${LICN_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_LIB}/glm/copying.txt" "${LICN_DIR}/COPYING.glm.txt")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_custom_command(TARGET ${OVEN_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_SDL}/SDL2.dll" "${BIN_IMG}/SDL2_image.dll" "${BIN_NET}/SDL2_net.dll" "${BIN_TTF}/SDL2_ttf.dll" "${BIN_IMG}/libjpeg-9.dll" "${BIN_IMG}/libpng16-16.dll" "${BIN_TTF}/libfreetype-6.dll" "${BIN_TTF}/zlib1.dll" "${EXE_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_SDL}/COPYING.txt" "${LICN_DIR}/COPYING.SDL.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_IMG}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_image.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_NET}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_net.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_TTF}/COPYING.txt" "${LICN_DIR}/COPYING.SDL_ttf.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_GLEW}/LICENSE.txt" "${LICN_DIR}/LICENSE.glew.txt"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_IMG}/LICENSE.png.txt" "${LICN_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_TTF}/LICENSE.freetype.txt" "${LICN_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BIN_TTF}/LICENSE.zlib.txt" "${LICN_DIR}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_custom_command(TARGET ${OVEN_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E make_directory "${FRAM_DIR}/${FRAM_SDL}" "${FRAM_DIR}/${FRAM_NET}" "${FRAM_DIR}/${FRAM_TTF}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_SDL}" "${FRAM_DIR}/${FRAM_SDL}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_IMG}" "${FRAM_DIR}/${FRAM_IMG}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_NET}" "${FRAM_DIR}/${FRAM_NET}"
						COMMAND "${CMAKE_COMMAND}" -E copy_directory "${DIR_LIB}/${FRAM_TTF}" "${FRAM_DIR}/${FRAM_TTF}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/Info.plist" "${APP_DIR}"
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/thrones.icns" "${DATA_DIR}")
else()
	add_custom_command(TARGET ${OVEN_NAME} POST_BUILD
						COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIR_RSC}/thrones.desktop" "${EXE_DIR}")
endif()

# prettyfiers

set(ALL_SRC ${THRONES_SRC} ${DATA_SRC} ${SERVER_SRC} ${OVEN_SRC})
foreach(FSRC IN LISTS ALL_SRC)
	get_filename_component(FGRP "${FSRC}" DIRECTORY)
	string(REPLACE "/" ";" FGRP "${FGRP}")
	list(REMOVE_AT FGRP 0)
	if(MSVC)
		string(REPLACE ";" "\\" FGRP "${FGRP}")
	endif()
	source_group("${FGRP}" FILES "${FSRC}")
endforeach()
