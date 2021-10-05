# Thrones  
Based on the board game by Chad Anthony Randell.  
See [RULES](doc/rules.html) or [DOCS](doc/docs.html) for details.  

## Build  
Used libraries are assimp, libcurl, GLEW, GLM, SDL2, SDL2_image, SDL2_ttf and by extension FreeType, libpng and zlib. The included fonts are Romanesque Serif Regular and Merriweather Regular.  
The CMakeLists.txt is written for CMake 3.10.2 with Clang, GCC or MSVC which need to support C++17.  

CMake options:  
- APPIMAGE : bool = 0  
  - package the client as an AppImage  
- ARCH : string  
  - CPU type string to pass to Clang's or GCC's "-march" option  
- CMAKE_BUILD_TYPE : string = Release  
  - can be set to "Debug"  
- EMBASE : string = ${CMAKE_BINARY_DIR}  
  - location where the target assets_emscripten should put the asset directory  
- EXTERNAL : bool = 1  
  - store preferences externally by default  
- LIBDROID : bool = 0  
  - download libraries for Android Studio  
- OPENGLES : bool = 0  
  - use OpenGL ES  
- SERVICE : bool = 0  
  - server program won't check keyboard input  
- SYSLIBS : bool = Linux: 1, macOS: 0  
  - use the installed assimp and curl libraries instead of downloading them  
- VER_ASSIMP : string = 5.0.1  
  - assimp version to download  
- VER_CURL : string = 7.80.0  
  - libcurl version to download  
- VER_GLEW : string = 2.2.0  
  - GLEW version to download  
- VER_GLM : string = 0.9.9.8  
  - GLM version to download  
- VER_SDL : string = 2.0.18  
  - SDL version to download  
- VER_IMG : string = 2.0.5  
  - SDL_image version to download  
- VER_TTF : string = 2.0.15  
  - SDL_ttf version to download  

### Android  
The "android" directory can be imported in Android Studio as a project, which builds only the game client.  
To install the necessary additional files, run CMake with the "-DLIBDROID=1" option.  
Next the CMake target "assets_android" needs to be built separately, which will create the assets in "android/app/src/main/assets". The requirements for this are as listed for the other systems.  
It might be necessary to set the NDK locations, which can be done in Android Studio under "File -> Project Structure -> SDK Location".  
If that isn't possible, try setting the line "ndk.dir=<path>" in the local.properties file.  
If you're on Windows, make sure that your Git supports symbolic links or check that "android/app/jni/src" contains the files of "src".  

### Emscripten  
A makefile with a target for the game client can be created with the CMake file using ```emcmake cmake```. Additional libraries will be downloaded.  
Before building the program, the target "assets_emscripten" needs to be built separately, which will create the "share" directory containing the assets. This directory will either be moved automatically by setting the CMake option "EMBASE" or will have to be copied manually to the directory of the makefile for the Emscripten build.  
The game client can then be built with ```emmake make```.  

### Linux  
The only supported compilers are Clang and GCC.
GLEW and GLM are downloaded and built while running CMake. SDL2, SDL2_image, SDL2_ttf and libcurl are only downloaded and built when building an AppImage.  
To create a menu entry for the game client, you can use the "rsc/thrones.desktop" launcher file.  
When building the game client on a Raspberry Pi, the CMake option "-DOPENGLES=1" should be set.  

### macOS  
All necessary dependencies are downloaded when running CMake.  

### Windows  
The only supported compilers are MSVC and MinGW.  
All necessary libraries are downloaded and built with NMake or mingw32-make while running CMake. Because of that you might need to run CMake through a developer console or have NMake or mingw32-make set up in the Path.  

## TODO  
- test and fix everything  
