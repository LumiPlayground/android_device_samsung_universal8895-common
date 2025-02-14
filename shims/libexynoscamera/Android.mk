#
# Copyright (C) 2018 The LineageOS Project
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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := CameraParameters.cpp

LOCAL_MODULE := libexynoscamera_shim
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := libbinder liblog libui libcutils libutils libcamera_metadata
LOCAL_SHARED_LIBRARIES += libion_exynos

LOCAL_C_INCLUDES += \
	$(TOP)/system/media/camera/include \
	$(TOP)/system/core/libion/kernel-headers \
	$(TOP)/hardware/samsung_slsi-linaro/exynos/include \
	$(TOP)/hardware/samsung_slsi-linaro/exynos5/include \
	$(TOP)/hardware/samsung_slsi-linaro/exynos/libcamera/8895 \
	$(TOP)/hardware/samsung_slsi-linaro/exynos/libion/include \
	$(TOP)/hardware/samsung_slsi-linaro/exynos/gralloc3/src \
	$(TOP)/hardware/libhardware_legacy/include/hardware_legacy \
	$(TOP)/frameworks/native/libs/ui/include

LOCAL_SRC_FILES := ExynosCameraMemory.cpp
LOCAL_MODULE := libexynoscamera_gralloc_shim
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_VENDOR_MODULE := true
include $(BUILD_SHARED_LIBRARY)
