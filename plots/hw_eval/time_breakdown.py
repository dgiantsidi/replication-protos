from os import read
import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import csv
import seaborn as sns


species = ('CPU', 'FPGA', 'SGX')
item_counts = {
    'pcie': np.array([5, 2, 8]),
    'sha': np.array([73, 34, 61]),
    'rsa': np.array([73, 34, 58])
}

width = 0.10  # the width of the bars: can also be len(x) sequence

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("pastel")


plt.rcParams["figure.figsize"] = (14, 6)
plt.rcParams.update({'font.size': 30})

fig, ax = plt.subplots()
bottom = np.zeros(3)

i = 0;
h = ['/', '-', 'o']
for item, item_count in item_counts.items():
    p = ax.bar(species, item_count, width, bottom=bottom, color=palette[i], edgecolor="black", hatch=h[i], label=item)
    i += 1
    bottom += item_count

ax.set_ylabel("Time (ns)")

ax.set_title('Latency breakdown for 128B transfers')
ax.legend()
ax.legend(loc=1, bbox_to_anchor=(1.01, 1.04))
plt.savefig('latency_breakdown.pdf',dpi=400)

plt.show()
