## Requirements
- openssl (get manually for scone)
- https://github.com/google/cityhash

## References
- http://hayageek.com/rsa-encryption-decryption-openssl-c/
-

## Build
- `Makefile` works for both native CPU and TEE (SCONE) but in SCONE you need to manually install/build some deps (fmt, openssl, cityhash, gflags)
- SCONE version has a bug so please copy the (manually installed) libraries into `/usr/lib/x86_64-linux-gnu/`
- `Hugepagesize=2097152 LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/:/usr/lib/gcc/x86_64-linux-gnu/7/:/usr/local/ssl/lib64 SCONE_VERSION=1 SCONE_LOG=7 SCONE_NO_FS_SHIELD=1 SCONE_NO_MMAP_ACCESS=1 SCONE_HEAP=3584M SCONE_LD_DEBUG=1 /opt/scone/lib/ld-scone-x86_64.so.1 ./bench --reps 1000`


