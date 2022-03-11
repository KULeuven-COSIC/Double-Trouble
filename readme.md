# Double Trouble

This repository provides a set of experiments for the paper *Double Trouble: Combined Heterogeneous Attacks on Non-Inclusive Cache Hierarchies* paper, to appear at *USENIX Security 2022*.


## Experiments

The following table lists all the experiments, and to which section of the paper they apply.
The order of the experiments is chosen such that:
- the initial experiments help understanding the non-inclusive LLC structure and the DDIO-based access to it,
- the later experiments allow
  - reverse engineering the underlying mechanisms, 
  - demonstrating the findings and 
  - analyzing the performance.

The first experiment is provided as a warm-up to the basic API usage, 
building of attacker-victim framework, eviction set creation, and cache-timing measurements.

For each experiment, a dedicated documentation file is provided.
This file explains the experiment, how to run it, and what the expected results are.

The links in the table take you to the corresponding documentation.

Experiment                                                         | Folder                | Section   |
-------------------------------------------------------------------|-----------------------|-----------|
[Basic Functionality](./docs/basics.md)                            | sw/basic              |           |
[Secondary Write](./docs/fig4_secondary_write.md)                  | sw/fig4               | Fig 4e    |
[CPU Write](./docs/fig4_cpu_write.md)                              | sw/fig4               | Fig 4c    |
[Shared Access & CPU Read](./docs/fig4_shared_access_cpu_read.md)  | sw/fig4               | Fig 4d,4f |
[Cache Timing Histogram](./docs/cache_timing_histogram.md)         | sw/basic              | Apx B     |
[Eviction Candidate](./docs/eviction_candidate.md)                 | sw/basic              | Sec 3.2.2 |
[DDIO Replacement Policy](./docs/ddio_rep_policy.md)               | sw/basic              | Apx D     |
[Eviction with Reduced EvSet](./docs/eviction_with_reduced_set.md) | sw/basic              | Sec 6.2   |
[EvSet Const](./docs/evset_construction.md)                        | sw/evset_construction | Sec 8.1.1 |
[DDIO/DDIO+ Reverse Engineering](./docs/ddio_reverse_eng.md)       | sw/ddio_reverse_eng   | Sec 5     |


## Basics of API

A list of hardware-assisted cache side-channel experiments is given above. 
These experiments make use of our hardware accelerator via a simple API.

To make the hardware access a given memory location, we make use of CSR 
(Control-Status Register) accesses through the functions in `fpga_utils.h`.
For instance, making the FPGA perform a read access to a given address includes three steps; 
1. Writing the target address to FPGA's `CSR_TARGET_ADDRESS` register, 
2. Initializing the `FPGA_READ` operation by writing it to `CSR_CONTROL` register,
3. Polling for completion of the operation by reading the `CSR_CONTROL` register.

For handling these operations, we defined macros as shown below:

```
#define HW_READ(hw_read_addr)  ({               \
  fpga_write(CSR_TARGET_ADDRESS, hw_read_addr); \
  fpga_write(CSR_CONTROL, FPGA_READ);           \
  while (fpga_read(CSR_CONTROL)==0);            })
```

We also provide a simple API for the hardware-based eviction set creation.
The corresponding functions are defined in `eviction_hw.h`.
The main three functions are listed below with an explanation of their arguments.

```
int flags = 0
flags |= FPGA_CREATE_EVS_HUGE; // or FPGA_CREATE_EVS_SMALL;
flags |= FPGA_CREATE_EVS_EN_TEST;
flags |= FPGA_CREATE_EVS_EN_TEST_MULTI;
flags |= FPGA_CREATE_EVS_EN_TEST_ONLYFIRST;

// Create an eviction set and get its execution time
time = evs_create(
  target_index,   // the offset over shared memory address
  threshold,      // access time threshold, whether read is from LLC or RAM
  WAIT,           // number of cycles to wait btw. consecutive memory ops.
  evs_len,        // number of requested congruent addresses 
  flags);         // the execution flags
  
// Read the eviction set from hardware
evs_len = evs_get(
  eviction_set_sw, // pointer to array for sw addresses
  eviction_set_hw, // pointer to array for hw addresses
  EVS_PRINT);      // verbose flag, enabling print of the evset

// Verify the correctness of the eviction set
nb_faulty = evs_verify(
  target_index,     // the offset over shared memory address
  eviction_set_sw,  // pointer to array for sw addresses
  threshold,        // access time threshold, whether read is from LLC or RAM
  evs_len,          // number of congruent addresses in the evset
  EVS_PRINT);       // verbose flag, enabling print of the evset
```


## Target Platform

This repository is tested on our local FPGA Accelerated Computer Server. It has a similar configuration to [FPGA Accelerated compute platforms of Intel Labs (IL) Academic Compute Environment (ACE)](https://wiki.intel-research.net/FPGA.html#fpga-system-classes).

Before compiling or running any code, you should first establish such a machine, which has the following two:
* Xeon processor, Skylake-{SP,X} or Cascade Lake-{SP,X}
* An Intel PAC (Arria10 or Stratix10).
  - For the covert channel between combined attackers, two PACs are needed.

In comparison to the ACE platforms, our local server provides us with root privileges on the machine, which is essential for changing the DDIO configuration, required for two experiments: eviction set construction with non-default DDIO ways, and reverse engineering of DDIO.

## How to cite this work

```
@inproceedings{Purnal2022double,
  author    = {Purnal, Antoon and Turan, Furkan and Verbauwhede, Ingrid},
  title     = {Double Trouble: Combined Heterogeneous Attacks on Non-Inclusive Cache Hierarchies},
  booktitle = {USENIX Security Symposium},
  year      = {2022},
}
```