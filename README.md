# Thrones  
Based on the board game by Chad Anthony Randell.  
See [RULES](doc/rules.html) or [DOCS](doc/docs.html) for details.  

## Build  
Used libraries are GLEW, GLM, SDL2, SDL2_image, SDL2_net, SDL2_ttf and by extension FreeType, libjpeg, libpng and zlib. The included fonts are Romanesque Serif Regular and Merriweather Regular.  
The CMakeLists.txt is written for CMake 3.10.2 with Clang, GCC or MSVC which need to support C++17.  

CMake variables:  
- ARCH : string = system default  
  - can be set to 32 or 64 for Clang or GCC  
- CMAKE_BUILD_TYPE : string = Release  
  - can be set to "Debug"  
- EXTERNAL : bool = 0  
  - store preferences externally by default  
- LIBDROID : bool = 0  
  - download libraries for Android Studio  
- OPENGLES : bool = 0  
  - use OpenGL ES  
- VER_GLEW : string = 2.1.0  
  - GLEW version to download  
- VER_GLM : string = 0.9.9.7  
  - GLM version to download  
- VER_SDL : string = 2.0.10  
  - SDL version to download  
- VER_IMG : string = 2.0.5  
  - SDL_image version to download  
- VER_NET : string = 2.0.1  
  - SDL_net version to download  
- VER_TTF : string = 2.0.15  
  - SDL_ttf version to download  

### Android  
The "android" directory can be imported in Android Studio as a project, which builds only the game client.  
To install the necessary additional files, run CMake with the "-DLIBDROID=1" option.  
Next the CMake target "assets_android" needs to be built separately, which will create the assets in "android/app/src/main/assets". The requirements for this are as listed for the other systems.  
If you're on Windows, make sure that your Git supports symbolic links or check that "android/app/jni/src" contains the files of "src".  

### Emscripten  
A makefile with a target for the game client can be created with the CMake file, which requires emsdk to be installed. Additional libraries will be downloaded.  
Before building the program, the assets for OpenGL ES need to be built, using the method below and the created "data" directory has to be copied to the directory of the makefile for the Emscripten build.  

### Linux  
The only supported compilers are Clang and GCC. All dependencies need to be installed manually.  
Installing the development packages for the listed libraries should be enough, assuming that all necessary dependencies are installed automatically.  
To create a menu entry for the game client, you can use the "rsc/thrones.desktop" launcher file.  
When building the game client on a Raspberry Pi, the CMake option "-DOPENGLES=1" should be set.  

### macOS  
All necessary dependencies are downloaded when running CMake.  

### Windows  
The only supported compilers are MSVC and MinGW. All necessary libraries are downloaded when running CMake.  


## TODO  
- shadows aren't working properly on some systems  
- check if macOS builds  
