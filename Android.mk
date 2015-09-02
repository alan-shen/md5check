LOCAL_PATH := $(call my-dir)

common_src_files := \
	cr32.c \
	md5.c \
	check.cpp

common_c_includes := \
	bootable/recovery \
	external/openssl/include \
	system/extras/ext4_utils \
	external/zlib \
	external/safe-iop/include

common_static_lib := \
	libminzip

common_share_lib := \
	libz \
	libstdc++ \
	libc \
	libselinux

include $(CLEAR_VARS)
LOCAL_SRC_FILES += \
	$(common_src_files)

LOCAL_STATIC_LIBRARIES := $(common_static_lib)
LOCAL_SHARED_LIBRARIES := $(common_share_lib)
LOCAL_C_INCLUDES += $(common_c_includes)

LOCAL_MODULE := libmitvmd5
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES += \
	$(common_src_files)

LOCAL_STATIC_LIBRARIES := $(common_static_lib)
LOCAL_SHARED_LIBRARIES := $(common_share_lib)
LOCAL_C_INCLUDES += $(common_c_includes)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmitvmd5
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES += \
	$(common_src_files) \
	main_check.cpp

LOCAL_STATIC_LIBRARIES := $(common_static_lib)
LOCAL_SHARED_LIBRARIES := $(common_share_lib)
LOCAL_C_INCLUDES += $(common_c_includes)

LOCAL_MODULE := mi_md5chk
#LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_SRC_FILES += \
	$(common_src_files) \
	main_list.cpp

LOCAL_STATIC_LIBRARIES := $(common_static_lib)
LOCAL_SHARED_LIBRARIES := $(common_share_lib)
LOCAL_C_INCLUDES += $(common_c_includes)

LOCAL_MODULE := mi_md5list
#LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(BUILD_EXECUTABLE)
