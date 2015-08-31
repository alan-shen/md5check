LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES += \
	cr32.c \
    md5.c \
    check.cpp \
    main.cpp

LOCAL_STATIC_LIBRARIES := \
    libminzip \
    libz \
    libstdc++ \
    libc 

LOCAL_C_INCLUDES += bootable/recovery
LOCAL_C_INCLUDES += external/openssl/include
LOCAL_C_INCLUDES += system/extras/ext4_utils

LOCAL_CFLAGS += -DMTK_ROOT_NORMAL_CHECK
LOCAL_CFLAGS += -DMTK_ROOT_ADVANCE_CHECK

LOCAL_MODULE := checkmd5
LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_EXECUTABLE)
