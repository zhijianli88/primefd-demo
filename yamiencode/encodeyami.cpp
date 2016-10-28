#include <stdio.h>
#include <string.h>
#include "yamivideocomp.h"
#include "vppinputoutput.h"
#include "dmabuf-alone.h"

//#include "dmabuf-alone.c"

using namespace YamiMediaCodec;

static int videoWidth;
static int videoHeight;
static int fps = 30;
static int intraPeriod = 30;
static int ipPeriod = 1;
static int bitRate = 0;
static int initQp = 26;
static VideoRateControl rcMode = RATE_CONTROL_CQP;
static int numRefFrames = 1;
static int idrInterval = 0;
static long unsigned int dmabuf[1024];
static int poolsize = 1;
struct drm_i915_gem_vgtbuffer vgtbuffer;
FILE *m_fp = stdout;
 
    bool frame_write(SharedPtr<VideoFrame>& frame, VADisplay &m_vaDisplay, const char *name)
    {
        VASurfaceID surface = (VASurfaceID)frame->surface;
        VAImage image;
	m_fp = fopen(name, "wb");

        uint8_t* buf = mapSurfaceToImage(m_vaDisplay, surface, image);
	printf("w %d, h %d, p0 %d, p1 %d\n", image.width, image.height, image.pitches[0], image.pitches[1]);
        if (buf != NULL) {
            for (int i = 0; i < image.height; i++) {
                fwrite(buf + i * image.pitches[0], 1, image.width, m_fp);
            }
            buf += image.offsets[1];
            for (int i = 0; i < image.height / 2; i++) {
                fwrite(buf + i * image.pitches[1], 1, image.width, m_fp);
            }
            unmapImage(m_vaDisplay, image);
	    fclose(m_fp);
            return true;
        }
        else {
	    fclose(m_fp);
            return false;
        }
    }

SharedPtr<FrameAllocator> createAllocator(const SharedPtr<VppOutput>& output, const SharedPtr<VADisplay>& display)
{
	uint32_t fourcc;
	int width, height;
	SharedPtr<FrameAllocator> allocator(new PooledFrameAllocator(display, 5));
	if (!output->getFormat(fourcc, width, height)
				|| !allocator->setFormat(fourcc, width,height)) {
		allocator.reset();
		ERROR("get Format failed");
	}
	printf("fourcc %x, w %d, h %d\n", fourcc, width, height);
	return allocator;
}

static void dump_encVideoParams(VideoParamsCommon *p)
{
    printf("resolution(%d * %d)\n", p->resolution.width, p->resolution.height);
    printf("frameRate(%d, %d)\n", p->frameRate.frameRateDenom, p->frameRate.frameRateNum);
    printf("intraPeriod %d, ipPeriod %d, rcMode %d, numRefFrames %d\n",
	    p->intraPeriod, p->ipPeriod, p->rcMode, p->numRefFrames);
    printf("rcParams(%d, %d)\n", p->rcParams.bitRate, p->rcParams.initQP);
}

int main(int argc,char** argv){
	IVideoEncoder *encoder = NULL;
	EncodeOutput* output;
	Encode_Status status;
	VideoEncOutputBuffer outputBuffer;
	uint32_t maxOutSize = 0;

	if(argc<2){
		printf("error: the number of input parameters must be 1:\nvmid\n");
		printf("usage: yamiencode <vmid>\n");
		return -1;
	}

	//set parameters
	int vmid = atoi(argv[1]);  //vmid handler
	char codec[] = "AVC";
	char outputfile[] = "test.h264";

	//set display parameters
	int fd = open("/dev/dri/card0",O_RDWR);
	if(fd == -1){
		fprintf(stderr,"Failed to open card0 \n");
		return -1;
	}

	//create AVC encoder
	encoder = createVideoEncoder(YAMI_MIME_H264);
	assert(encoder != NULL);

	dmabuf[0] = test_dmabuf(fd, vmid, &vgtbuffer);
	for (int i = 1; i < poolsize; i++) {
		dmabuf[i] = dmabuf[0];
		fprintf(stderr, "dmabuf[%d] fd is %ld\n", i, dmabuf[i]);
	}

	videoWidth = vgtbuffer.width;
	videoHeight = vgtbuffer.height;

	//create outputfile
	output = EncodeOutput::create(outputfile,videoWidth,videoHeight,codec);
	if(!output){
		fprintf(stderr,"fail to init output stream!\n");
		return -1;
	}

	NativeDisplay nativeDisplay;
	nativeDisplay.type = NATIVE_DISPLAY_DRM;
	nativeDisplay.handle = -1;
	encoder->setNativeDisplay(&nativeDisplay);

	//configure encoding parameters
	VideoParamsCommon encVideoParams;
	encVideoParams.size = sizeof(VideoParamsCommon);
	encoder->getParameters(VideoParamsTypeCommon,&encVideoParams);

	encVideoParams.resolution.width = vgtbuffer.width;
	encVideoParams.resolution.height = vgtbuffer.height;
	encVideoParams.frameRate.frameRateDenom = 1;
	encVideoParams.frameRate.frameRateNum = fps;
	encVideoParams.intraPeriod = intraPeriod;
	encVideoParams.ipPeriod = ipPeriod;
	encVideoParams.rcParams.bitRate = bitRate;
	encVideoParams.rcParams.initQP = initQp;
	encVideoParams.rcMode = rcMode;
	encVideoParams.numRefFrames = numRefFrames;

	encVideoParams.size = sizeof(VideoParamsCommon);
	encoder->setParameters(VideoParamsTypeCommon,&encVideoParams);

	dump_encVideoParams(&encVideoParams);

	//configure AVC encoding parameters
	VideoParamsAVC encVideoParamsAVC;
	encVideoParamsAVC.size = sizeof(VideoParamsAVC);
	encoder->getParameters(VideoParamsTypeAVC,&encVideoParamsAVC);
	encVideoParamsAVC.idrInterval = idrInterval;
	encVideoParamsAVC.size = sizeof(VideoParamsAVC);
	encoder->setParameters(VideoParamsTypeAVC,&encVideoParamsAVC);

	VideoConfigAVCStreamFormat streamFormat;
	streamFormat.size = sizeof(VideoConfigAVCStreamFormat);
	streamFormat.streamFormat = AVC_STREAM_FORMAT_ANNEXB;
	encoder->setParameters(VideoConfigTypeAVCStreamFormat, &streamFormat);

	status = encoder->start();
	assert(status == ENCODE_SUCCESS);

	//init output buffer
	encoder->getMaxOutSize(&maxOutSize);

	if (!createOutputBuffer(&outputBuffer, maxOutSize)) {
		fprintf (stderr, "fail to create output\n");
		delete output;
		return -1;
	}

	VADisplay vaDisplay = vaGetDisplayDRM(fd);
	int major, minor;
	VAStatus va_status;
	va_status = vaInitialize(vaDisplay,&major,&minor);
	if(va_status != VA_STATUS_SUCCESS){
		fprintf(stderr,"init va failed status = %d",status);
		return -1;
	}

	//get the surface
	SurfaceCreate* surface = new SurfaceCreate;
	if (!surface->PooledFrameAlloc(&vaDisplay,videoWidth,videoHeight,dmabuf,poolsize)) {
	    fprintf(stderr, "surface->PooledFrameAlloc failed\n");
	    return false;
	}
    
	//create vpp
	SharedPtr<IVideoPostProcess> vpp;
	vpp.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
	if (vpp->setNativeDisplay(nativeDisplay) != YAMI_SUCCESS)
	    return false;

	//init vpp
	SharedPtr<VADisplay> display;
	display.reset(new VADisplay(vaDisplay));
    
	char vppoutput[] = "trans.yuv";
	SharedPtr<VppOutput> output1 = VppOutput::create(vppoutput,VA_FOURCC_RGBX,videoWidth,videoHeight);
	if(!output1){
		printf("create output1 failed!\n");
		return -1;
	}
	SharedPtr<FrameAllocator> allocator = createAllocator(output1,display);

	//encode frame
    for(int i=0;i<poolsize;i++){
		SharedPtr<VideoFrame> frame = surface->alloc();
		//vpp process
		SharedPtr<VideoFrame> dest = allocator->alloc();
		status = vpp->process(frame,dest);
		if(status != YAMI_SUCCESS){
			printf("failed to scale! %d",status);
			break;
		}

		frame_write(frame, vaDisplay, "frame.rgbx");
	        printf("\n\n\n");
		frame_write(dest, vaDisplay, "frame.yuv");
	        printf("\n\n\n");
		if(dest){
			status = encoder->encode(frame);
			assert(status == ENCODE_SUCCESS);
		}
		else{
			break;
		}

		//get the output buffer
		do{
			status = encoder->getOutput(&outputBuffer,false);
			if(status == ENCODE_SUCCESS
			  && output->write(outputBuffer.data,outputBuffer.dataSize)){
				printf("output data size:%d\n",outputBuffer.dataSize);
				for (int j = 0; j < 240; j++)
					output->write(outputBuffer.data,outputBuffer.dataSize);
			}
		}while(status != ENCODE_BUFFER_NO_MORE);
	}

#if 0
	// drain the output buffer
	do{
		status = encoder->getOutput(&outputBuffer,true);
		if(status == ENCODE_SUCCESS){
			output->write(outputBuffer.data,outputBuffer.dataSize);
		}
	}while(status != ENCODE_BUFFER_NO_MORE);
#endif
	encoder->stop();
	releaseVideoEncoder(encoder);
	free(outputBuffer.data);
	delete output;
	fprintf(stderr,"encode done!\n");
	return 0;
}
