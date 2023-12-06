from os import read
import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import csv
import seaborn as sns
# Import statistics Library
import statistics

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("muted")


plt.rcParams["figure.figsize"] = (14, 8)
plt.rcParams.update({'font.size': 40})


x = ['Nat-lib', 'Nat', 'SGX', 'AMD-sev', 'TNIC']

x_axis = np.arange(len(x))
latency_batch_1 = [
        5.72190181,
46.1731345,
186.346,
130.32,
34.1685759
]

latency_batch_8 = [
1.202523915,
6.26416845,
23.7862561,
16.7879468,
4.7561
]

latency_batch_16 = [
0.899298785,
3.4295752,
12.18833715,
8.6865661,
2.17640115
]

fig, ax1 = plt.subplots()

ax1.plot(x_axis, latency_batch_1, linestyle='--', color=palette[9], marker="X", markersize=20, label="batch=1")
ax1.plot(x_axis, latency_batch_8, linestyle='--', color=palette[0], marker="8", markersize=20, label="batch=8")
ax1.plot(x_axis, latency_batch_16, linestyle='--', color=palette[3], marker=">", markersize=20, label="batch=16")

plt.yscale('log')
#ax1.set_title('Latency (us)')
#ax1.set_xlabel('packet size (B')
ax1.set_ylabel('Latency (us)')
ax1.set_xticks(x_axis)
ax1.set_xticklabels(x)
plt.tight_layout()
#plt.legend(loc='best', bbox_to_anchor=(0.55, 0.7, 0.4, 0.4), ncol=3, borderpad=0.01, columnspacing=0.05)
#plt.legend(loc='best', bbox_to_anchor=(0.87, 0.55), ncol=2, borderpad=0.01, columnspacing=0.05)
plt.legend(loc='best', ncol=1, borderpad=0.01, columnspacing=0.05)
#plt.legend(loc='best', bbox_to_anchor=(0.55, 0.2), ncol=1, borderpad=0.01, columnspacing=0.05)
plt.savefig('bft_pb_lat.pdf',dpi=400)
