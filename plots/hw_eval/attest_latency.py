from os import read
import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import csv
import seaborn as sns

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("pastel")


plt.rcParams["figure.figsize"] = (21, 8)
plt.rcParams.update({'font.size': 60})

fig, ax = plt.subplots()

sha_lat_64 = [700, 1200, 800]
sha_lat_128 = [700, 1000, 400]
sha_lat_256 = [700, 800, 300]

envs = ['CPU', 'SGX', 'U280']
x = np.arange(len(envs))
low = int(min(sha_lat_64))
high = int(max(sha_lat_64))
plt.ylim([0, math.ceil(high+0.5*(high-low))])
width = 0.10
x2 = [r+width for r in x]
x3 = [r+width for r in x2]
ax.bar(x, sha_lat_64, width, color= palette[3], hatch="o", edgecolor="black", label="64B")
ax.bar(x2, sha_lat_128, width, color= palette[4], hatch="X", edgecolor="black", label="128B")
ax.bar(x3, sha_lat_256, width, color= palette[2], hatch="-", edgecolor="black", label="1K")
ax.set_ylabel("Latency (ns)")
ax.set_xticks(x2)
ax.set_xticklabels(envs)
#ax.set_title("")
ax.legend(loc=1, bbox_to_anchor=(1.01, 1.04))
plt.savefig('hw_eval_attest_latency.pdf',dpi=400)
#plt.show()
