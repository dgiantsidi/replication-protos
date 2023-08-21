export CUR_PATH=$(shell $(pwd))
CXX=g++
CXX_FLAGS=-Wall -std=c++17 -Wno-error -Wall -g# -fsanitize=address
INCLUDES=-I/usr/local/include -I/usr/local/ssl/include -I./net
LDS=-L/usr/local/lib -L/usr/local/ssl/lib64
LD_FLAGS=-lcrypto -lfmt #-lcityhash -lgflags #-fsanitize=address
LD_FLAGS_BENCH=-lcrypto -lfmt -lgflags #-fsanitize=address

SRCs = net/shared.cpp

target: server

all: clean main1 main2 main3 bench server
main1: clean
	$(CXX) $(CXX_FLAGS) $(INCLUDES) main.cc $(LDS) $(LD_FLAGS) -o $@ 
main2: 
	$(CXX) $(CXX_FLAGS) $(INCLUDES) main2.cc $(LDS) $(LD_FLAGS) -o $@
main3: 
	$(CXX) $(CXX_FLAGS) $(INCLUDES) main3.cc $(LDS) $(LD_FLAGS) -o $@
bench: clean
	$(CXX) $(CXX_FLAGS) $(INCLUDES) bench.cc $(LDS) $(LD_FLAGS_BENCH) -o $@ 

server:  clean
	$(CXX) $(CXX_FLAGS) $(INCLUDES) attest_server.cc $(SRCs) $(LDS) $(LD_FLAGS) -o $@ 

client: 
	$(CXX) $(CXX_FLAGS) $(INCLUDES) client.cc $(SRCs) $(LDS) $(LD_FLAGS) -o $@ 

clean:
	rm -f main1 main2 main3 bench server client
