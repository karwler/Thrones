# Thrones  
Based on the board game by Chad Anthony Randell.  
See [RULES](doc/rules.html) or [DOCS](doc/docs.html) for details.  

## Build  
Used libraries are assimp, libcurl, GLEW, GLM, SDL2, SDL2_image, SDL2_ttf and by extension FreeType, HarfBuzz, stb_image and zlib. The included fonts are Romanesque Serif Regular and Merriweather Regular.  
The CMakeLists.txt is written for CMake 3.12.4 with Clang, GCC or MSVC which need to support C++17.  

CMake options:  
- APPIMAGE : bool = OFF  
  - package the client as an AppImage  
- ARCH : string  
  - CPU type string to pass to Clang's or GCC's "-march" option  
- EMBASE : string = ${CMAKE_BINARY_DIR}  
  - location where the target assets_emscripten should put the asset directory  
- EXTERNAL : bool = ON  
  - store preferences externally by default  
- LIBDROID : bool = OFF  
  - download libraries for Android Studio  
- OPENGLES : int = 0  
  - set to any non zero number to use OpenGL ES 3.0 or 32 for debugging  
- OPENVR : bool = ON  
  - build with VR support  
- SERVICE : bool = OFF  
  - server program won't check keyboard input  
- SYSLIBS : bool = ON on Linux, OFF on macOS  
  - use the installed assimp and curl libraries instead of downloading them  
- TEXDIV : int = 1 for OpenGL, 2 for OpenGL ES  
  - texture assets' resolution divisor (1 = full size, 2 = half size, etc.)  

### Android  
The "android" directory can be imported in Android Studio as a project, which builds only the game client.  
To install the necessary additional files, run CMake with the "-DLIBDROID=1" option.  
Next the CMake target "assets_android" needs to be built separately, which will create the assets in "android/app/src/main/assets". The requirements for this are as listed for the other systems.  
It might be necessary to set the NDK locations, which can be done in Android Studio under "File -> Project Structure -> SDK Location".  
If that isn't possible, try setting the line "ndk.dir=<path>" in the local.properties file.  
If you're on Windows, make sure that your Git supports symbolic links or check that "android/app/jni/src" contains the files of "src".  

```bash
mkdir build
cd build
cmake .. -DLIBDROID=1
make assets_android
```

### Emscripten  
A makefile with a target for the game client can be created with the CMake file using ```emcmake cmake```. Additional libraries will be downloaded.  
Before building the program, the target "assets_emscripten" needs to be built separately, which will create the "share" directory containing the assets. This directory will either be moved automatically by setting the CMake option "EMBASE" or will have to be copied manually to the directory of the makefile for the Emscripten build.  
The game client can then be built with ```emmake make```.  

```bash
mkdir build buildem
cd build
cmake .. -DEMBASE=buildem
make assets_emscripten
cd ../buildem
emcmake cmake ..
emmake make
```

### Linux  
The only supported compilers are Clang and GCC.
GLEW and GLM are downloaded and built while running CMake. SDL2, SDL2_image, SDL2_ttf and libcurl are only downloaded and built when building an AppImage.  
To create a menu entry for the game client, you can use the "rsc/thrones.desktop" launcher file.  
When building the game client on a Raspberry Pi, the CMake option "-DOPENGLES=1" should be set.  

```bash
mkdir build
cd build
cmake ..
make
```

### macOS  
All necessary dependencies are downloaded when running CMake.  

```bash
mkdir build
cd build
cmake ..
make
```

### Windows  
The only supported compilers are MSVC and MinGW.  
All necessary libraries are downloaded and built with NMake or mingw32-make while running CMake. Because of that you might need to run CMake through a developer console or have NMake or mingw32-make set up in the Path.  

```batch
mkdir build
cd build
cmake .. -G "NMake Makefiles"
nmake
```

## TODO  
- test and fix everything  
