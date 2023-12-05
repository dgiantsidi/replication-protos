
import matplotlib.pyplot as plt
import numpy as np
# Import statistics Library
import statistics

#define custom function
def g_mean(x):
    a = np.log(x)
    return np.exp(a.mean())



x = []
y = []
cnt = 0
for line in open('scone_empty.txt', 'r'):
    x.append(cnt)
    y.append(int(line))
    cnt += 1

y_scone = []
for line in open('scone_out.txt', 'r'):
    y_scone.append(int(line))

y_normal = []
for line in open('normal.txt', 'r'):
    y_normal.append(int(line))

plt.title("Students Marks")
plt.xlabel('Roll Number')
plt.ylabel('Marks')
plt.yticks(y)
plt.plot(x, y_scone, marker = 'X', c = 'b', label="SGX (attest)", linestyle = 'dotted', ms = 10)
plt.plot(x, y, marker = 'o', c = 'g', label="SGX (empty)", linestyle = 'dotted', ms = 10)
plt.plot(x, y_normal, marker = '*', c = 'r', label="Native (attest)", linestyle = 'dotted', ms = 10)
plt.legend()
print(statistics.mean(y_scone))
print(statistics.mean(y_normal))
print(statistics.mean(y))
#calculate geometric mean
print(g_mean(y_scone))
print(g_mean(y_normal))
print(g_mean(y))
plt.show()

