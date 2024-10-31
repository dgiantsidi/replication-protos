# Attestation Kernel Resource Utilization

## Vivado Utilization Report

```
+----------------------------------------------------+-----------------------+----------------+----------------+--------------+-------------+----------------+-------------+-----------+----------+------------+
|                     Instance                       |        Module         |   Total LUTs   |   Logic LUTs   |    LUTRAMs   |     SRLs    |       FFs      |    RAMB36   |   RAMB18  |   URAM   | DSP Blocks |
+----------------------------------------------------+-----------------------+----------------+----------------+--------------+-------------+----------------+-------------+-----------+----------+------------+
| cyt_top (whole project, includes everything below) |                 (top) | 216905(16.64%) | 201053(15.42%) | 13520(2.25%) | 2332(0.39%) | 423891(16.26%) | 289(14.34%) | 46(1.14%) | 0(0.00%) |   0(0.00%) |
|   inst_user_wrapper_0 (attestation kernel)         | design_user_wrapper_0 |   34138(2.62%) |   32884(2.52%) |  1216(0.20%) |   38(0.01%) |   56914(2.18%) |   72(3.58%) |  9(0.22%) | 0(0.00%) |   0(0.00%) |
```

## BRAM

RAMB36 is the most used individual resource of the attestation kernel. A RAMB18 is one half of a RAMB36, but because both RAMB18 in a RAMB36 share some resources, two RAMB18 may be put in two separate RAMB36. We assume each RAMB18 gets its own RAMB36 and the attestation kernel takes 81 RAMB36. URAM can also be used if all RAMB36 are full, so we calculate the total BRAM usage like this:

- RAMB36 + URAM capacity: 72576k + 276480k = 349056k
- Attestation kernel RAMB36 usage: 2916k/72576k = 4.02%
- Whole design without attestation kernel total BRAM usage: 9144k/349056k = 2.62%
- Attestation kernel total BRAM usage: 2916k/349056k = 0.835%

BRAM usage is not the limiting factor for how many attestation kernels can fit on the FPGA.

## Registers

- Whole design without attestation kernel: 14.08%
- Attestation kernel: 2.18%

## LUTs

- Whole design without attestation kernel: 14.02%
- Remaining LUTs: 85.98%
- Attestation kernel: 2.62%
- Mamimum number of attestation kernels: 85,98%/2,62% = 32,82 => 32

TNIC with 32 attestation kernel uses 97.86% of LUTs.

## Others (from detailed report in Vivado GUI)

The 4th most used resource of the attestation kernel is CARRY8.

- Whole design without attestation kernel: 1.0125%
- Attestation kernel: 0.4633%
