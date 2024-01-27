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
palette = sns.color_palette("muted")

plt.rcParams["figure.figsize"] = (21, 8)
plt.rcParams.update({'font.size': 40})

x = ['128', '256', '512', '1K', '2K', '4K', '8K', '16K', '32K']
x_axis = np.arange(len(x))

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

ax1.semilogy(x_axis, rdma_hw, linestyle='--', color=palette[0], marker="8", markersize=20, label="RDMA-hw")
ax1.semilogy(x_axis, tnic_sender_throughput, linestyle='--', color=palette[5], marker="s", markersize=20, label="TNIC w/ A.")
ax1.semilogy(x_axis, tnic_throughput, linestyle='--', color=palette[3], marker=">", markersize=20, label="TNIC")

ax1.set_xlabel('packet size (B)')
ax1.set_ylabel('Throughput (MB/s)')
ax1.set_xticks(x_axis)
ax1.set_xticklabels(x)
plt.tight_layout()
plt.legend(loc='best', ncol=2, borderpad=0.01, columnspacing=0.05)
plt.savefig('rpc_thr.pdf',dpi=400)
