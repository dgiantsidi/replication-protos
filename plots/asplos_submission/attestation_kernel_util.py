import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns

plt.rcParams['hatch.linewidth'] = 2.5
palette = sns.color_palette("muted")

plt.rcParams["figure.figsize"] = (21, 8)
plt.rcParams.update({'font.size': 40})

x = ['1', '2', '4', '8', '16', '32']
x_axis = np.arange(len(x))

base_lut_util = 14.02
att_kernel_lut_util = 2.62
total_lut_util = [base_lut_util + (int(num_kernels) * att_kernel_lut_util) for num_kernels in x]
print("Total LUT util: ", total_lut_util)

base_bram_util = 2.62
att_kernel_bram_util = 0.835
total_bram_util = [base_bram_util + (int(num_kernels) * att_kernel_bram_util) for num_kernels in x]
print("Totla BRAM util: ", total_bram_util)

base_reg_util = 14.08
att_kernel_reg_util = 2.18
total_reg_util = [base_reg_util + (int(num_kernels) * att_kernel_reg_util) for num_kernels in x]
print("Total Flip-flop util: ", total_reg_util)

# CARRY8 is the 4th most used resource, use it as the upper bound for all other resources
base_others_util = 1.0125
att_kernel_others_util = 0.4633
total_others_util = [base_others_util + (int(num_kernels) * att_kernel_others_util) for num_kernels in x]
print("Total Others util: ", total_others_util)

fig, ax1 = plt.subplots()

ax1.semilogy(x_axis, total_lut_util, linestyle='--', color=palette[0], marker="8", markersize=20, label="LUTs")
ax1.semilogy(x_axis, total_bram_util, linestyle='--', color=palette[5], marker="s", markersize=20, label="BRAM")
ax1.semilogy(x_axis, total_reg_util, linestyle='--', color=palette[3], marker=">", markersize=20, label="Registers")
ax1.semilogy(x_axis, total_others_util, linestyle='--', color=palette[2], marker="*", markersize=20, label="Others")

ax1.set_xlabel('Number of attestation kernels')
ax1.set_xticks(x_axis)
ax1.set_xticklabels(x)

ax1.set_ylabel('Resource utilization (%)')
ax1.set_yticks(np.arange(0, 100, 25))
ax1.set_yscale('linear')

plt.tight_layout()
plt.legend(loc='best', ncol=2, borderpad=0.01, columnspacing=0.05)
plt.savefig('attestation_kernel_util.pdf',dpi=400)
plt.show()
