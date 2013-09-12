# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := secondreality-android
LOCAL_SRC_FILES := u2-port.c platform-android.c readp.c sin1024.c \
	u2/alku/main.c \
	u2/pam/outtaa.c u2/pam/pam-asm.c \
	u2/beg/beg.c \
	u2/glenz/glenz-main.c u2/glenz/zoomer.c u2/glenz/glenz-asm.c \
	u2/lens/lens-main.c u2/lens/lens-asm.c \
	u2/dots/dots-main.c u2/dots/dots-asm.c \
	u2/end/end.c
LOCAL_ARM_MODE	:= arm
LOCAL_CFLAGS	+= -march=armv7-a -mtune=cortex-a8 -mfloat-abi=softfp -mfpu=neon -g #-Os
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv1_CM
LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
