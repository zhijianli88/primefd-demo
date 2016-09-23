#Makefile

SRC = encodeyami.cpp yamivideocomp.cpp decodeinput.cpp \
	encodeInputCamera.cpp encodeinput.cpp encodeInputDecoder.cpp \
	vppinputoutput.cpp vppinputdecode.cpp vppoutputencode.cpp

INC_PATH = -I /usr/local/include -I /usr/local/include/libyami -I ../
INC_PATH += -I/opt/yami/vaapi/include/va -I/opt/yami/libyami/include/libyami -I/opt/yami/vaapi/include

LD_PATH = -L /usr/local/lib
LD_PATH += -L/opt/yami/libyami/lib/ -L/opt/yami/vaapi/lib/

LDFLAG = -lyami -lva -lva-drm

CXX = g++

CC = gcc

yamiencode:
	$(CXX) $(SRC) $(INC_PATH) $(LD_PATH) $(LDFLAG) -o $@

all: yamiencode

clean:
	rm -f yamiencode
	rm -f *.h264
