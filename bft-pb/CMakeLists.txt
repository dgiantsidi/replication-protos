# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (bft-pb)

SET(CMAKE_CXX_FLAGS  "-DERPC_DPDK=true -g -O3")
SET(EXTRA_FLAGS "")
if (SANITIZE)
	SET(CMAKE_CXX_FLAGS  "-DERPC_DPDK=true -g -O3 -fsanitize=address")
	SET(EXTRA_FLAGS "-fsanitize=address")
endif()

set(ATTEST_KERNEL "")

if (FPGA)
	set(ATTEST_KERNEL "FPGA")
	add_compile_options("-DFPGA")
endif()
if (AMD)
	set(ATTEST_KERNEL "AMDSEV")
	add_compile_options("-DAMD")
endif()
if (SGX)
	set(ATTEST_KERNEL "SGX")
	add_compile_options("-DSGX")
endif()

link_directories($ENV{HOME}/workspace/replication-protos/eRPC/build)
link_directories(/usr/local/lib64)
link_directories($ENV{HOME}/workspace/replication-protos/dpdk-stable-19.11.14/usr/lib)
include_directories($ENV{HOME}/workspace/replication-protos/eRPC/src)
include_directories($ENV{HOME}/workspace/replication-protos/dpdk-stable-19.11.14/x86_64-native-linuxapp-gcc/include)
include_directories($ENV{HOME}/workspace/replication-protos/)

find_library(FMT_lib fmt)
if(NOT FMT_lib)
    message(STATUS "fmt library not found")
endif()

#add_executable(pb main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)
add_executable(pb_donna_${ATTEST_KERNEL} main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)
add_executable(pb_martha_${ATTEST_KERNEL} main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)
add_executable(pb_amy_${ATTEST_KERNEL} main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)



#target_link_libraries(pb -lpthread -lfmt -lcrypto -lnuma -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
#target_link_libraries(pb_donna -lpthread -lfmt -lcrypto -lnuma -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
#target_link_libraries(pb_martha -lpthread -lfmt -lnuma -lcrypto -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
#target_link_libraries(pb_amy -lpthread -lfmt -lnuma -lcrypto -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)

#target_link_libraries(pb -lpthread -lfmt -lcrypto -lnuma -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
target_link_libraries(pb_donna_${ATTEST_KERNEL} -lpthread -lfmt -lcrypto -lnuma -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port ${EXTRA_FLAGS})
target_link_libraries(pb_martha_${ATTEST_KERNEL}  -lpthread -lfmt -lnuma -lcrypto -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port ${EXTRA_FLAGS})
target_link_libraries(pb_amy_${ATTEST_KERNEL}  -lpthread -lfmt -lnuma -lcrypto -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port ${EXTRA_FLAGS})
