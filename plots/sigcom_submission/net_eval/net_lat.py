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


#plt.rcParams["figure.figsize"] = (21, 8)
#plt.rcParams.update({'font.size': 40})

COLUMN_FIGSIZE = (6.5, 3.4)
#fig = plt.figure(figsize=COLUMN_FIGSIZE)
plt.rcParams['figure.figsize'] = [15, 4]
plt.rcParams["figure.autolayout"] = True

#plt.rcParams["figure.figsize"] = COLUMN_FIGSIZE
plt.rcParams.update({'font.size': 17})



x = ['128', '256', '512', '1K', '2K', '4K', '8K', '16K', '32K']
x_axis = np.arange(len(x))
#tnic_tps = [2.41, 10, 40, 50, 100, 200, 1000]
tnic_latency = [15.55502,
        18.06195,
        23.42498,
        34.34789,
        56.002,
        99.01102,
        142.15489,
        228.11605,
        399.40459]

tnic_latency_sender = [
        10.377,
        11.71049,
        14.6019,
        20.112,
        31.12097,
        53.1533,
        95.75007,
        181.00867,
        351.82211]


hw_rdma = [
        5.25877,
        5.10717,
        5.23772,
        5.60486,
        5.87281,
        7.04846,
        17.88628,
        12.39792,
        19.70433]

#eRPC_sgx_tps = [3.41, 10, 10, 20, 150, 200, 500]
sw_RDMA_sgx_latency = [
        84.23,
        82.9,
        84.186,
        0,
        0,
        0,
        0,
        0,
        0]

sw_RDMA = [
        16.07,
        16.08,
        15.586,
        16.67,
        31.07,
        36.71,
        65.01,
        71.225,
        102.4838]

#eRPC_nat_tps = [4.41, 10, 10, 20, 150, 200, 500]
#eRPC_nat_latency = [7, 8.1, 8.6, 8.8, 9, 10, 20]

#tcp_nat_tps = [5.41, 10, 10, 20, 150, 200, 500]
#tcp_nat_latency = [7, 8.1, 8.6, 8.8, 9, 10, 20]

fig, ax1 = plt.subplots()

x1 = np.arange(len(x))
low = 0
high = 200
plt.ylim([0, 550])
width1 = 1.4/len(x)
χ1 = [k-3*width1 for k in x1]
x2 = [r+width1 for r in x1]
x3 = [r+width1 for r in x2]
x4 = [r+width1 for r in x3]
x5 = [r+width1 for r in x4]
x6 = [r+width1 for r in x5]
ax1.bar(x1, hw_rdma, width=width1, color= palette[9], hatch="o", edgecolor="black", label="RDMA-hw")
ax1.bar(x2, sw_RDMA, width=width1, color= palette[4], hatch="\\", edgecolor="black", label="DRCT-IO")
ax1.bar(x3, tnic_latency, width=width1, color= palette[2], hatch="-", edgecolor="black", label="TNIC")
ax1.bar(x4, sw_RDMA_sgx_latency, width=width1, color= palette[3], hatch="/", edgecolor="black", label="DRCT-IO-att")
ax1.bar(x5, tnic_latency_sender, width=width1, color= palette[1], hatch="*", edgecolor="black", label="TNIC-att")
#empty=[0, 0, 0, 0, 0, 0, 0, 0, 0]
#ax1.bar(x6, empty, width1*4, color= palette[7], hatch="+", edgecolor="black")


  #   plt.text(rect.get_x() + rect.get_width() / 2.0, height, f'{height:.0f}', ha='center', va='bottom')
for k, y, p in zip(x1, hw_rdma, hw_rdma):
    s = " " + f'{p:.0f}'+"us"
    plt.text(k, y, s, ha='center', va='bottom', fontsize=15, weight="bold", rotation=90)

for k, y, p in zip(x2, sw_RDMA, sw_RDMA):
    s = " " + f'{p:.0f}'+"us"
    plt.text(k, y, s, ha='center', va='bottom', fontsize=15, weight="bold", rotation=90)

for k, y, p in zip(x3, tnic_latency, tnic_latency):
    s = " " + f'{p:.0f}' +"us"
    plt.text(k, y, s, ha='center', va='bottom', fontsize=15, weight="bold", rotation=90)

for k, y, p in zip(x4, sw_RDMA_sgx_latency, sw_RDMA_sgx_latency):
    s = " " + f'{p:.0f}'+"us"
    if p != 0:
        plt.text(k, y, s, ha='center', va='bottom', fontsize=15, weight="bold", rotation=90)
    else:
        plt.text(k, y, "X", ha='center', va='bottom', fontsize=15, weight="bold", c='r')

for k, y, p in zip(x5, tnic_latency_sender, tnic_latency_sender):
    s = " " + f'{p:.0f}'+"us"
    plt.text(k, y, s, ha='center', va='bottom', fontsize=15, weight="bold", rotation=90)
#ax1.plot(x_axis, hw_rdma, linestyle='--', color=palette[9], marker="X", markersize=20, label="RDMA-hw")
#ax1.plot(x_axis, sw_RDMA, linestyle='--', color=palette[0], marker="8", markersize=20, label="D-I/O")
#ax1.plot(x_axis, tnic_latency, linestyle='--', color=palette[3], marker=">", markersize=20, label="TNIC")
#ax1.plot(x_axis, sw_RDMA_sgx_latency, linestyle='--', color=palette[2], marker="o", markersize=20, label="D-I/O w/ A.")
#ax1.plot(x_axis, tnic_latency_sender, linestyle='--', color=palette[5], marker="s", markersize=20, label="TNIC w/ A.")

plt.text(4.1, 500, "Lower is better ↓", ha='center', va='center', fontsize=17, weight='bold', c='blue')


#ax1.set_title('Latency (us)')
#ax1.set_yscale('log')
ax1.set_xlabel('packet size (B)')
ax1.set_ylabel('Latency (us)')
ax1.set_xticks(x3)
ax1.set_xticklabels(x)
plt.tight_layout()
#plt.legend(loc='best', bbox_to_anchor=(0.55, 0.7, 0.4, 0.4), ncol=3, borderpad=0.01, columnspacing=0.05)
#plt.legend(loc='best', bbox_to_anchor=(0.87, 0.55), ncol=2, borderpad=0.01, columnspacing=0.05)
plt.legend(loc='best', ncol=2, borderpad=0.5, columnspacing=0.2, bbox_to_anchor=(0.33, .98))
#plt.legend(loc='best', bbox_to_anchor=(0.55, 0.2), ncol=1, borderpad=0.01, columnspacing=0.05)
plt.savefig('rpc_lat.pdf',dpi=400)
