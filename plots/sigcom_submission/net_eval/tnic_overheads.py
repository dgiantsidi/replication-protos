from os import read
import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import csv
import seaborn as sns

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("pastel")


plt.rcParams["figure.figsize"] = (5.5, 2.7)
plt.rcParams.update({'font.size': 17})

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
plt.ylim([0, math.ceil(high+0.2*(high-low))])
width = 0.30
x2 = [r+width for r in x]
ax.bar(x, overheads_BFT, width, color= palette[3], edgecolor="black")
#ax.bar(x2, speedup_hw, width, color= palette[6], hatch="*", edgecolor="black", label="Speedup due to TNIC")

for k, y, p in zip(x, overheads_BFT, overheads_BFT):
     s = "-" + str(f'{p:.0f}')+ "x"
     plt.text(k, y, s, ha='center', va='bottom', fontsize=15, weight="bold", c='red')

#plt.text(4.1, math.ceil(high+0.4*(high-low)), "Lower is better ↓", ha='center', va='center', fontsize=15, weight='bold', c='blue')
ax.set_ylabel("Slowdown")
ax.set_xticks(x)
ax.set_xlabel("packet size (B)")
ax.set_xticklabels(envs)
#ax.set_title("")
plt.tight_layout()

#ax.legend(loc="upper right", bbox_to_anchor=(1.01, 1.04))
plt.savefig('tnic_overheads.pdf',dpi=400)
#plt.show()
