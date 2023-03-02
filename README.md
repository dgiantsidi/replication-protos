# replication-protos

- eRPC (`39b0fefb0cecc7845a3b8e82a3a2ed339a975891`)
- dpdk (`dpdk-stable-19.11.14`)

## UoE cluster
We pin port-1 to DPDK-driver (igb_uio)

e.g. `sudo python dpdk-devbind.py --bind=igb_uio 0000:01:00.1/enp1s0f1` (to-be-confirmed)


## Build eRPC

- build dpdk (get the DPDK 19.11.14 (LTS) from https://core.dpdk.org/download/ )
- comment out the app and kernel dirs from GNUMakefile
`make install T=x86_64-native-linuxapp-gcc DESTDIR=usr CXFLAGS="-Wno-error"`


- modify the CMakefile.txt
- add this compile option: `-msse4.1` and remove the `-Werror`
- include the local paths (todo)
- change this in file: eRPC/src/rpc_impl/rpc_rx.cc:line 22
```
if (unlikely(!pkthdr->check_magic())) {
          ERPC_WARN("Rpc %u: Received packet %s with bad magic number.Dropping.\n", rpc_id, pkthdr->to_string().c_str());
          continue;
      }
```
- `mkdir build && cd build && cmake .. -DPERF=OFF -DTRANSPORT=dpdk`
- `cp src/config.h ../src`
- `make`
-
- for the applications to be build also use the linking flag `-Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port`
