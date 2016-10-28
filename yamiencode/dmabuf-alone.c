#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <sys/mman.h>
#include <xf86drm.h>

#include "drm.h"
#include "i915_drm.h"
#include "dmabuf-alone.h"

#define PLAN_A 0

#if PLAN_A
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define DRIVER_INTEL (0x1 << 1)

#define LOCAL_I915_GEM_USERPTR       0x33
#define LOCAL_IOCTL_I915_GEM_USERPTR DRM_IOWR (DRM_COMMAND_BASE + LOCAL_I915_GEM_USERPTR, struct local_i915_gem_userptr)
struct local_i915_gem_userptr {
	uint64_t user_ptr;
	uint64_t user_size;
	uint32_t flags;
#define LOCAL_I915_USERPTR_READ_ONLY (1<<0)
#define LOCAL_I915_USERPTR_UNSYNCHRONIZED (1<<31)
	uint32_t handle;
};

static uint32_t userptr_flags = LOCAL_I915_USERPTR_UNSYNCHRONIZED;

#define WIDTH 512
#define HEIGHT 512

static uint32_t linear[WIDTH*HEIGHT];
void *ptr;

static int
gem_userptr(int fd, void *ptr, int size, int read_only, uint32_t flags, uint32_t *handle)
{
	struct local_i915_gem_userptr userptr;
	int ret;

	memset(&userptr, 0, sizeof(userptr));
	userptr.user_ptr = (uintptr_t)ptr;
	userptr.user_size = size;
	userptr.flags = flags;
	if (read_only)
		userptr.flags |= LOCAL_I915_USERPTR_READ_ONLY;

	ret = drmIoctl(fd, LOCAL_IOCTL_I915_GEM_USERPTR, &userptr);
	if (ret)
		ret = errno;
	if (ret == 0)
		*handle = userptr.handle;

	return ret;
}

static uint32_t create_userptr_bo(int fd, uint64_t size)
{
	uint32_t handle;

	ptr = mmap(NULL, size,
		   PROT_READ | PROT_WRITE,
		   MAP_ANONYMOUS | MAP_SHARED,
		   -1, 0);

	if (gem_userptr(fd, (uint32_t *)ptr, size, 0, userptr_flags, &handle))
	    fprintf(stderr, "gem_userptr failed\n");

	return handle;
}
#else

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <limits.h>
#define __user
#include <xf86drm.h>
#include "i915_drm.h"

#define DRM_I915_GEM_VGTBUFFER          0x36
#define DRM_IOCTL_I915_GEM_VGTBUFFER    DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_VGTBUFFER, struct drm_i915_gem_vgtbuffer)

static void write_ppm(uint8_t *data,  int w, int h, int bpp)
{
	FILE *fp;
	int i;

	fprintf(stderr, "w %d, h %d, bpp %d\n", w, h, bpp);

	if ((fp = fopen("desktop.ppm", "wb")) == NULL) {
		fprintf(stderr, "failed to open file\n");
		exit(1);
	}

	fprintf(fp, "%s\n%d %d\n255\n", "P6", w, h);

	for (i = 0; i < (w * h * bpp / 8); i++, data++) {
		if (i % 4 == 3)
			continue;
		putc((int) *data, fp);
	}

	fclose(fp);
	return;
}

int create_vgtbuffer_handle(int fd1, int vmid, struct drm_i915_gem_vgtbuffer *v)
{
	int ret;
	struct drm_i915_gem_vgtbuffer vcreate;

	memset(&vcreate, 0, sizeof(struct drm_i915_gem_vgtbuffer));
	vcreate.flags = I915_VGTBUFFER_CHECK_CAPABILITY;
	if (!drmIoctl(fd1, DRM_IOCTL_I915_GEM_VGTBUFFER, &vcreate)) {
		perror("check good");
	} else {
		perror("check failed\n");
		exit(1);
	}

	drmSetMaster(fd1);

	memset(&vcreate,0,sizeof(struct drm_i915_gem_vgtbuffer));
	vcreate.vmid = vmid;//xen_domid; //Gust xl number
	vcreate.plane_id = I915_VGT_PLANE_PRIMARY;
	vcreate.phys_pipe_id = UINT_MAX;//UINT_MAX;
	ret = drmIoctl(fd1, DRM_IOCTL_I915_GEM_VGTBUFFER, &vcreate);
	if (ret) {
	    perror("ioctl DRM_IOCTL_I915_GEM_VGTBUFFER error\n");
	    exit(1);
	}
	printf("vmid=%d\n", vcreate.vmid);
	printf("handle=%d\n", vcreate.handle);
	printf("vcreate.width=%d, height=%d\n",vcreate.width,  vcreate.height);
	printf("vcreate.stride=%d\n",vcreate.stride);
	printf("bpp:%d\n", vcreate.bpp);
	printf("tiled:%d\n", vcreate.tiled);
	printf("start=%d\n", vcreate.start);
	printf("size=%d\n", vcreate.size);
	printf("fd=%d\n", fd1);
	printf("stride %d, user_ptr %lx, user_size %d, drm_format %x, hw_format %x\n",
		vcreate.stride, vcreate.user_ptr, vcreate.user_size, vcreate.drm_format,
		vcreate.hw_format);

	struct drm_i915_gem_mmap_gtt mmap_arg;
	memset(&mmap_arg,0,sizeof(struct drm_i915_gem_mmap_gtt));
	uint8_t *addr=NULL;
	mmap_arg.handle = vcreate.handle;
	drmIoctl(fd1, DRM_IOCTL_I915_GEM_MMAP_GTT, &mmap_arg);

	addr = (uint8_t *)mmap(0, (vcreate.width*vcreate.height*vcreate.bpp)/8, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd1, mmap_arg.offset);
	printf("handle=%d, offset=%llu, addr=%p\n",mmap_arg.handle, mmap_arg.offset, (unsigned int*)addr);


	write_ppm(addr, vcreate.width, vcreate.height, vcreate.bpp);

	*v = vcreate;
	return vcreate.handle;
}

#endif
static int export_handle(int fd, uint32_t handle, int *outfd)
{
	struct drm_prime_handle args;
	int ret;

	args.handle = handle;
	args.flags = DRM_CLOEXEC;
	args.fd = -1;

	ret = drmIoctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args);
	if (ret) {
		ret = errno;
		fprintf(stderr, "DRM_IOCTL_PRIME_HANDLE_TO_FD failed\n");
	}
	*outfd = args.fd;

	return ret;
}

int test_dmabuf(int fd1, int vmid, struct drm_i915_gem_vgtbuffer *vcreate)
{
	uint32_t handle;
	int dma_buf_fd = -1;
	int ret;
#if PLAN_A
	handle = create_userptr_bo(fd1, sizeof(linear));
	memset(ptr, 0, sizeof(linear));
#else
	handle = create_vgtbuffer_handle(fd1, vmid, vcreate);

#endif

	ret = export_handle(fd1, handle, &dma_buf_fd);
	if (ret) {
           fprintf(stderr, "export_handle failed\n");
	   exit(1);
	}

	return dma_buf_fd;
}
#if 0
int main(int argc, char *argv[])
{
   return test_dmabuf();
}
#endif

