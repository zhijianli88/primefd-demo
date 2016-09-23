#include <deque>
#include <va/va_compat.h>
#include <VideoPostProcessHost.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#if __ENABLE_X11__
#include <X11/Xlib.h>
#endif
#include <va/va.h>
#include <va/va_drm.h>
#include "common/log.h"
#include "VideoEncoderInterface.h"
#include "VideoEncoderHost.h"
#include "encodeinput.h"

using namespace YamiMediaCodec; 


template <class T>
class Video_Pool
{
public:
	Video_Pool(std::deque<SharedPtr<T> >& buffers)
	{
		m_holder.swap(buffers);
		for (size_t i = 0; i < m_holder.size(); i++) {
			m_freed.push_back(m_holder[i].get());
		}

	};
	SharedPtr<T> alloc(){
		SharedPtr<T> ret;
		if(!m_freed.empty()){
			T* p = m_freed.front();
			m_freed.pop_front();
			ret.reset(p);
		}
		return ret;
	};
private:
	std::deque<T*> m_freed;
	std::deque<SharedPtr<T> > m_holder;
};


struct SurfaceDestoryer
{
private:
    SharedPtr<VADisplay> m_display;
    std::vector<VASurfaceID> m_surfaces;
public:
    SurfaceDestoryer(const SharedPtr<VADisplay>& display, std::vector<VASurfaceID>& surfaces)
        :m_display(display)
    {
        m_surfaces.swap(surfaces);
    }
    void operator()(Video_Pool<VideoFrame>* pool)
    {
        if (m_surfaces.size())
            vaDestroySurfaces(*m_display, &m_surfaces[0], m_surfaces.size());
        delete pool;
    }

};

class SurfaceCreate{
public:
	void bindToSurface(std::vector<VASurfaceID>& surfaces);
	bool PooledFrameAlloc(VADisplay display,uint32_t width,uint32_t height,uint32_t handle,int poolsize);  //maybe global variables
	SharedPtr<VideoFrame> alloc();
private:
	SharedPtr<VADisplay> m_display;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_handle;
	SharedPtr<Video_Pool<VideoFrame> > m_pool;
	int m_poolsize;
}; 


