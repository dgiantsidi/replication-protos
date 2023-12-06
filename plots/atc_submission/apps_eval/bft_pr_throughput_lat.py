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

throughput_no_audit = [
        109.725,
14.325,
3.569,
5.101,
19.275,
]

latency_no_audit = [
        9.113682,
69.804,
280.17419,
196.02,
51.88
]


throughput_audit = [
        81739.97598,
10732.07221,
2676.802827,
3823.865125,
14464.67957
]

latency_audit = [
        12.233916,
93.17865,
373.58,
261.5155,
69.13392
        ]

fig, (ax1, ax2) = plt.subplots(1, 2, sharey=False, figsize=(20, 8))

plt.yscale('log')
ax1.plot(x_axis, throughput_no_audit, linestyle='--', color=palette[2], marker="s", markersize=20, label="w/o audit")
ax1.plot(x_axis, throughput_audit, linestyle='--', color=palette[0], marker=">", markersize=20, label="w/ audit")
#ax1.plot(x_axis, eRPC_sgx_tps, linestyle='--', color=palette[0], marker="8", markersize=20, label="eRPC w/ SGX")
#ax1.plot(x_axis, eRPC_nat_tps, linestyle='--', color=palette[2], marker="o", markersize=20, label="eRPC w/o SGX")
#ax1.plot(x_axis, tcp_nat_tps, linestyle='--', color=palette[3], marker=">", markersize=20, label="TCP w/o SGX")

#ax1.set_title('kOp/s')
#ax1.set_xlabel('packet size (B)')
ax1.set_ylabel('kOp/s')
ax1.set_xticks(x_axis)
ax1.set_xticklabels(x, rotation=90, ha='right')
#ax1.set_xticklabels(x)
ax1.set_yscale('log')

ax2.plot(x_axis, latency_no_audit, linestyle='--', color=palette[2], marker="s", markersize=20, label="w/o audit")
ax2.plot(x_axis, latency_audit, linestyle='--', color=palette[0], marker=">", markersize=20, label="w/ audit")
#ax2.plot(x_axis, eRPC_sgx_latency, linestyle='--', color=palette[0], marker="8", markersize=20, label="eRPC w/ SGX")
#ax2.plot(x_axis, eRPC_nat_latency, linestyle='--', color=palette[2], marker="o", markersize=20, label="eRPC w/o SGX")
#ax2.plot(x_axis, tcp_nat_latency, linestyle='--', color=palette[3], marker=">", markersize=20, label="TCP w/o SGX")
ax2.set_xticks(x_axis)
ax2.set_xticklabels(x, rotation=90, ha='right')
#ax2.set_xlabel('packet size (B)')
ax2.set_ylabel('Latency (us)')
#ax2.set_title('Latency')

#fig.suptitle('Different types of oscillations', fontsize=16)
plt.legend(loc='best')
#plt.show()
plt.tight_layout()

plt.savefig('bftpr_lat_throughput.pdf',dpi=400)
