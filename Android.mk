LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
    md5_check.cpp \
    md5.c \
    main.cpp

LOCAL_MODULE := checkmd5

LOCAL_MODULE_TAGS := eng

LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -DROOT_CHECK

LOCAL_STATIC_LIBRARIES := \
    libext4_utils_static \
    libsparse_static \
    libminzip \
    libz \
    libmtdutils \
    libfs_mgr \
    libcutils \
    liblog \
    libselinux \
    libstdc++ \
    libm \
    libc 


LOCAL_C_INCLUDES += external/openssl/include
LOCAL_C_INCLUDES += system/extras/ext4_utils 
LOCAL_LDFLAGS := -Wl,--whole-archive $(PRODUCT_OUT)/obj/STATIC_LIBRARIES/libm_intermediates/libm.a -Wl,--no-whole-archive

include $(BUILD_HOST_EXECUTABLE)
