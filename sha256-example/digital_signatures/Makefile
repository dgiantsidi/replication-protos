export CUR_PATH=$(realpath .)
BOTAN_INST_DIR=$(CUR_PATH)/botan
CUSTOM_OPENSSL=${CUR_PATH}/openssl


CXX=g++
CXX_FLAGS=-Wall -std=c++17 -Wno-error -Wall -g -O3 -mavx #-fsanitize=address
INCLUDES=-I/usr/local/include -I/usr/local/ssl/include -I./net  -I$(BOTAN_INST_DIR)/build/include -I../../crypto #-DOPENSSL_INCLUDE_DIR=/usr/include/openssl-1.0
LDS=-L/usr/local/lib -L/usr/local/ssl/lib64 -L$(BOTAN_INST_DIR)
LD_FLAGS=-lcrypto -lfmt #-lcityhash -lgflags #-fsanitize=address
LD_FLAGS_BENCH=-lcrypto -lfmt -lgflags #-lbotan-2#-fsanitize=address
CUSTOM_LDS=${LDS} #-L${CUSTOM_OPENSSL}
CUSTOM_INCLUDES=-I/usr/local/include -I/usr/local/ssl/include# -I${CUSTOM_OPENSSL}/include

SRCs = net/shared.cpp

target: server

all: clean main1 main2 main3 bench bench_custom server
main1: clean
	$(CXX) $(CXX_FLAGS) $(INCLUDES) main.cc $(LDS) $(LD_FLAGS) -o $@ 
main2: 
	$(CXX) $(CXX_FLAGS) $(INCLUDES) main2.cc $(LDS) $(LD_FLAGS) -o $@
main3: 
	$(CXX) $(CXX_FLAGS) $(INCLUDES) main3.cc $(LDS) $(LD_FLAGS) -o $@
bench_custom: clean
	$(CXX) $(CXX_FLAGS) $(CUSTOM_INCLUDES) bench.cc $(CUSTOM_LDS) $(LD_FLAGS_BENCH) -o $@ 
bench: clean
	$(CXX) $(CXX_FLAGS) $(INCLUDES) bench.cc rdtsc.cpp $(LDS) $(LD_FLAGS_BENCH) -o $@ 

server:  clean
	$(CXX) $(CXX_FLAGS) $(INCLUDES) attest_server.cc $(SRCs) $(LDS) $(LD_FLAGS) -o $@ 

client: 
	$(CXX) $(CXX_FLAGS) $(INCLUDES) client.cc $(SRCs) $(LDS) $(LD_FLAGS) -o $@ 

clean:
	rm -f main1 main2 main3 bench server client
