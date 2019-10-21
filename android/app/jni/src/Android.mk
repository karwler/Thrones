LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include $(LOCAL_PATH)/../glm

# Add your application source files here...
LOCAL_SRC_FILES := engine/audioSys.cpp engine/fileSys.cpp engine/scene.cpp engine/windowSys.cpp engine/world.cpp oven/oven.cpp prog/game.cpp prog/netcp.cpp prog/program.cpp prog/progs.cpp server/server.cpp utils/layouts.cpp utils/objects.cpp utils/text.cpp utils/utils.cpp utils/widgets.cpp

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_net SDL2_ttf

LOCAL_LDLIBS := -lGLESv3 -llog

include $(BUILD_SHARED_LIBRARY)
