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

#plt.rcParams["figure.figsize"] = (21, 8)
#plt.rcParams.update({'font.size': 60})
fig = plt.figure(figsize=COLUMN_FIGSIZE)
plt.rcParams.update({'font.size': 20})
plt.rcParams["figure.figsize"] = (7.4, 3)




species = ('Intel-x86', 'AMD', 'SGX', 'AMD-sev', 'TNIC')
item_counts = {
    'access+transfer': np.array([9, 27, 16, 35, 17]),
    'computation': np.array([1.6, 4, 29.3, 63, 7]) #TIME DIFF     'computation': np.array([10.6, 31, 45.3, 90, 23])
}

width = 0.30  # the width of the bars: can also be len(x) sequence

#plt.rcParams['hatch.linewidth'] = 2.5
#palette = sns.color_palette("pastel")


#plt.rcParams["figure.figsize"] = (14, 6)
#plt.rcParams.update({'font.size': 30})

fig, ax = plt.subplots()
bottom = np.zeros(5)

i = 0;
h = ['/', '-', 'o']
for item, item_count in item_counts.items():
    p = ax.bar(species, item_count, width, bottom=bottom, color=palette[i], edgecolor="black", hatch=h[i], label=item)
    i += 1
    bottom += item_count

ax.set_ylabel("Latency (us)")

#ax.set_title('Latency breakdown for 64B transfers')
ax.legend()
ax.legend(loc=0)
plt.savefig('latency_breakdown.pdf',dpi=400)

plt.show()
