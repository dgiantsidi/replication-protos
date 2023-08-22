from matplotlib.cbook import get_sample_data
import matplotlib.pyplot as plt
from os import read
import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import csv
import seaborn as sns


import numpy as np

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("dark")


plt.rcParams["figure.figsize"] = (18, 6)
plt.rcParams.update({'font.size': 30})



x = ['64', '128', '256', '512', '1K', '2K', '4K']
x_axis = np.arange(len(x))
tnic_tps = [2.41, 10, 40, 50, 100, 200, 1000]
tnic_latency = [5, 5.1, 5.6, 7.8, 8, 8, 16]

eRPC_sgx_tps = [3.41, 10, 10, 20, 150, 200, 500]
eRPC_sgx_latency = [7, 8.1, 8.6, 8.8, 9, 10, 20]

eRPC_nat_tps = [4.41, 10, 10, 20, 150, 200, 500]
eRPC_nat_latency = [7, 8.1, 8.6, 8.8, 9, 10, 20]

tcp_nat_tps = [5.41, 10, 10, 20, 150, 200, 500]
tcp_nat_latency = [7, 8.1, 8.6, 8.8, 9, 10, 20]

fig, (ax1, ax2) = plt.subplots(1, 2, sharey=False, figsize=(20, 8))

ax1.plot(x_axis, tnic_tps, linestyle='--', color=palette[9], marker="X", markersize=20, label="TNIC")
ax1.plot(x_axis, eRPC_sgx_tps, linestyle='--', color=palette[0], marker="8", markersize=20, label="eRPC w/ SGX")
ax1.plot(x_axis, eRPC_nat_tps, linestyle='--', color=palette[2], marker="o", markersize=20, label="eRPC w/o SGX")
ax1.plot(x_axis, tcp_nat_tps, linestyle='--', color=palette[3], marker=">", markersize=20, label="TCP w/o SGX")

ax1.set_title('Throughput')
ax1.set_xlabel('packet size (B)')
ax1.set_ylabel('MB/s')
ax1.set_xticks(x_axis)
ax1.set_xticklabels(x)


ax2.plot(x_axis, tnic_latency, linestyle='--', color=palette[9], marker="X", markersize=20, label="TNIC")
ax2.plot(x_axis, eRPC_sgx_latency, linestyle='--', color=palette[0], marker="8", markersize=20, label="eRPC w/ SGX")
ax2.plot(x_axis, eRPC_nat_latency, linestyle='--', color=palette[2], marker="o", markersize=20, label="eRPC w/o SGX")
ax2.plot(x_axis, tcp_nat_latency, linestyle='--', color=palette[3], marker=">", markersize=20, label="TCP w/o SGX")

ax2.set_xticks(x_axis)
ax2.set_xticklabels(x)
ax2.set_xlabel('packet size (B)')
ax2.set_ylabel('us')
ax2.set_title('Latency')

#fig.suptitle('Different types of oscillations', fontsize=16)
plt.legend()
#plt.show()
plt.savefig('rpc_lat_throughput.pdf',dpi=400)
