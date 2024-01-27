from os import read
import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import csv
import seaborn as sns
# Import statistics Library
import statistics

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("pastel")

plt.rcParams["figure.figsize"] = (6.5, 3.4)
plt.rcParams.update({'font.size': 17})

x = ['128', '256', '512', '1K', '2K', '4K', '8K', '16K', '32K']
x_axis = np.arange(len(x))
width=0.20
x_axis2 = [k+width for k in x_axis]
x_axis3 = [k+2*width for k in x_axis]
rdma_hw = [
        280.45,
        560.18,
        1004.07,
        1673.55,
        4102.87,
        6295.72,
        6809.49,
        9000.07,
        9816.18]

tnic_throughput = [
        23.73,
        37.97,
        54.47,
        69.49,
        80.64,
        87.65,
        91.64,
        93.77,
        94.87]

tnic_sender_throughput = [
        23.70,
        38.02,
        54.47,
        69.51,
        80.65,
        87.66,
        91.64,
        93.77,
        94.87]

fig, ax1 = plt.subplots()


ax1.bar(x_axis, rdma_hw, hatch='o', width=width, color=palette[9], edgecolor="black", label="RDMA-hw")

ax1.bar(x_axis2, tnic_sender_throughput, width=width, color= palette[1], hatch="*", edgecolor="black", label="TNIC-att")
ax1.bar(x_axis3, tnic_throughput, color= palette[2], width=width, hatch="-", edgecolor="black", label="TNIC")
plt.text(4.1, 400000, "Higher is better ↑", ha='center', va='center', fontsize=17, weight='bold', c='blue')

plt.ylim([10, 200000])
ax1.set_xlabel('packet size (B)')
ax1.set_ylabel('Throughput (MB/s)')
ax1.set_xticks(x_axis2)
ax1.set_xticklabels(x)
ax1.set_yscale('log')
plt.tight_layout()
plt.legend(loc='upper center', ncol=3, borderpad=0.1, columnspacing=0.5)
plt.savefig('rpc_thr.pdf',dpi=400)
