from os import read
import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import csv
import seaborn as sns

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("pastel")

COLUMN_FIGSIZE = (6.5, 3.4)
plt.rcParams["figure.figsize"] = (7.4, 3)
#fig = plt.figure(figsize=COLUMN_FIGSIZE)
plt.rcParams.update({'font.size': 20})

fig, ax = plt.subplots()

# Native UoE    Native AMD  Intel SGX   AMD-sev     FPGA
lat_64 = [10.6865, 31, 45.3, 90, 23]
lat_128 = [10.7295, 31, 46.5, 90, 23]

envs = ['Intel-x86', 'AMD', 'SGX', 'AMD-sev', 'TNIC']
x = np.arange(len(envs))
low = int(min(lat_64))
high = int(max(lat_64))
plt.ylim([0, 130])
width = 0.30
x2 = [r+width for r in x]
x3 = [r-width/2 for r in x2]
ax.bar(x, lat_64, width, color= palette[2], hatch="o ", edgecolor="black", label="64B")
ax.bar(x2, lat_128, width, color= palette[4], hatch="X", edgecolor="black", label="128B")


 #   plt.text(rect.get_x() + rect.get_width() / 2.0, height, f'{height:.0f}', ha='center', va='bottom')
for x, y, p in zip(x, lat_64, lat_64):
    #plt.text(x, y, p, f{p:.0f}, ha='center', va='bottom')
    s = " " + f'{p:.0f}' + "us"
    plt.text(x, y, s, ha='center', va='bottom', fontsize=15, weight="bold", rotation=90)

for x, y, p in zip(x2, lat_128, lat_128):
    #plt.text(x, y, p, f{p:.0f}, ha='center', va='bottom')
    s = " " + f'{p:.0f}' + "us"
    plt.text(x, y, s, ha='center', va='bottom', fontsize=15, weight="bold", rotation=90)

#ax.bar(x3, sha_lat_256, width, color= palette[2], hatch="-", edgecolor="black", label="1K")
ax.set_ylabel("Latency (us)")
ax.set_xticks(x3)
ax.set_xticklabels(envs)
#ax.set_title("")
ax.legend(loc=1, bbox_to_anchor=(0.38, 0.98))
plt.savefig('hw_eval_attest_latency.pdf',dpi=400)

#plt.show()
