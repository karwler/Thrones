# Thrones  
A game under construction.  

Used libraries are SDL2, SDL2_net, SDL2_ttf, GLEW, GLM and by extension FreeType and zlib. The included font is Merriweather.  
The CMakeLists.txt is written for at least CMake 3.10.2 with Clang, GCC or MSVC which need to support C++17.  
You can create a Makefile for a debug build by running CMake with the "-DCMAKE_BUILD_TYPE=Debug" option. Otherwise it'll default to a release build.  

## Running the game  
Run the "Thrones_server" executable, which will handle communication between players. To use a custom port, run the executable with the port number as the first command line argument.  
Run two instances of the "Thrones" executable, one for each player. Enter the address and port of the computer on which "Thrones_server" is running, or leave it default if testing on the same computer, and press "Connect".  

The game starts with the setup, which consists of the following phases in order:  
- placing your home tiles  
- placing the middle row tiles  
- placing your pieces  

Press "Next" or "Prev" to switch between phases.  
You can place tiles/pieces by dragging them from the icon bar or using the mouse wheel to select one and left clicking on the board, switch their positions by dragging them using the left mouse button or clear them by right clicking on them. Fortress tiles will be places automatically, where no tiles were placed.  
After the setup the server program determines the first player, the middle row tiles will be rearranged accordingly and the actual match starts.  
You can move pieces by dragging them using the left mouse button and fire by dragging a piece using the right mouse button.  

Tile colors:  
plains : light green  
forest : dark green  
mountain : gray  
water : blue  
fortress : brown  

The game client can be run with the following command line arguments:  
-s [address] : set server address (leave empty to reset)  
-p [port] : set port number (leave empty to reset)  
-c : connect to server immediately  
-d : place objects beforehand (only for debug build)  

## To do  
- implement fate's favor  
- add move indicator  
- add server UI  
- add server game settings  
- test new packets  
- test firing on fortress  
- test survival checks  
- test screen modes  
- test gradients  
- test obj loading  
- alternate game modes  
