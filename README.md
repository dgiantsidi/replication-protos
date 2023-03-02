# replication-protos

- eRPC (`39b0fefb0cecc7845a3b8e82a3a2ed339a975891`)
- dpdk (`dpdk-stable-19.11.14`)

# UoE cluster
We pin port-1 to DPDK-driver (igb_uio)
`sudo python dpdk-devbind.py --bind=igb_uio 0000:01:00.1/enp1s0f1` (to-be-confirmed)

