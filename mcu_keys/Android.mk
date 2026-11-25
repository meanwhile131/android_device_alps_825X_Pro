LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := mcu_keys
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := mcu_keys.c
# libts59xhw.so is a 32-bit ARM library from the stock ROM.
LOCAL_MULTILIB := 32
LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := liblog libdl
LOCAL_CFLAGS := -Wall
include $(BUILD_EXECUTABLE)
