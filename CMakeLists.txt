# project properties
cmake_minimum_required(VERSION 3.10.2)
project(Thrones LANGUAGES CXX)
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")
endif()

# additional stuff
set(DATA_NAME "game_data")
string(CONCAT SERVER_NAME ${PROJECT_NAME} "_Server")
set(OVEN_NAME "toven")

# source files
set(SRC_FILES
	"rsc/shaders/gui.frag"
	"rsc/shaders/gui.vert"
	"rsc/shaders/scene.frag"
	"rsc/shaders/scene.vert"
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
	"src/utils/cvec2.h"
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

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	list(APPEND SRC_FILES "rsc/resource.rc")
endif()

# library/framework directories
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	if(MSVC)
		if(${CMAKE_SIZEOF_VOID_P} MATCHES "8")
			set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib/msvc64")
		else()
			set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib/msvc32")
		endif()
	endif()
	
	link_directories(${LIB_DIR})
	set(DLL_FILES
		"libfreetype-6.dll"
		"SDL2.dll"
		"SDL2_net.dll"
		"SDL2_ttf.dll"
		"zlib1.dll")	
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	find_library(LIB_SDL2 SDL2)
	find_library(LIB_SDL2_NET SDL2_net)
	find_library(LIB_SDL2_TTF SDL2_ttf)
	find_library(LIB_OPENGL OpenGL)
endif()

# definitions
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	add_definitions(-D_UNICODE -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	add_definitions(-DDEBUG -D_DEBUG)
else()
	add_definitions(-DNDEBUG)
endif()

# set main target
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
add_dependencies(${PROJECT_NAME} ${DATA_NAME})

# include directories and link libraries
include_directories("${CMAKE_SOURCE_DIR}/src")
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	include_directories("${CMAKE_SOURCE_DIR}/lib/includeGL" "${CMAKE_SOURCE_DIR}/lib/includeSDL")
	target_link_libraries(${PROJECT_NAME} SDL2 SDL2_net SDL2_ttf glew32s glu32 opengl32)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	include_directories("${CMAKE_SOURCE_DIR}/lib/includeGL")
	target_link_libraries(${PROJECT_NAME} ${LIB_SDL2} ${LIB_SDL2_NET} ${LIB_SDL2_TTF} ${LIB_OPENGL})
else()
	target_link_libraries(${PROJECT_NAME} SDL2 SDL2_net SDL2_ttf GLEW GLU GL)
endif()

# target properties
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set_target_properties(${PROJECT_NAME} PROPERTIES
						RUNTIME_OUTPUT_DIRECTORY_DEBUG ${EXECUTABLE_OUTPUT_PATH}
						RUNTIME_OUTPUT_DIRECTORY_RELEASE ${EXECUTABLE_OUTPUT_PATH})

if((${CMAKE_CXX_COMPILER_ID} MATCHES "GNU") OR (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang"))
	if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -no-pie")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-covered-switch-default -Wno-exit-time-destructors -Wno-global-constructors")
	endif()

	if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
	else()
		if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
		endif()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wno-padded")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-implicit-fallthrough -Wno-float-equal -Wno-parentheses -Wno-shadow -Wno-switch -Wno-switch-enum -Wno-unused-result")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set_target_properties(${PROJECT_NAME} PROPERTIES
							OUTPUT_NAME ${PROJECT_NAME}
							LINK_FLAGS "/SUBSYSTEM:WINDOWS")
else()
	string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
	set_target_properties(${PROJECT_NAME} PROPERTIES
							OUTPUT_NAME ${PROJECT_NAME_LOWER})
endif()

# post buils commands
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	foreach(dll_file ${DLL_FILES})
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
							COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_DIR}/${dll_file}" ${EXECUTABLE_OUTPUT_PATH})
	endforeach()
endif()

# game data copier target
file(GLOB ASSET_WAV LIST_DIRECTORIES false "${CMAKE_SOURCE_DIR}/rsc/audio/*.wav")
file(GLOB ASSET_DAT LIST_DIRECTORIES false "${CMAKE_SOURCE_DIR}/rsc/data/*")
file(GLOB ASSET_MTL LIST_DIRECTORIES false "${CMAKE_SOURCE_DIR}/rsc/materials/*.mtl")
file(GLOB ASSET_OBJ LIST_DIRECTORIES false "${CMAKE_SOURCE_DIR}/rsc/objects/*.obj")
file(GLOB ASSET_SHD LIST_DIRECTORIES false "${CMAKE_SOURCE_DIR}/rsc/shaders/*.frag" "${CMAKE_SOURCE_DIR}/rsc/shaders/*.vert")
file(GLOB ASSET_BMP LIST_DIRECTORIES false "${CMAKE_SOURCE_DIR}/rsc/textures/*.bmp")
set(DATA_SRC ${ASSET_WAV} ${ASSET_DAT} ${ASSET_MTL} ${ASSET_OBJ} ${ASSET_SHD} ${ASSET_BMP})
add_custom_target(${DATA_NAME} ALL
					SOURCES ${DATA_SRC}
					COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/rsc/data" "${EXECUTABLE_OUTPUT_PATH}/data"
					COMMAND ${EXECUTABLE_OUTPUT_PATH}/${OVEN_NAME} -a "${EXECUTABLE_OUTPUT_PATH}/data/audio.dat" ${ASSET_WAV}
					COMMAND ${EXECUTABLE_OUTPUT_PATH}/${OVEN_NAME} -m "${EXECUTABLE_OUTPUT_PATH}/data/materials.dat" ${ASSET_MTL}
					COMMAND ${EXECUTABLE_OUTPUT_PATH}/${OVEN_NAME} -o "${EXECUTABLE_OUTPUT_PATH}/data/objects.dat" ${ASSET_OBJ}
					COMMAND ${EXECUTABLE_OUTPUT_PATH}/${OVEN_NAME} -s "${EXECUTABLE_OUTPUT_PATH}/data/shaders.dat" ${ASSET_SHD}
					COMMAND ${EXECUTABLE_OUTPUT_PATH}/${OVEN_NAME} -t "${EXECUTABLE_OUTPUT_PATH}/data/textures.dat" ${ASSET_BMP})
add_dependencies(${DATA_NAME} ${OVEN_NAME})

# set server program target
set(SERVER_SRC
	"src/server/server.cpp"
	"src/server/server.h"
	"src/server/serverProg.cpp"
	"src/utils/text.cpp"
	"src/utils/text.h")
add_executable(${SERVER_NAME} ${SERVER_SRC})
target_compile_features(${SERVER_NAME} PUBLIC cxx_std_17)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(${SERVER_NAME} ${LIB_SDL2} ${LIB_SDL2_NET})
else()
	target_link_libraries(${SERVER_NAME} SDL2 SDL2_net)
endif()

set_target_properties(${SERVER_NAME} PROPERTIES
						RUNTIME_OUTPUT_DIRECTORY_DEBUG ${EXECUTABLE_OUTPUT_PATH}
						RUNTIME_OUTPUT_DIRECTORY_RELEASE ${EXECUTABLE_OUTPUT_PATH})
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set_target_properties(${SERVER_NAME} PROPERTIES
							OUTPUT_NAME ${SERVER_NAME}
							LINK_FLAGS "/SUBSYSTEM:CONSOLE")
else()
	string(TOLOWER ${SERVER_NAME} SERVER_NAME_LOWER)
	set_target_properties(${SERVER_NAME} PROPERTIES
							OUTPUT_NAME ${SERVER_NAME_LOWER})
endif()

# set asset building program target
set(OVEN_SRC
	"src/oven/oven.cpp"
	"src/oven/oven.h"
	"src/oven/ovenProg.cpp"
	"src/utils/text.cpp"
	"src/utils/text.h")
add_executable(${OVEN_NAME} ${OVEN_SRC})
target_compile_features(${OVEN_NAME} PUBLIC cxx_std_17)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(${OVEN_NAME} ${LIB_SDL2})
else()
	target_link_libraries(${OVEN_NAME} SDL2)
endif()

set_target_properties(${OVEN_NAME} PROPERTIES
						RUNTIME_OUTPUT_DIRECTORY_DEBUG ${EXECUTABLE_OUTPUT_PATH}
						RUNTIME_OUTPUT_DIRECTORY_RELEASE ${EXECUTABLE_OUTPUT_PATH})
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set_target_properties(${OVEN_NAME} PROPERTIES
							OUTPUT_NAME ${OVEN_NAME}
							LINK_FLAGS "/SUBSYSTEM:CONSOLE")
else()
	set_target_properties(${OVEN_NAME} PROPERTIES
							OUTPUT_NAME ${OVEN_NAME})
endif()

# I don't know how to make these work in a function
if(MSVC)
	foreach(source IN LISTS SRC_FILES)
		get_filename_component(dirs ${source} PATH)
		string(REPLACE "/" ";" dirs "${dirs}")
		list(REMOVE_AT dirs 0)
		list(INSERT dirs 0 "Source")
		string(REPLACE ";" "\\" dirs "${dirs}")
		source_group("${dirs}" FILES ${source})
	endforeach()

	foreach(source IN LISTS DATA_SRC)
		get_filename_component(dirs ${source} PATH)
		string(REPLACE "${CMAKE_SOURCE_DIR}/rsc" "" dirs "${dirs}")
		string(REPLACE "/" ";" dirs "${dirs}")
		list(INSERT dirs 0 "Source")
		string(REPLACE ";" "\\" dirs "${dirs}")
		source_group("${dirs}" FILES ${source})
	endforeach()

	foreach(source IN LISTS SERVER_SRC)
		get_filename_component(dirs ${source} PATH)
		string(REPLACE "/" ";" dirs "${dirs}")
		list(REMOVE_AT dirs 0)
		list(INSERT dirs 0 "Source")
		string(REPLACE ";" "\\" dirs "${dirs}")
		source_group("${dirs}" FILES ${source})
	endforeach()

	foreach(source IN LISTS OVEN_SRC)
		get_filename_component(dirs ${source} PATH)
		string(REPLACE "/" ";" dirs "${dirs}")
		list(REMOVE_AT dirs 0)
		list(INSERT dirs 0 "Source")
		string(REPLACE ";" "\\" dirs "${dirs}")
		source_group("${dirs}" FILES ${source})
	endforeach()
endif()
