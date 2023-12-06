from os import read
import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import csv
import seaborn as sns

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("pastel")


plt.rcParams["figure.figsize"] = (23, 8)
plt.rcParams.update({'font.size': 60})

fig, ax = plt.subplots()

# Native UoE    Native AMD  Intel SGX   AMD-sev     FPGA
lat_64 = [10.6865, 31, 45.3, 90, 23]
lat_128 = [10.7295, 31, 46.5, 90, 23]

envs = ['CPU-1', 'CPU-2', 'SGX', 'AMD-sev', 'FPGA']
x = np.arange(len(envs))
low = int(min(lat_64))
high = int(max(lat_64))
plt.ylim([0, math.ceil(high+0.5*(high-low))])
width = 0.30
x2 = [r+width for r in x]
x3 = [r+width for r in x2]
ax.bar(x, lat_64, width, color= palette[3], hatch="o", edgecolor="black", label="64B")
ax.bar(x2, lat_128, width, color= palette[4], hatch="X", edgecolor="black", label="128B")
#ax.bar(x3, sha_lat_256, width, color= palette[2], hatch="-", edgecolor="black", label="1K")
ax.set_ylabel("Latency (us)")
ax.set_xticks(x2)
ax.set_xticklabels(envs)
#ax.set_title("")
ax.legend(loc=1, bbox_to_anchor=(0.38, 1.04))
plt.savefig('hw_eval_attest_latency.pdf',dpi=400)
#plt.show()
