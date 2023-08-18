from os import read
import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import csv
import seaborn as sns

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("pastel")


plt.rcParams["figure.figsize"] = (16, 8)
plt.rcParams.update({'font.size': 40})

fig, ax = plt.subplots()

rsa_lat_64 = [700, 1200, 800]
rsa_lat_128 = [700, 1000, 400]
rsa_lat_256 = [700, 800, 300]

envs = ['CPU', 'SGX', 'U280']
x = np.arange(len(envs))
low = int(min(rsa_lat_64))
high = int(max(rsa_lat_64))
plt.ylim([0, math.ceil(high+0.5*(high-low))])
width = 0.10
x2 = [r+width for r in x]
x3 = [r+width for r in x2]

ax.bar(x, rsa_lat_64, width, color= palette[3], hatch="o", edgecolor="black", label="64B")
ax.bar(x2, rsa_lat_128, width, color= palette[4], hatch="X", edgecolor="black", label="128B")
ax.bar(x3, rsa_lat_256, width, color= palette[2], hatch="-", edgecolor="black", label="256B")
ax.set_ylabel("Latency (ns)")
ax.set_xticks(x2)
ax.set_xticklabels(envs)
ax.set_title("RSA")
ax.legend()
#plt.savefig('rsa.pdf',dpi=400)
plt.show()
