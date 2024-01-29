from os import read
import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import csv
import seaborn as sns

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("pastel")


plt.rcParams["figure.figsize"] = (9, 4)
plt.rcParams.update({'font.size': 20})

fig, ax = plt.subplots()

# Native UoE    Native AMD  Intel SGX   AMD-sev     FPGA
throughput_batch_1 = [
175,
22,
5.366,
7.673,
29.266]

throughput_batch_8 = [
831.584,
159.638,
42.041,
59.566,
210.256]

throughput_batch_16 = [
1111.977,
291.581,
82.045,
115.120,
459.474
]

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

lat_128 = [10.7295, 31, 46.5, 90, 23]
plt.yscale("log")
envs = ['SSL-lib', 'SSL-server', 'SGX', 'AMD-sev', 'TNIC']
x = np.arange(len(envs))
low = int(min(throughput_batch_1))
high = int(max(throughput_batch_16))
plt.ylim([0, 100000])
width = 0.30
x2 = [r+width for r in x]
x3 = [r+width for r in x2]
ax.bar(x, throughput_batch_1, width, color= palette[3], hatch="o", edgecolor="black", label="batch=1")
ax.bar(x2, throughput_batch_8, width, color= palette[4], hatch="X", edgecolor="black", label="batch=8")
ax.bar(x3, throughput_batch_16, width, color= palette[8], hatch="-", edgecolor="black", label="batch=16")
#ax.bar(x3, sha_lat_256, width, color= palette[2], hatch="-", edgecolor="black", label="1K")

for k, y, p in zip(x, throughput_batch_1, latency_batch_1):
       s = " " + str(f'{p:.1f}')+ "us"
       plt.text(k, y, s, ha='center', va='bottom', fontsize=15, weight="bold", rotation=90)

for k, y, p in zip(x2, throughput_batch_8, latency_batch_8):
       s = " " + str(f'{p:.1f}')+ "us"
       plt.text(k, y, s, ha='center', va='bottom', fontsize=15, weight="bold", rotation=90)

for k, y, p in zip(x3, throughput_batch_16, latency_batch_16):
       s = " " + str(f'{p:.1f}')+ "us"
       plt.text(k, y, s, ha='center', va='bottom', fontsize=15, weight="bold", rotation=90)

ax.set_ylabel("kOp/s")
ax.set_xticks(x2)
ax.set_xticklabels(envs)
#ax.set_title("")
ax.legend(loc='best', ncol=3, borderpad=0.1, columnspacing=0.5)
plt.savefig('bft_pb.pdf',dpi=400)
#plt.show()
