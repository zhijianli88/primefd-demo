#include "yamivideocomp.h"
#include <va/va_drmcommon.h>

using namespace YamiMediaCodec;
#define BPP 32

void SurfaceCreate::bindToSurface(std::vector<VASurfaceID>& surfaces){
	VASurfaceAttribExternalBuffers external;	
	memset(&external, 0, sizeof(external));

	external.pixel_format = VA_FOURCC_BGRX;
	external.width = m_width;
	external.height = m_height;
	external.data_size = m_width * m_height * BPP / 8;
	external.num_planes = 1;
//	external.pitches[0] = m_pitch; //can be obtained from vcreate
	external.buffers = (long unsigned int*)&m_handle;
	external.num_buffers = 1;

	VASurfaceAttrib attribs[2];
	attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
	attribs[0].type = VASurfaceAttribMemoryType;
	attribs[0].value.type = VAGenericValueTypeInteger;
	attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;

	attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
	attribs[1].type = VASurfaceAttribExternalBufferDescriptor;
	attribs[1].value.type = VAGenericValueTypePointer;
	attribs[1].value.value.p = &external;

	VASurfaceID id;
	VAStatus vaStatus = vaCreateSurfaces(*m_display, VA_RT_FORMAT_RGB32, m_width, m_height, &surfaces[0], surfaces.size(),attribs, sizeof(attribs)/sizeof(attribs[0]));

	if (vaStatus != VA_STATUS_SUCCESS){
		fprintf(stderr,"VA SURFACE CREATE FAILED!\n");
		return;
	}

	return;
}

bool SurfaceCreate::PooledFrameAlloc(VADisplay display,uint32_t width,uint32_t height,uint32_t handle,int poolsize){
	std::vector<VASurfaceID> surfaces;
	surfaces.resize(m_poolsize);
	
	*m_display = display;
	m_width = width;
	m_height = height;
	m_handle = handle;
	m_poolsize = poolsize;
	
	bindToSurface(surfaces);
	std::deque<SharedPtr<VideoFrame> > buffers;
	for(size_t i=0;i<surfaces.size();i++){
		SharedPtr<VideoFrame> f(new VideoFrame);
		memset(f.get(),0,sizeof(VideoFrame));
		f->surface = (intptr_t)surfaces[i];
		f->fourcc = YAMI_FOURCC_RGBX;
		buffers.push_back(f);
	}
	m_pool.reset(new Video_Pool<VideoFrame>(buffers),SurfaceDestoryer(m_display,surfaces));
	return true;
}

SharedPtr<VideoFrame> SurfaceCreate::alloc(){
	return m_pool->alloc();
}
