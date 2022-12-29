LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := main
SDL_PATH := ../SDL
LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include $(LOCAL_PATH)/../glm
LOCAL_SRC_FILES := engine/audioSys.cpp engine/fileSys.cpp engine/inputSys.cpp engine/scene.cpp engine/shaders.cpp engine/windowSys.cpp engine/world.cpp oven/oven.cpp prog/board.cpp prog/game.cpp prog/guiGen.cpp prog/netcp.cpp prog/program.cpp prog/progs.cpp prog/recorder.cpp prog/types.cpp server/server.cpp utils/context.cpp utils/layouts.cpp utils/objects.cpp utils/settings.cpp utils/text.cpp utils/utils.cpp utils/widgets.cpp
LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_ttf
LOCAL_LDLIBS := -lGLESv3 -llog
include $(BUILD_SHARED_LIBRARY)
