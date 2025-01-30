The project contains the code for the networking evaluation. 

## Requirements

- OpenSSL. Manual installation needed for `SCONE`. We used a non publicly available image and the system is not tested in the publicly available image.
- Botan-2: Update for 2.10.0 release (commit 3a459487df5de4cb411efdda0010e617d6903284 (HEAD, tag: 2.10.0))

## Build

- `Makefile` works for both native CPU and TEE (`SCONE`) 
- For `SCONE` you need to manually install and build from scratch some deps (fmt, openssl, cityhash, gflags)
  
- When running in `SCONE` please copy the (manually installed) libraries into `/usr/lib/x86_64-linux-gnu/`

Run in `SCONE` with

```sh
Hugepagesize=2097152 LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/:/usr/lib/gcc/x86_64-linux-gnu/7/:/usr/local/ssl/lib64 SCONE_VERSION=1 SCONE_LOG=7 SCONE_NO_FS_SHIELD=1 SCONE_NO_MMAP_ACCESS=1 SCONE_HEAP=3584M SCONE_LD_DEBUG=1 /opt/scone/lib/ld-scone-x86_64.so.1 <binary> <input>`
```

- For the AMD-SEV we followed the instructions here to setup the VMs and the networking `https://github.com/TUM-DSE/CVM_eval.git`.


