from os import read
import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import csv
import seaborn as sns

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("pastel")


plt.rcParams["figure.figsize"] = (10, 8)
plt.rcParams.update({'font.size': 40})

fig, ax = plt.subplots()

rsa_lat = [700, 1200, 800]
envs = ['CPU', 'SGX', 'U280']
x = np.arange(len(envs))
low = int(min(rsa_lat))
high = int(max(rsa_lat))
plt.ylim([math.ceil(low-0.5*(high-low)), math.ceil(high+0.5*(high-low))])


ax.bar(envs, rsa_lat, width = 0.23, color= palette[2], hatch='/', edgecolor="black")
ax.set_ylabel("Latency (ns)")
ax.set_title("sha256")
plt.savefig('sha.pdf',dpi=400)
plt.show()
