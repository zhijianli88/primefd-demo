#include <stdio.h>
#include <string.h>
#include "yamivideocomp.h"

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
static int dmabuf;
static int poolsize = 5;

int main(int argc,char** argv){
	IVideoEncoder *encoder = NULL;
	EncodeOutput* output;
	Encode_Status status;
	VideoEncOutputBuffer outputBuffer;
	uint32_t maxOutSize = 0;

	if(argc<4){
		printf("error: the number of input parameters must be 3:\nvideowidth\nwideoheight\ndmabuf_handler\n");
		printf("usage: yamiencode <videowidth> <videoheight> <dmabuf_handler>\n");
		return -1;
	}

	//set parameters
	videoWidth = atoi(argv[1]);  //width
	videoHeight = atoi(argv[2]); //height
	dmabuf = atoi(argv[3]);  //dmabuf handler
	char codec[] = "AVC";
	char outputfile[] = "test.h264";
    
	//create outputfile
	output = EncodeOutput::create(outputfile,videoWidth,videoHeight,codec);
	if(!output){
		fprintf(stderr,"fail to init output stream!\n");
		return -1;
	}
	
	//create AVC encoder
	encoder = createVideoEncoder(YAMI_MIME_H264);	
	assert(encoder != NULL);

	//set display parameters
	int fd = open("/dev/dri/card0",O_RDWR);
	if(fd == -1){
		fprintf(stderr,"Failed to open card0 \n");
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
	NativeDisplay nativeDisplay;
	nativeDisplay.type = NATIVE_DISPLAY_DRM;
	nativeDisplay.handle = (intptr_t)vaDisplay;
	encoder->setNativeDisplay(&nativeDisplay);

	//configure encoding parameters
	VideoParamsCommon encVideoParams;
	encVideoParams.size = sizeof(VideoParamsCommon);
	encoder->getParameters(VideoParamsTypeCommon,&encVideoParams);

	encVideoParams.resolution.width = videoWidth;
	encVideoParams.resolution.height = videoHeight;
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
	
	//get the surface
	SurfaceCreate* surface = new SurfaceCreate;
	surface->PooledFrameAlloc(vaDisplay,videoWidth,videoHeight,dmabuf,poolsize);  

	//encode frame
    for(int i=0;i<poolsize;i++){
		SharedPtr<VideoFrame> frame = surface->alloc();
		//vpp?

		if(frame){
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
				printf("output data size:%d",outputBuffer.dataSize);
			}
		}while(status != ENCODE_BUFFER_NO_MORE);
	}
	
	// drain the output buffer
	do{
		status = encoder->getOutput(&outputBuffer,true);
		if(status == ENCODE_SUCCESS){
			output->write(outputBuffer.data,outputBuffer.dataSize);
		}
	}while(status != ENCODE_BUFFER_NO_MORE);

	encoder->stop();
	releaseVideoEncoder(encoder);
	free(outputBuffer.data);
	delete output;
	fprintf(stderr,"encode done!\n");
	return 0;
}
