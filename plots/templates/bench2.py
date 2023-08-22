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

rsa_sign_lat = [700, 1200, 800]
rsa_ver_lat = [400, 400, 400]
width = 0.10
envs = ['CPU', 'SGX', 'U280']
x = np.arange(len(envs))
x2 = [r+width for r in x]
low = 0
high = int(max(rsa_sign_lat))
plt.ylim([0, math.ceil(high+0.5*(high-low))])


ax.bar(x, rsa_sign_lat, width = 0.10, color= palette[6], hatch='/', edgecolor="black", label="sign")
ax.bar(x2, rsa_ver_lat, width = 0.10, color= palette[8], hatch='o', edgecolor="black", label="verify")
ax.set_ylabel("Latency (ns)")
ax.set_title("RSA")
ax.set_xticks([r+width/2 for r in x ])
ax.set_xticklabels(envs)
ax.legend()
plt.savefig('rsa.pdf',dpi=400)
plt.show()
