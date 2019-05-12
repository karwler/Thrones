# project properties
cmake_minimum_required(VERSION 3.10.2)
project(Thrones LANGUAGES CXX)
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")
endif()

# source files
set(SRC_FILES
	"src/engine/fileSys.cpp"
	"src/engine/fileSys.h"
	"src/engine/scene.cpp"
	"src/engine/scene.h"
	"src/engine/windowSys.cpp"
	"src/engine/windowSys.h"
	"src/engine/world.cpp"
	"src/engine/world.h"
	"src/prog/game.cpp"
	"src/prog/game.h"
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
	add_definitions(-D_UNICODE -D_CRT_SECURE_NO_WARNINGS)
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
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wno-padded")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-array-bounds -Wno-deprecated -Wno-float-equal -Wno-parentheses -Wno-shadow -Wno-switch -Wno-switch-enum -Wno-unused-result")
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
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
						COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/rsc/debug_run.bat" ${EXECUTABLE_OUTPUT_PATH})
else()
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
						COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/rsc/debug_run.sh" ${EXECUTABLE_OUTPUT_PATH})
endif()
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#					COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/rsc/icon.ico" ${EXECUTABLE_OUTPUT_PATH}
					COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/rsc/data" ${EXECUTABLE_OUTPUT_PATH})

# filter file for ms visual studio
foreach(source IN LISTS SRC_FILES)
	get_filename_component(source_dir ${source} PATH)
	string(REPLACE "/" ";" dirs "${source_dir}")
	list(GET dirs 0 dir0)
	
	if(${dir0} MATCHES "rsc")
		set(filter_path "Source")
	else()
		list(REMOVE_AT dirs 0)
		list(INSERT dirs 0 "Source")
		string(REPLACE ";" "\\" filter_path "${dirs}")
	endif()
	source_group("${filter_path}" FILES ${source})
endforeach()

# set server program target
set(SERVER_SRC
	"src/server/server.cpp"
	"src/server/server.h"
	"src/server/serverProg.cpp")

string(CONCAT SERVER_NAME ${PROJECT_NAME} "_Server")
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

foreach(source IN LISTS SERVER_SRC)
	get_filename_component(source_dir ${source} PATH)
	string(REPLACE "/" ";" dirs "${source_dir}")
	
	list(REMOVE_AT dirs 0)
	list(INSERT dirs 0 "Source")
	string(REPLACE ";" "\\" filter_path "${dirs}")
	source_group("${filter_path}" FILES ${source})
endforeach()
