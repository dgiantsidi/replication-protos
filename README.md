# replication-protos

- eRPC (`39b0fefb0cecc7845a3b8e82a3a2ed339a975891`)
- dpdk (`dpdk-stable-19.11.14`)

## UoE cluster
We pin port-1 to DPDK-driver (igb_uio). Note, it should only be port-1!

e.g., `sudo python dpdk-devbind.py --bind=igb_uio 0000:01:00.1/enp1s0f1` (confirmed)


### Build dpdk 
- We get the DPDK 19.11.14 (LTS) from https://core.dpdk.org/download/ 
- Comment out the app and kernel dirs from GNUMakefile and execute
-
	`make install T=x86_64-native-linuxapp-gcc DESTDIR=usr CXFLAGS="-Wno-error"`

### Build eRPC library
- Modify the CMakefile.txt by adding this compile option: `-msse4.1` and removing the `-Werror`
- Include the local paths with dpdk-installation, etc. to the CMakefile.txt (todo)
- Change this in file: `eRPC/src/rpc_impl/rpc_rx.cc:line 22`
	```
	if (unlikely(!pkthdr->check_magic())) {
	 ERPC_WARN("Rpc %u: Received packet %s with bad magic number.Dropping.\n", 
 		rpc_id, pkthdr->to_string().c_str());
	 continue;
	}	
	```

- Then we build the library `mkdir build && cd build && cmake .. -DPERF=OFF -DTRANSPORT=dpdk`
- `cp src/config.h ../src`
- `make` (in the build dir)

- SOS: To properly link the applications the following linking flags need to be added

    `-Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port`



## doctor-config TUM
We pin E810-port to DPDK-driver (igb_uio or vfio-pci).
- Fetch the ice driver firmware from
	`git clone git://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git`
- Patch the ice driver in dpdk src code (`dpdk/drivers/net/ice/ice_ethdev.c`) by replacing the line

	`#define ICE_DFLT_PKG_FILE "/lib/firmware/intel/ice/ddp/ice.pkg"`
	
	with 

	`#define ICE_DFLT_PKG_FILE "${linux-firmware}/intel/ice/ddp/ice-1.3.26.0.pkg"`
