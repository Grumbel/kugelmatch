# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# libmain.so: the game, linked against the prebuilt SDL2 (see build-apk.sh).
# Headers are staged at jni/SDL/include/SDL2/SDL.h; the code uses #include <SDL.h>
# (pkg-config style), so LOCAL_C_INCLUDES must list the SDL2 subdirectory.
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := main
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(wildcard $(LOCAL_PATH)/*.cpp))
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../SDL/include/SDL2 \
	$(LOCAL_PATH)/../SDL/include
LOCAL_CPPFLAGS += -std=c++17 -Wno-psabi -Wall -Wextra -O2 \
	-DKUGELMATCH_VERSION_STRING=\"$(KUGELMATCH_VERSION)\"
LOCAL_CPP_FEATURES := exceptions rtti
LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_LDLIBS := -llog -landroid -lGLESv2 -lEGL

include $(BUILD_SHARED_LIBRARY)
