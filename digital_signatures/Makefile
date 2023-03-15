export CUR_PATH=$(shell $(pwd))
CXX=g++
CXX_FLAGS=-Wall -std=c++17 -Wno-error -Wall -g -fsanitize=address
INCLUDES=-I/usr/local/include
LDS=-L/usr/local/lib
LD_FLAGS=-lssl -lcrypto -lfmt -lcityhash -fsanitize=address

target: main1

all: clean main1 main2 main3
main1: clean
	$(CXX) $(CXX_FLAGS) $(INCLUDES) main.cc $(LDS) $(LD_FLAGS) -o $@
main2: clean
	$(CXX) $(CXX_FLAGS) $(INCLUDES) main2.cc $(LDS) $(LD_FLAGS) -o $@
main3: clean
	$(CXX) $(CXX_FLAGS) $(INCLUDES) main3.cc $(LDS) $(LD_FLAGS) -o $@

clean:
	rm -f main1 main2 main3
