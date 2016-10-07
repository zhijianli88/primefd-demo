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

	gem_userptr(fd, (uint32_t *)ptr, size, 0, userptr_flags, &handle);

	return handle;
}

static int export_handle(int fd, uint32_t handle, int *outfd)
{
	struct drm_prime_handle args;
	int ret;

	args.handle = handle;
	args.flags = DRM_CLOEXEC;
	args.fd = -1;

	ret = drmIoctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args);
	if (ret)
		ret = errno;
	*outfd = args.fd;

	return ret;
}

int test_dmabuf(int fd1)
{
	uint32_t handle;
	int dma_buf_fd = -1;
	int ret;

//	fd1 = open("/dev/dri/card0", O_RDWR);

	//fd1 = drm_open_driver(DRIVER_INTEL);

	handle = create_userptr_bo(fd1, sizeof(linear));
	memset(ptr, 0, sizeof(linear));

	ret = export_handle(fd1, handle, &dma_buf_fd);
#if 0
	if (userptr_flags & LOCAL_I915_USERPTR_UNSYNCHRONIZED && ret) {
		igt_assert(ret == EINVAL || ret == ENODEV);
		free_userptr_bo(fd1, handle);
		close(fd1);
		return 0;
	} else {
		igt_assert_eq(ret, 0);
		igt_assert_lte(0, dma_buf_fd);
	}

	fd2 = drm_open_driver(DRIVER_INTEL);
	handle_import = prime_fd_to_handle(fd2, dma_buf_fd);
	check_bo(fd1, handle, 1, fd2, handle_import);

	/* close dma_buf, check whether nothing disappears. */
	close(dma_buf_fd);
	check_bo(fd1, handle, 1, fd2, handle_import);

	/* destroy userptr object and expect SIGBUS */
	free_userptr_bo(fd1, handle);
	close(fd1);

	if (gem_has_llc(fd2)) {
		struct sigaction sigact, orig_sigact;

		memset(&sigact, 0, sizeof(sigact));
		sigact.sa_sigaction = sigbus;
		sigact.sa_flags = SA_SIGINFO;
		ret = sigaction(SIGBUS, &sigact, &orig_sigact);
		igt_assert_eq(ret, 0);

		orig_sigbus = orig_sigact.sa_sigaction;

		sigbus_cnt = 0;
		check_bo(fd2, handle_import, -1, fd2, handle_import);
		igt_assert(sigbus_cnt > 0);

		ret = sigaction(SIGBUS, &orig_sigact, NULL);
		igt_assert_eq(ret, 0);
	}

	close(fd2);
	reset_handle_ptr();
#endif
	return dma_buf_fd;
}
#if 0
int main(int argc, char *argv[])
{
   return test_dmabuf();
}
#endif

