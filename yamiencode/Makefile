#Makefile

SRC = encodeyami.cpp yamivideocomp.cpp decodeinput.cpp \
	encodeInputCamera.cpp encodeinput.cpp encodeInputDecoder.cpp

INC_PATH = -I /usr/local/include -I /usr/local/include/libyami -I ../

LD_PATH = -L /usr/local/lib

LDFLAG = -lyami -lva -lva-drm

CXX = g++

CC = gcc

yamiencode:
	$(CXX) $(SRC) $(INC_PATH) $(LD_PATH) $(LDFLAG) -o $@

all: yamiencode

clean:
	rm -f yamiencode
	rm -f *.h264
