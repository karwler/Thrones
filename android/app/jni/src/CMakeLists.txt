cmake_minimum_required(VERSION 3.6)
project(Thrones)

find_library(SDL2 SDL2)
add_library(main SHARED)
target_sources(main PRIVATE
	"engine/audioSys.cpp"
	"engine/fileSys.cpp"
	"engine/inputSys.cpp"
	"engine/scene.cpp"
	"engine/windowSys.cpp"
	"engine/world.cpp"
	"oven/oven.cpp"
	"prog/board.cpp"
	"prog/game.cpp"
	"prog/guiGen.cpp"
	"prog/netcp.cpp"
	"prog/program.cpp"
	"prog/progs.cpp"
	"prog/types.cpp"
	"server/server.cpp"
	"utils/context.cpp"
	"utils/layouts.cpp"
	"utils/objects.cpp"
	"utils/settings.cpp"
	"utils/text.cpp"
	"utils/utils.cpp"
	"utils/widgets.cpp")
target_link_libraries(main SDL2 SDL2_image SDL2_ttf)
