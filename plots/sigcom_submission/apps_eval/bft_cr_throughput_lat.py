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


plt.rcParams["figure.figsize"] = (30, 10)
plt.rcParams.update({'font.size': 40})

x = ['Nat-lib', 'Nat', 'SGX', 'AMD-sev', 'TNIC']
x_axis = np.arange(len(x))

throughput = [
        198.576,
33.807,
8.459,
12.187,
42.606
]

latency = [
        5.035847,
29.5794645,
118.207697,
82.0509625,
23.470503
]

fig, (ax1, ax2) = plt.subplots(1, 2, sharey=False, figsize=(20, 8))

ax1.plot(x_axis, throughput, linestyle='--', color=palette[9], marker="X", markersize=20, label="TNIC")
#ax1.plot(x_axis, eRPC_sgx_tps, linestyle='--', color=palette[0], marker="8", markersize=20, label="eRPC w/ SGX")
#ax1.plot(x_axis, eRPC_nat_tps, linestyle='--', color=palette[2], marker="o", markersize=20, label="eRPC w/o SGX")
#ax1.plot(x_axis, tcp_nat_tps, linestyle='--', color=palette[3], marker=">", markersize=20, label="TCP w/o SGX")

#ax1.set_title('kOp/s')
#ax1.set_xlabel('packet size (B)')
ax1.set_ylabel('kOp/s')
ax1.set_xticks(x_axis)
ax1.set_xticklabels(x, rotation=90, ha='right')
#ax1.set_xticklabels(x)


ax2.plot(x_axis, latency, linestyle='--', color=palette[9], marker="X", markersize=20, label="TNIC")
#ax2.plot(x_axis, eRPC_sgx_latency, linestyle='--', color=palette[0], marker="8", markersize=20, label="eRPC w/ SGX")
#ax2.plot(x_axis, eRPC_nat_latency, linestyle='--', color=palette[2], marker="o", markersize=20, label="eRPC w/o SGX")
#ax2.plot(x_axis, tcp_nat_latency, linestyle='--', color=palette[3], marker=">", markersize=20, label="TCP w/o SGX")

ax2.set_xticks(x_axis)
ax2.set_xticklabels(x, rotation=90, ha='right')
#ax2.set_xlabel('packet size (B)')
ax2.set_ylabel('Latency (us)')
#ax2.set_title('Latency')

#fig.suptitle('Different types of oscillations', fontsize=16)
#plt.legend()
#plt.show()
plt.tight_layout()

plt.savefig('bftcr_lat_throughput.pdf',dpi=400)
