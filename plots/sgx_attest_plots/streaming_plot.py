
import matplotlib.pyplot as plt
import numpy as np
# Import statistics Library
import statistics

plt.rcParams["figure.figsize"] = (40,10)
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
    plt.ylabel('Latency (us)')
    plt.yticks(y)
    plt.plot(x[start:step], y_scone[start:step], marker = 'X', c = 'b', label="SGX (attest)", linestyle = 'dotted', ms = 10)
    plt.plot(x[start:step], y[start:step], marker = 'o', c = 'g', label="SGX (empty)", linestyle = 'dotted', ms = 10)
    plt.plot(x[start:step], y_normal[start:step], marker = '*', c = 'r', label="Native (attest)", linestyle = 'dotted', ms = 10)
    plt.legend()
 #   plt.show()
    plt.savefig('foo'+str(start)+'.pdf', bbox_inches='tight', dpi=300)

print(statistics.mean(y_scone))
print(statistics.mean(y_normal))
print(statistics.mean(y))
#calculate geometric mean
print(g_mean(y_scone))
print(g_mean(y_normal))
print(g_mean(y))



