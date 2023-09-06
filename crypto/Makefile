export CUR_PATH=$(realpath .)

ifeq ($(SANITIZER),ON)
CXX_FLAGS ?= -fsanitize=address
LD_FLAGS_BENCH ?= -fsanitize=address
endif

CXX=g++
CXX_FLAGS+=-Wall -std=c++17 -Wno-error -Wall -g -O3 -mavx #-fsanitize=address
INCLUDES=-I/usr/local/include -I/usr/local/ssl/include -I./net  
LDS=-L/usr/local/lib -L/usr/local/ssl/lib64
LD_FLAGS=-lcrypto -lfmt #-lcityhash -lgflags #-fsanitize=address
LD_FLAGS_BENCH+=-lcrypto -lfmt -lgflags #-fsanitize=address



target: bench

all: bench
bench: clean
	$(CXX) $(CXX_FLAGS) $(INCLUDES) bench.cc $(LDS) $(LD_FLAGS_BENCH) -o $@ 


clean:
	rm -f bench

