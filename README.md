# Thrones  
A game under construction.  

Used libraries are SDL2, SDL2_net, SDL2_ttf, GLEW, GLM and by extension FreeType and zlib. The included font is Merriweather.  
The CMakeLists.txt is written for at least CMake 3.10.2 with Clang, GCC or MSVC which need to support C++17.  
You can create a Makefile for a debug build by running CMake with the "-DCMAKE_BUILD_TYPE=Debug" option. Otherwise it'll default to a release build.  

## Running the game  
Either run the "Thrones_server" executable, which will handle communication between players or run the "Thrones" executable, click "Host" and "Open" to start a server.  
Run either two instances or a second instance of the "Thrones" executable, which will be the player client. Enter the address and port of the computer on which the server instance is running, or leave it default if testing on the same computer, and press "Connect".  

The game starts with the setup, which consists of the following phases in order:  
- placing your home tiles  
- placing the middle row tiles  
- placing your pieces  

Press "Next" or "Prev" to switch between phases.  
You can place tiles/pieces by dragging them from the icon bar or using the mouse wheel to select one and holding the left mouse button and dragging across the board, switch their positions by dragging them using the left mouse button or clear them by holding the right mouse button and dragging. Fortress tiles will be places automatically, where no tiles were placed.  
After the setup the server program determines the first player, the middle row tiles will be rearranged accordingly and the actual match starts.  
You can move pieces by dragging them using the left mouse button and fire by dragging a piece using the right mouse button.  
To use a move/attack/fire fate's favor, hold down the left alt key while dragging the piece. To negate an attack, use the "Negate" button.  

Tile colors:  
plains : light green  
forest : dark green  
mountain : gray  
water : blue  
fortress : brown  

Game client command line options:  
-s [address] : set server address (leave empty to reset)  
-p [port] : set port number (leave empty to reset)  
-c : connect to server immediately  
-d : place objects beforehand (only for debug build)  

Game server command line options:  
-p [port] : port to use (default is 39741)  
-c [name] : game config to load (default is "default")  
-f [file] : game config file to load (default is "game.ini")  

## Game Configuration  
It's possible to change the following game rules:  
- homeland width: 3 - 71 (same as board width)  
- homeland height: 3 - 35 (board height: 9 - 71)  
- survival check pass probability: 0% - 100%  
- fate's favor limit: 0 - 255 (per player)  
- dragon move step limit: 0 - 255  
- dragon step diagonal: whether a dragon can make diagonal steps  
- max total piece amount: 189 (per player)  
- tile/piece amounts are adjusted to fit the board size  
- amount of simultaneous fortress captures required to win  
- amount of thrones required to win  
- capturers: pieces which can capture fortresses  
- shift left: whether overlapping middle tiles are shifted to the left  
- shift near: whether overlapping middle tiles are shifted to the closest or farthest empty tile  

Different configurations are stored with unique names in the "game.ini" file in the game's root directory.  
Other client settings are stored in the "settings.ini" file in the game client's root directory.   

## To do  
- add move indicator  
- alternate game modes  
