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
plt.rcParams.update({'font.size': 40})

fig, ax = plt.subplots()

overheads_BFT = [
        2.957919818,
3.536586799,
4.47236202,
6.128233355,
9.535809944,
14.04718478,
7.947705728,
18.39954202,
20.26988941
]

speedup_hw = [
        5.414972144,
4.589759135,
3.59385579,
0,
0,
0,
0,
0,
0]

envs = ['128', '256', '512', '1K', '2K', '4K', '8K', '16K', '32K']

x = np.arange(len(envs))
low = int(min(speedup_hw))
high = int(max(overheads_BFT))
plt.ylim([0, math.ceil(high+0.5*(high-low))])
width = 0.30
x2 = [r+width for r in x]
ax.bar(x, overheads_BFT, width, color= palette[3], hatch="o", edgecolor="black", label="Overheads due to BFT")
ax.bar(x2, speedup_hw, width, color= palette[6], hatch="*", edgecolor="black", label="Speedup due to TNIC")
#ax.set_ylabel("Slowdown")
ax.set_xticks(x2)
ax.set_xlabel("packet size (B)")
ax.set_xticklabels(envs)
#ax.set_title("")
plt.tight_layout()

ax.legend(loc="upper right", bbox_to_anchor=(1.01, 1.04))
plt.savefig('tnic_overheads.pdf',dpi=400)
#plt.show()
