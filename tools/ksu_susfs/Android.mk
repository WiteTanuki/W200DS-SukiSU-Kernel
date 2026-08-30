LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := ksu_susfs
LOCAL_SRC_FILES := main.c
LOCAL_CFLAGS := -O2 -Wall -Wextra -Werror -fstack-protector-strong \
	-D_FORTIFY_SOURCE=2 -ffunction-sections -fdata-sections
LOCAL_LDFLAGS := -Wl,--gc-sections
LOCAL_STRIP := true
include $(BUILD_EXECUTABLE)
