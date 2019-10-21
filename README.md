# Thrones  
**Based on the board game by Chad Anthony Randell:**  
"Thrones is a game designed to cultivate cunning and strategic maneuvering on a battlefield created by you, your opponent, and the battle between you.  
The struggle for supremacy will take many forms as your infantry melee, cavalry charge, and artillery fire.  
You may strike with your Throne’s royal guard or unleash a Dragon on any challenger to your claim. Not even fate itself is out of your influence and reach.  
Your enemies, however, are diverse, and their lands will be treacherous to a conqueror. Destruction of their armies is just one path to victory;  
if you succeed in claiming their Fortress as your own, a pretender’s family will surely plea for peace, which you can consider as their army disbands in defeat."  

See [RULES](RULES.md) for details on gameplay.  

## Build  
Used libraries are GLEW, GLM, SDL2, SDL2_net, SDL2_ttf and by extension FreeType and zlib. The included font is Romanesque Serif Regular.  
The CMakeLists.txt is written for CMake 3.10.2 with Clang, GCC or MSVC which need to support C++17.  

CMake variables:  
- CMAKE_BUILD_TYPE : string = Release  
  - can be set to "Debug"  
- EXTERNAL : bool = 0  
  - store preferences externally by default  
- LIBDROID : bool = 0  
  - download libraries for Android Studio  
- OPENGLES : bool = 0  
  - use OpenGL ES  
- VER_SDL : string = 2.0.10  
  - SDL version to download  
- VER_NET : string = 2.0.1  
  - SDL_net version to download  
- VER_TTF : string = 2.0.15  
  - SDL_ttf version to download  
- VER_GLEW : string = 2.1.0  
  - GLEW version to download  
- VER_GLM : string = 0.9.9.6  
  - GLM version to download  

### Android  
The "android" directory can be imported in Android Studio as a project, which builds only the game client.  
To install the necessary additional files, run CMake with the "-DLIBDROID=1" option.  
Next the CMake target "assets_android" needs to be built separately, which will create the assets in "android/app/src/main/assets". The requirements for this are as listed for the other systems.  
If you're on Windows, make sure that your Git supports symbolic links or check that "android/app/jni/src" contains the files of "src".  

### Linux  
The only supported compilers are Clang and GCC. All dependencies need to be installed manually.  
Installing the development packages for the listed libraries should be enough, assuming that all necessary dependencies are installed automatically.  
To create a menu entry for the game client, you can use the "rsc/thrones.desktop" launcher file.  
When building the game client on a Raspberry Pi, the CMake option "-DOPENGLES=1" should be set.  

### macOS  
All necessary dependencies are downloaded when running CMake.  

### Windows  
The only supported compilers are MSVC and MinGW. All necessary libraries are downloaded when running CMake.  
When building the game client in Visual Studio for the first time, "assets" will fail to build and needs to be built separately afterwards to succeed.  


## Programs  
Either run the "Server" executable, which will handle communication between players or run the "Thrones" executable, click "Host" and "Open" to start a server.  
Run either two instances or a second instance of the "Thrones" executable, which will be the player client.  
Enter the address and port of the computer on which the server instance is running, or leave it default if testing on the same computer, and press "Connect".  

The game program requires OpenGL 3.0 or OpenGL ES 3.0.  
Game configurations are stored with unique names in "game.ini", tile/piece setups in "setup.ini" and other client settings in "settings.ini".  
On Android these files are in the "/storage/sdcard0/Android/data/com.carnkaw.thrones/files" directory, on Linux in "$HOME/.config/thrones", on macOS in "$HOME/Library/Preferences/Thrones" and on Windows in "%AppData%/Thrones".  
The files can also be stored in "data" when running in portable mode on Linux or Windows or within the application bundle on macOS.  

Game client command line options:  
- -e [0|1]  
  - store settings externally (default is 1 when compiled with -DEXTERNAL)  
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
You can place tiles/pieces by dragging them from the icon bar or by using the mouse wheel/number keys to select one and holding the left mouse button and dragging across the board.  
Tiles/pieces can be switched by dragging them using the left mouse button or cleared by holding the right mouse button and dragging or using the "Delete" button.  
Fortress tiles will be places automatically, where no tiles were placed.  
After the setup the server program determines the first player, the middle row tiles will be rearranged accordingly and the actual match starts.  

You can move pieces by dragging them using the left mouse button and fire by dragging a piece using the right mouse button.  
Because by default the warhorse switches with any piece, drag it with the right mouse button to explicitly attack.  
To use a move/attack/fire fate's favor, hold down the left alt key while dragging the piece.  
The camera can be rotated by holding down the middle mouse button and reset by pressing C.  
