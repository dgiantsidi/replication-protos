export CUR_PATH=$(shell $(pwd))
CXX=g++
CXX_FLAGS=-Wall -std=c++17 -Wno-error -Wall
INCLUDES=-I/usr/local/include
LDS=-L/usr/local/lib
LD_FLAGS=-lssl -lcrypto -lfmt -lcityhash

target: all

all:
	$(CXX) $(CXX_FLAGS) $(INCLUDES) main.cc $(LDS) $(LD_FLAGS)
