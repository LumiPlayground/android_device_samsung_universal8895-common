/*
 * Copyright 2013, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed toggle an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "ExynosCameraMemoryAllocator"
#include "ExynosCameraMemory.h"

namespace android {


gralloc_module_t const *ExynosCameraGrallocAllocator::m_grallocHal;
gralloc_module_t const *ExynosCameraStreamAllocator::m_grallocHal;

ExynosCameraGraphicBufferAllocator::ExynosCameraGraphicBufferAllocator(int cameraId)
{
    m_cameraId = cameraId;
}

ExynosCameraGraphicBufferAllocator::~ExynosCameraGraphicBufferAllocator()
{
}

status_t ExynosCameraGraphicBufferAllocator::init(void)
{
    m_width = 0;
    m_height = 0;
    m_stride = 0;
    m_halPixelFormat = 0;
    m_grallocUsage = GRALLOC_SET_USAGE_FOR_CAMERA;

    for (int i = 0; i < VIDEO_MAX_FRAME; i++) {
        m_flagGraphicBufferAlloc[i] = false;
    }

    return NO_ERROR;
}

status_t ExynosCameraGraphicBufferAllocator::setSize(int width, int height, int stride)
{
    m_width  = width;
    m_height = height;
    m_stride = stride;

    return NO_ERROR;
}

status_t ExynosCameraGraphicBufferAllocator::getSize(int *width, int *height, int *stride)
{
    *width = m_width;
    *height = m_height;
    *stride = m_stride;

    return NO_ERROR;
}

status_t ExynosCameraGraphicBufferAllocator::setHalPixelFormat(int halPixelFormat)
{
    m_halPixelFormat = halPixelFormat;

    return NO_ERROR;
}

int ExynosCameraGraphicBufferAllocator::getHalPixelFormat(void)
{
    return m_halPixelFormat;
}

status_t ExynosCameraGraphicBufferAllocator::setGrallocUsage(int grallocUsage)
{
    m_grallocUsage = grallocUsage;

    return NO_ERROR;
}

int ExynosCameraGraphicBufferAllocator::getGrallocUsage(void)
{
    return m_grallocUsage;
}

sp<GraphicBuffer> ExynosCameraGraphicBufferAllocator::alloc(int index, int planeCount, int fdArr[], char *bufAddr[], unsigned int bufSize[])
{
    if ((index < 0) || (index >= VIDEO_MAX_FRAME)) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Buffer index error (%d/%d), assert!!!!",
            __FUNCTION__, __LINE__, index, VIDEO_MAX_FRAME);
    }

    sp<GraphicBuffer> graphicBuffer;

    if (m_flagGraphicBufferAlloc[index] == false) {
        graphicBuffer = m_alloc(index, planeCount, fdArr, bufAddr, bufSize, m_width, m_height, m_halPixelFormat, m_grallocUsage, m_stride);
        if (graphicBuffer == 0) {
            ALOGE("ERR(%s[%d]):m_alloc() fail", __FUNCTION__, __LINE__);
            goto done;
        }
    } else {
        graphicBuffer = m_graphicBuffer[index];
        if (graphicBuffer == 0) {
            ALOGE("ERR(%s[%d]):m_graphicBuffer[%d] == 0. so, fail", __FUNCTION__, __LINE__, index);
            goto done;
        }
    }

done:
    return graphicBuffer;
}

status_t ExynosCameraGraphicBufferAllocator::free(int index)
{
    if (m_flagGraphicBufferAlloc[index] == false)
        return NO_ERROR;

    m_flagGraphicBufferAlloc[index] = false;

    delete m_privateHandle[index];
    m_privateHandle[index] = NULL;

    m_graphicBuffer[index] = 0;

    return NO_ERROR;
}

sp<GraphicBuffer> ExynosCameraGraphicBufferAllocator::m_alloc(int index,
                                                              int planeCount,
                                                              int fdArr[],
                                                              char *bufAddr[],
                                                              unsigned int bufSize[],
                                                              int width,
                                                              int height,
                                                              int halPixelFormat,
                                                              int grallocUsage,
                                                              int stride)
{
    if (m_flagGraphicBufferAlloc[index] == true) {
        ALOGE("ERR(%s[%d]):%d is already allocated. so, fail!!",
            __FUNCTION__, __LINE__, index);
        goto done;
    }

    if (planeCount <= 0) {
        ALOGE("ERR(%s[%d]):invalid value : planeCount(%d). so, fail!!",
            __FUNCTION__, __LINE__, planeCount);
        goto done;
    }

    if (width == 0 || height == 0 || halPixelFormat == 0 || grallocUsage <= 0 || stride <= 0) {
        ALOGE("ERR(%s[%d]):invalid value : width(%d), height(%d), halPixelFormat(%d), grallocUsage(%d), stride(%d). so, fail!!",
            __FUNCTION__, __LINE__, width, height, halPixelFormat, grallocUsage, stride);
        goto done;
    }

    if (planeCount == 1) {
        m_privateHandle[index] = new private_handle_t(fdArr[0], -1, -1, bufSize[0], 0, 0, grallocUsage, width, height,
            halPixelFormat, halPixelFormat, halPixelFormat, width, height, 0);

        m_privateHandle[index]->base = (uint64_t)bufAddr[0];
        m_privateHandle[index]->offset = 0;
    } else {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):planeCount(%d) is not yet support, assert!!!!",
            __FUNCTION__, __LINE__, planeCount);
    }

    ALOGV("DEBUG(%s[%d]):new GraphicBuffer(bufAddr(%p), width(%d), height(%d), halPixelFormat(%d), grallocUsage(%d), stride(%d), m_privateHandle[%d](%p), false)",
            __FUNCTION__, __LINE__, bufAddr[0], width, height, halPixelFormat, grallocUsage, stride, index, m_privateHandle[index]);

    m_graphicBuffer[index] = new GraphicBuffer((native_handle_t*)m_privateHandle[index], GraphicBuffer::WRAP_HANDLE,
                                               width, height, (PixelFormat)halPixelFormat, 1, (uint64_t)grallocUsage, stride);

    m_flagGraphicBufferAlloc[index] = true;

done:
    return m_graphicBuffer[index];
}

ExynosCameraIonAllocator::ExynosCameraIonAllocator(int cameraId)
{
    m_cameraId    = cameraId;
    m_ionClient   = 0;
    m_ionAlign    = 0;
    m_ionHeapMask = 0;
    m_ionFlags    = 0;
}

status_t ExynosCameraIonAllocator::free(
        __unused int size,
        int *fd,
        char **addr,
        bool mapNeeded)
{
    status_t ret = NO_ERROR;
    int ionFd = *fd;
    char *ionAddr = *addr;

    if (ionFd < 0) {
        ALOGE("ERR(%s):ion_fd is lower than zero", __FUNCTION__);
        ret = BAD_VALUE;
        goto func_exit;
    }

    if (mapNeeded == true) {
        if (ionAddr == NULL) {
            ALOGE("ERR(%s):ion_addr equals NULL", __FUNCTION__);
            ret = BAD_VALUE;
            goto func_close_exit;
        }

        if (munmap(ionAddr, size) < 0) {
            ALOGE("ERR(%s):munmap failed", __FUNCTION__);
            ret = INVALID_OPERATION;
            goto func_close_exit;
        }
    }

func_close_exit:

#ifdef USE_LIB_ION_LEGACY
    ion_close(ionFd);
#else
    exynos_ion_close(ionFd);
#endif
    ionFd   = -1;
    ionAddr = NULL;

func_exit:

    *fd   = ionFd;
    *addr = ionAddr;

    return ret;
}

status_t ExynosCameraIonAllocator::map(int size, int fd, char **addr)
{
    status_t ret = NO_ERROR;
    char *ionAddr = NULL;

    if (size == 0) {
        ALOGE("ERR(%s):size equals zero", __FUNCTION__);
        ret = BAD_VALUE;
        goto func_exit;
    }

    if (fd <= 0) {
        ALOGE("ERR(%s):fd=%d failed", __FUNCTION__, fd);
        ret = BAD_VALUE;
        goto func_exit;
    }

    ionAddr = (char *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    if (ionAddr == (char *)MAP_FAILED || ionAddr == NULL) {
        ALOGE("ERR(%s):ion_map(size=%d) failed, (fd=%d), (%s)", __FUNCTION__, size, fd, strerror(errno));
        close(fd);
        ionAddr = NULL;
        ret = INVALID_OPERATION;
        goto func_exit;
    }

func_exit:

    *addr = ionAddr;

    return ret;
}

ExynosCameraGrallocAllocator::ExynosCameraGrallocAllocator(int cameraId)
{
    m_cameraId = cameraId;
    m_allocator = NULL;
    m_minUndequeueBufferMargin = 0;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&m_grallocHal))
        ALOGE("ERR(%s):Loading gralloc HAL failed", __FUNCTION__);

    m_grallocUsage = GRALLOC_SET_USAGE_FOR_CAMERA;
}

ExynosCameraGrallocAllocator::~ExynosCameraGrallocAllocator()
{
    m_minUndequeueBufferMargin = 0;
}

status_t ExynosCameraGrallocAllocator::init(
        preview_stream_ops *allocator,
        int bufCount,
        int minUndequeueBufferCount,
        int grallocUsage)
{
    status_t ret = NO_ERROR;

    m_allocator = allocator;
    if( minUndequeueBufferCount < 0 ) {
        m_minUndequeueBufferMargin = 0;
    } else {
        m_minUndequeueBufferMargin = minUndequeueBufferCount;
    }

    if (setBufferCount(bufCount) != 0) {
        ALOGE("ERR(%s):setBufferCount failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allocator->set_usage(m_allocator, grallocUsage) != 0) {
        ALOGE("ERR(%s):set_usage failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    m_grallocUsage = grallocUsage;
    m_halPixelFormat = 0;

    if (m_grallocHal == NULL) {
        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&m_grallocHal))
            ALOGE("ERR(%s):Loading gralloc HAL failed", __FUNCTION__);
    }

func_exit:

    return ret;
}

status_t ExynosCameraGrallocAllocator::alloc(
        buffer_handle_t **bufHandle,
        int fd[],
        char *addr[],
        int  *bufStride,
        bool *isLocked)
{
    status_t ret = NO_ERROR;
    int   width  = 0;
    int   height = 0;
    int   grallocFd[3] = {0};
    android_ycbcr ycbcr;
    const private_handle_t *priv_handle = NULL;
    ExynosCameraDurationTimer   dequeuebufferTimer;
    ExynosCameraDurationTimer   lockbufferTimer;

    for (int retryCount = 5; retryCount > 0; retryCount--) {
#ifdef EXYNOS_CAMERA_MEMORY_TRACE
        ALOGI("INFO(%s[%d]):dequeue_buffer retryCount=%d",
            __FUNCTION__, __LINE__, retryCount);
#endif
        if (m_allocator == NULL) {
            ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
            ret = INVALID_OPERATION;
            goto func_exit;
        }

        dequeuebufferTimer.start();
        ret = m_allocator->dequeue_buffer(m_allocator, bufHandle, bufStride);
        dequeuebufferTimer.stop();

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
        ALOGD("DEBUG(%s[%d]):Check dequeue buffer performance, duration(%ju usec)",
                __FUNCTION__, __LINE__, dequeuebufferTimer.durationUsecs());
#else
        if (dequeuebufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
            ALOGW("WRN(%s[%d]):dequeue_buffer() duration(%ju msec)",
                    __FUNCTION__, __LINE__, dequeuebufferTimer.durationMsecs());
#endif

        if (ret == NO_INIT) {
            ALOGW("WARN(%s):BufferQueue is abandoned", __FUNCTION__);
            return ret;
        } else if (ret != 0) {
            ALOGE("ERR(%s):dequeue_buffer failed", __FUNCTION__);
            continue;
        }

        if (bufHandle == NULL) {
            ALOGE("ERR(%s):bufHandle == NULL failed, retry(%d)", __FUNCTION__, retryCount);
            continue;
        }

        lockbufferTimer.start();
        ret = m_allocator->lock_buffer(m_allocator, *bufHandle);
        lockbufferTimer.stop();
        if (ret != 0)
            ALOGE("ERR(%s):lock_buffer failed, but go on to the next step ...", __FUNCTION__);

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
        ALOGD("DEBUG(%s[%d]):Check lock buffer performance, duration(%ju usec)",
                __FUNCTION__, __LINE__, lockbufferTimer.durationUsecs());
#else
        if (lockbufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
            ALOGW("WRN(%s[%d]):lock_buffer() duration(%ju msec)",
                    __FUNCTION__, __LINE__, lockbufferTimer.durationMsecs());
#endif

        if (*isLocked == false) {
            lockbufferTimer.start();
            ret = m_grallocHal->lock_ycbcr(m_grallocHal, **bufHandle, GRALLOC_LOCK_FOR_CAMERA,
                                    0, 0,/* left, top */ width, height, &ycbcr);
            lockbufferTimer.stop();

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
            ALOGD("DEBUG(%s[%d]):Check grallocHAL lock performance, duration(%ju usec)",
                    __FUNCTION__, __LINE__, lockbufferTimer.durationUsecs());
#else
            if (lockbufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
                ALOGW("WRN(%s[%d]):grallocHAL->lock_ycbcr() duration(%ju msec)",
                        __FUNCTION__, __LINE__, lockbufferTimer.durationMsecs());
#endif

            if (ret != 0) {
                ALOGE("ERR(%s):grallocHal->lock_ycbcr failed.. retry", __FUNCTION__);

                if (m_allocator->cancel_buffer(m_allocator, *bufHandle) != 0)
                    ALOGE("ERR(%s):cancel_buffer failed", __FUNCTION__);
                ret = INVALID_OPERATION;
                goto func_exit;
            }
            break;
        }
    }

    if (bufHandle == NULL) {
        ALOGE("ERR(%s):bufHandle == NULL failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (*bufHandle == NULL) {
        ALOGE("@@@@ERR(%s):*bufHandle == NULL failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    priv_handle = private_handle_t::dynamicCast(**bufHandle);

    grallocFd[0] = priv_handle->fd;
    grallocFd[1] = priv_handle->fd1;
    grallocFd[2] = priv_handle->fd2;
    *isLocked    = true;

func_exit:

    switch (m_halPixelFormat) {
        /* three plane */
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
        fd[2] = grallocFd[2];
        addr[2] = (char *)ycbcr.cr;
        /* two plane */
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
        fd[1] = grallocFd[1];
        if (m_halPixelFormat == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M) {
            addr[1] = (char *)ycbcr.cb;
        } else if (m_halPixelFormat == HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M ||
                m_halPixelFormat == HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL) {
            addr[1] = (char *)ycbcr.cr;
        }
        /* one plane */
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YV12:
        fd[0] = grallocFd[0];
        addr[0] = (char *)ycbcr.y;
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "invalid halPixelFormat(%x)", m_halPixelFormat);
        break;
    }

    return ret;
}

status_t ExynosCameraGrallocAllocator::setBufferCount(int bufCount)
{
    status_t ret = NO_ERROR;

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        ret = INVALID_OPERATION;
        return ret;
    }

    if (m_allocator->set_buffer_count(m_allocator, bufCount) != 0) {
        ALOGE("ERR(%s):set_buffer_count failed [bufCount=%d]", __FUNCTION__, bufCount);
        ret = INVALID_OPERATION;
    }

    return ret;
}
status_t ExynosCameraGrallocAllocator::setBuffersGeometry(
        int width,
        int height,
        int halPixelFormat)
{
    status_t ret = NO_ERROR;

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        ret = INVALID_OPERATION;
        return ret;
    }

    if (m_allocator->set_buffers_geometry(
                    m_allocator,
                    width, height,
                    halPixelFormat) != 0) {
        ALOGE("ERR(%s):set_buffers_geometry failed", __FUNCTION__);
        ret = INVALID_OPERATION;
    }

    m_halPixelFormat = halPixelFormat;

    return ret;
}

int ExynosCameraGrallocAllocator::getGrallocUsage(void)
{
    return m_grallocUsage;
}

status_t ExynosCameraGrallocAllocator::getAllocator(preview_stream_ops **allocator)
{
    *allocator = m_allocator;

    return NO_ERROR;
}

int ExynosCameraGrallocAllocator::getMinUndequeueBuffer()
{
    int minUndeqBufCount = 0;

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        return INVALID_OPERATION;
    }

    if (m_allocator->get_min_undequeued_buffer_count(m_allocator, &minUndeqBufCount) != 0) {
        ALOGE("ERR(%s):enqueue_buffer failed", __FUNCTION__);
        return INVALID_OPERATION;
    }

    return minUndeqBufCount < 2 ? (minUndeqBufCount + m_minUndequeueBufferMargin) : minUndeqBufCount;
}

status_t ExynosCameraGrallocAllocator::dequeueBuffer(
        buffer_handle_t **bufHandle,
        int fd[],
        char *addr[],
        bool *isLocked, Mutex *lock)
{
    int bufStride = 0;
    status_t ret = NO_ERROR;

    lock->unlock();
    ret = alloc(bufHandle, fd, addr, &bufStride, isLocked);
    lock->lock();
    if (ret == NO_INIT) {
        ALOGW("WARN(%s):BufferQueue is abandoned", __FUNCTION__);
        return ret;
    } else if (ret != NO_ERROR) {
        ALOGE("ERR(%s):alloc failed", __FUNCTION__);
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t ExynosCameraGrallocAllocator::enqueueBuffer(buffer_handle_t *handle, Mutex *lock)
{
    status_t ret = NO_ERROR;
    ExynosCameraDurationTimer   enqueuebufferTimer;

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        return INVALID_OPERATION;
    }

    enqueuebufferTimer.start();
    lock->unlock();
    ret = m_allocator->enqueue_buffer(m_allocator, handle);
    lock->lock();
    enqueuebufferTimer.stop();

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
    ALOGD("DEBUG(%s[%d]):Check enqueue buffer performance, duration(%ju usec)",
            __FUNCTION__, __LINE__, enqueuebufferTimer.durationUsecs());
#else
    if (enqueuebufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
        ALOGW("WRN(%s[%d]):enqueue_buffer() duration(%ju msec)",
                __FUNCTION__, __LINE__, enqueuebufferTimer.durationMsecs());
#endif

    if (ret != 0) {
        ALOGE("ERR(%s):enqueue_buffer failed", __FUNCTION__);
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t ExynosCameraGrallocAllocator::cancelBuffer(buffer_handle_t *handle, Mutex *lock)
{
    status_t ret = NO_ERROR;
    ExynosCameraDurationTimer   cancelbufferTimer;

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        return INVALID_OPERATION;
    }

    if (m_grallocHal->unlock(m_grallocHal, *handle) != 0) {
        ALOGE("ERR(%s):grallocHal->unlock failed", __FUNCTION__);
        return INVALID_OPERATION;
    }

    cancelbufferTimer.start();
    lock->unlock();
    ret = m_allocator->cancel_buffer(m_allocator, handle);
    lock->lock();
    cancelbufferTimer.stop();

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
    ALOGD("DEBUG(%s[%d]):Check cancel buffer performance, duration(%ju usec)",
            __FUNCTION__, __LINE__, cancelbufferTimer.durationUsecs());
#else
    if (cancelbufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
        ALOGW("WRN(%s[%d]):cancel_buffer() duration(%ju msec)",
                __FUNCTION__, __LINE__, cancelbufferTimer.durationMsecs());
#endif

    if (ret != 0) {
        ALOGE("ERR(%s):cancel_buffer failed", __FUNCTION__);
        return INVALID_OPERATION;
    }
    return NO_ERROR;
}

ExynosCameraStreamAllocator::ExynosCameraStreamAllocator()
{
    m_allocator = NULL;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&m_grallocHal))
        ALOGE("ERR(%s):Loading gralloc HAL failed", __FUNCTION__);
}

ExynosCameraStreamAllocator::~ExynosCameraStreamAllocator()
{
}

int ExynosCameraStreamAllocator::lock(
        buffer_handle_t **bufHandle,
        int fd[],
        char *addr[],
        bool *isLocked,
        int planeCount)
{
    int ret = 0;
    uint32_t width  = 0;
    uint32_t height = 0;
    uint32_t usage  = 0;
    uint32_t format = 0;
    void  *grallocAddr[3] = {NULL};
    const private_handle_t *priv_handle = NULL;
    int   grallocFd[3] = {0};
    android_ycbcr ycbcr;
    ExynosCameraDurationTimer   lockbufferTimer;

    if (bufHandle == NULL) {
        ALOGE("ERR(%s):bufHandle equals NULL, failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (*bufHandle == NULL) {
        ALOGE("ERR(%s):*bufHandle == NULL, failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    width  = m_allocator->width;
    height = m_allocator->height;
    usage  = m_allocator->usage;
    format = m_allocator->format;

    switch (format) {
    case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_RAW16:
    case HAL_PIXEL_FORMAT_RAW_OPAQUE:
    case HAL_PIXEL_FORMAT_BLOB:
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        if (planeCount == 1) {
            lockbufferTimer.start();
            ret = m_grallocHal->lock(
                    m_grallocHal,
                    **bufHandle,
                    usage,
                    0, 0, /* left, top */
                    width, height,
                    grallocAddr);
            lockbufferTimer.stop();
            break;
        }
    default:
        android_ycbcr ycbcr;
        lockbufferTimer.start();
        ret = m_grallocHal->lock_ycbcr(
                m_grallocHal,
                **bufHandle,
                usage,
                0, 0, /* left, top */
                width, height,
                &ycbcr);
        lockbufferTimer.stop();
        break;
    }

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
    ALOGD("DEBUG(%s[%d]):Check grallocHAL lock performance, duration(%ju usec)",
            __FUNCTION__, __LINE__, lockbufferTimer.durationUsecs());
#else
    if (lockbufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
        ALOGW("WRN(%s[%d]):grallocHAL->lock() duration(%ju msec)",
                __FUNCTION__, __LINE__, lockbufferTimer.durationMsecs());
#endif

    if (ret != 0) {
        ALOGE("ERR(%s):grallocHal->lock failed.. ", __FUNCTION__);
        goto func_exit;
    }

    priv_handle = private_handle_t::dynamicCast(**bufHandle);

    switch (planeCount) {
    case 3:
        grallocFd[2] = priv_handle->fd2;
        grallocAddr[2] = ycbcr.cr;
    case 2:
        grallocFd[1] = priv_handle->fd1;
        grallocAddr[1] = ycbcr.cb;
    case 1:
        grallocFd[0] = priv_handle->fd;
        if (grallocAddr[0] == NULL) {
            grallocAddr[0] = ycbcr.y;
        }
        break;
    default:
        break;
    }

    *isLocked    = true;

func_exit:
    switch (planeCount) {
    case 3:
        fd[2]   = grallocFd[2];
        addr[2] = (char *)grallocAddr[2];
    case 2:
        fd[1]   = grallocFd[1];
        addr[1] = (char *)grallocAddr[1];
    case 1:
        fd[0]   = grallocFd[0];
        addr[0] = (char *)grallocAddr[0];
        break;
    default:
        break;
    }

    return ret;
}
}
