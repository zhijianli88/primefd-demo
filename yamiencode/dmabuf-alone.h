#ifndef __dmabuf__h__
#define __dmabuf__h__

#include <xf86drm.h>
#include "i915_drm.h"

struct drm_i915_gem_vgtbuffer {
        __u32 vmid;
        __u32 plane_id;
#define I915_VGT_PLANE_PRIMARY 1
#define I915_VGT_PLANE_SPRITE 2
#define I915_VGT_PLANE_CURSOR 3
        __u32 pipe_id;
        __u32 phys_pipe_id;
        __u8  enabled;
        __u8  tiled;
        __u32 bpp;
        __u32 hw_format;
        __u32 drm_format;
        __u32 start;
        __u32 x_pos;
        __u32 y_pos;
        __u32 x_offset;
        __u32 y_offset;
        __u32 size;
        __u32 width;
        __u32 height;
        __u32 stride;
        __u64 user_ptr;
        __u32 user_size;
        __u32 flags;
#define I915_VGTBUFFER_READ_ONLY (1<<0)
#define I915_VGTBUFFER_QUERY_ONLY (1<<1)
#define I915_VGTBUFFER_CHECK_CAPABILITY (1<<2)
#define I915_VGTBUFFER_UNSYNCHRONIZED 0x80000000
        __u32 handle;
};

extern struct drm_i915_gem_vgtbuffer vgtbuffer;

int test_dmabuf(int fd1, int vmid, struct drm_i915_gem_vgtbuffer *);
#endif
