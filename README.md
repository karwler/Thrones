# Thrones  
Designed by Chad Anthony.  

Thrones is a game designed to cultivate cunning and strategic maneuvering on a battlefield created by you, your opponent, and the battle between you. The struggle for supremacy will take many forms as your infantry melee, cavalry charge, and artillery fire. You may strike with your Throne’s royal guard or unleash a Dragon on any challenger to your claim. Not even fate itself is out of your influence and reach.  
Your enemies, however, are diverse, and their lands will be treacherous to a conqueror. Destruction of their armies is just one path to victory; if you succeed in claiming their Fortress as your own, a pretender’s family will surely plea for peace, which you can consider as their army disbands in defeat.  

See [RULES](RULES.md) for detail on gameplay.  

Used libraries are SDL 2.0.10, SDL_net 2.0.1, SDL_ttf 2.0.15, GLEW 2.1.0, GLM 0.9.9 and by extension FreeType and zlib. The included font is Romanesque Serif Regular.  
The CMakeLists.txt is written for CMake 3.10.2 with Clang, GCC or MSVC which need to support C++17.  
You can create a Makefile for a debug build by running CMake with the "-DCMAKE_BUILD_TYPE=Debug" option. Otherwise it'll default to a release build.  

## Running the game  
Either run the "Thrones_server" executable, which will handle communication between players or run the "Thrones" executable, click "Host" and "Open" to start a server.  
Run either two instances or a second instance of the "Thrones" executable, which will be the player client. Enter the address and port of the computer on which the server instance is running, or leave it default if testing on the same computer, and press "Connect".  
Different game configurations are stored with unique names in the "game.ini" file in the game's root directory. Tile/piece setups are stored in "setup.ini" and other client settings are stored in the "settings.ini" file in the game client's root directory.   

Game client command line options:  
- -d  
  - place objects beforehand (only for debug build)  

Game server command line options:  
- -p [port]  
  - port to use (default is 39741)  
- v  
  - write output to console  
- l  
  - write output to log file  

Game server keys:  
- L: list rooms  
- P: list players  
- Q: close program  

## Gameplay  
The game starts with the setup, which consists of the following phases:  
1. placing your home tiles  
2. placing the middle row tiles  
3. placing your pieces  

Press "Next" or "Prev" to switch between phases.  
You can place tiles/pieces by dragging them from the icon bar or by using the mouse wheel/number keys to select one and holding the left mouse button and dragging across the board, switch their positions by dragging them using the left mouse button or clear them by holding the right mouse button and dragging. Fortress tiles will be places automatically, where no tiles were placed.  
After the setup the server program determines the first player, the middle row tiles will be rearranged accordingly and the actual match starts.  

You can move pieces by dragging them using the left mouse button and fire by dragging a piece using the right mouse button. Because by default the warhorse switches with any piece, drag it with the right mouse button to explicitly attack.  
To use a move/attack/fire fate's favor, hold down the left alt key while dragging the piece.  
The camera can be rotated by holding down the middle mouse button and reset by pressing C.  
