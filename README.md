# Thrones  
Based on the board game by Chad Anthony Randell.  
See [RULES](doc/rules.html) or [DOCS](doc/docs.html) for details.  

## Build  
Used libraries are libcurl, GLEW, GLM, SDL2, SDL2_image, SDL2_ttf and by extension FreeType, libjpeg, libpng and zlib. The included fonts are Romanesque Serif Regular and Merriweather Regular.  
The CMakeLists.txt is written for CMake 3.10.2 with Clang, GCC or MSVC which need to support C++17.  

CMake variables:  
- CMAKE_BUILD_TYPE : string = Release  
  - can be set to "Debug"  
- EXTERNAL : bool = 0  
  - store preferences externally by default  
- LIBDROID : bool = 0  
  - download libraries for Android Studio  
- NATIVE : bool = 0
  - build for the current CPU (only available for Clang and GCC)  
- OPENGLES : bool = 0  
  - use OpenGL ES  
- SERVICE : bool = 0  
  - server program won't check keyboard input  
- WEBUTILS : bool = 1
  - build the client with curl (only available for Linux and Windows)  
- VER_CURL : string = 7.72.0  
  - libcurl version to download  
- VER_GLEW : string = 2.1.0  
  - GLEW version to download  
- VER_GLM : string = 0.9.9.8  
  - GLM version to download  
- VER_SDL : string = 2.0.12  
  - SDL version to download  
- VER_IMG : string = 2.0.5  
  - SDL_image version to download  
- VER_TTF : string = 2.0.15  
  - SDL_ttf version to download  

### Android  
The "android" directory can be imported in Android Studio as a project, which builds only the game client.  
To install the necessary additional files, run CMake with the "-DLIBDROID=1" option.  
Next the CMake target "assets_android" needs to be built separately, which will create the assets in "android/app/src/main/assets". The requirements for this are as listed for the other systems.  
It might be necessary to set the NDK location, which can be done in Android Studio under "File -> Project Structure -> SDK Location -> Android NDK Location".  
If you're on Windows, make sure that your Git supports symbolic links or check that "android/app/jni/src" contains the files of "src".  

### Emscripten  
A makefile with a target for the game client can be created with the CMake file, which requires emsdk to be installed. Additional libraries will be downloaded.  
Before building the program, the assets for OpenGL ES need to be built, using the method below and the created "data" directory has to be copied to the directory of the makefile for the Emscripten build.  

### Linux  
The only supported compilers are Clang and GCC.
GLEW and GLM are downloaded and built while running CMake. SDL2, SDL2_image, SDL2_ttf and libcurl need to be installed manually.  
To create a menu entry for the game client, you can use the "rsc/thrones.desktop" launcher file.  
When building the game client on a Raspberry Pi, the CMake option "-DOPENGLES=1" should be set.  

### macOS  
All necessary dependencies are downloaded when running CMake.  

### Windows  
All necessary libraries are downloaded and built with NMake while running CMake. Because of that you need to run CMake through the developer console or have NMake set up in the Path.  
The only supported compiler is MSVC. MinGW might also work, but hasn't been tested yet.  


## TODO  
- shadows aren't working properly on some systems  
