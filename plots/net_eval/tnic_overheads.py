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

overheads_integrity = [2, 3, 4]
overheads_BFT = [80, 90, 100]

envs = ['64B', '128B', '256B']
x = np.arange(len(envs))
low = int(min(overheads_integrity))
high = int(max(overheads_BFT))
plt.ylim([0, math.ceil(high+0.5*(high-low))])
width = 0.10
x2 = [r+width for r in x]
ax.bar(x, overheads_integrity, width, color= palette[3], hatch="o", edgecolor="black", label="TNIC w/ integrity")
ax.bar(x2, overheads_BFT, width, color= palette[6], hatch="*", edgecolor="black", label="TNIC w/ BFT")
ax.set_ylabel("Slowdown")
ax.set_xticks(x2)
ax.set_xticklabels(envs)
#ax.set_title("")
ax.legend(loc=1, bbox_to_anchor=(1.01, 1.04))
plt.savefig('tnic_overheads.pdf',dpi=400)
#plt.show()
