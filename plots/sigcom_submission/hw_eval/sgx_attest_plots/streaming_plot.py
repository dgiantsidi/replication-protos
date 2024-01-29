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

COLUMN_FIGSIZE = (7.4, 3)
fig = plt.figure(figsize=COLUMN_FIGSIZE)

#plt.rcParams["figure.figsize"] = COLUMN_FIGSIZE
plt.rcParams.update({'font.size': 20})


#define custom function
def g_mean(x):
    a = np.log(x)
    return np.exp(a.mean())

step = 50
y = []
x = []
cnt = 0
for line in open('sgx_empty_300.txt', 'r'):
    x.append(cnt)
    y.append(int(line))
    cnt += 1

y_scone = []
for line in open('sgx_stream_300.txt', 'r'):
    y_scone.append(int(line))

y_normal = []
for line in open('normal_stream_300.txt', 'r'):
    y_normal.append(int(line))

for step in range(50, 350, 50):
    start = step - 50;
    print(start)
    print(step)

    plt.ylabel('Latency (us)')
    #plt.yticks(y_scone)
    #plt.plot(x[start:step], y[start:step], marker = 'o', c = 'g', label="S-Empty", linestyle = 'dotted', ms = 20)
    plt.plot(x[start:step], y_scone[start:step], marker = 'X', c = 'b', label="SGX", linestyle = 'dotted', ms = 12)
    plt.plot(x[start:step], y[start:step], marker = 'o', c = 'g', label="SGX-emtpy", linestyle = 'dotted', ms = 12)
    plt.plot(x[start:step], y_normal[start:step], marker = '*', c = 'r', label="Intel-x86", linestyle = 'dotted', ms = 12)
    plt.legend(loc='upper center', bbox_to_anchor=(0.3, 0.8, 0.4, 0.4), ncol=3, fontsize = 20, frameon=True, borderpad=0.1, columnspacing=1)
    labels = [i for i in range(0, 50)]
    print(start)
    print(step)
    plt.xticks([])
    print(g_mean(y_scone[start:step]))
           # (ncol=3, bbox_to_anchor=(0.2, 0.9))
#   plt.show()
    plt.savefig('foo'+str(start)+'.pdf',  dpi=400)
    plt.clf()

print(statistics.mean(y_scone))
print(statistics.mean(y_normal))
print(statistics.mean(y))
#calculate geometric mean
print("geomenric mean=")
print(g_mean(y_scone))
print(g_mean(y_normal))
print(g_mean(y))



