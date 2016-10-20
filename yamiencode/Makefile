#Makefile

SRC = encodeyami.cpp yamivideocomp.cpp decodeinput.cpp \
	encodeInputCamera.cpp encodeinput.cpp encodeInputDecoder.cpp \
	vppinputoutput.cpp vppinputdecode.cpp vppoutputencode.cpp dmabuf-alone.c

INC_PATH = -I. -I /usr/local/include -I /usr/local/include/libyami -I ../ -I/usr/include/drm/
INC_PATH += -I/opt/yami/vaapi/include/va -I/opt/yami/libyami/include/libyami -I/opt/yami/vaapi/include

LD_PATH = -L /usr/local/lib
LD_PATH += -L/opt/yami/libyami/lib/ -L/opt/yami/vaapi/lib/

LDFLAG = -lyami -lva -lva-drm -ldrm -g

CXX = g++

CC = gcc

yamiencode: $(SRC)
#	gcc -o dmabuf-alone.o -c dmabuf-alone.c -I /usr/include/drm/ #-ldrm_intel -ldrm
	$(CXX) $(SRC) $(INC_PATH) $(LD_PATH) $(LDFLAG) -o $@

all: yamiencode

clean:
	rm -f yamiencode
	rm -f *.h264
