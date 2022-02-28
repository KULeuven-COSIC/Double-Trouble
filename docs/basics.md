# Basics

This code is provided as an introduction to our experiment app structure. It exemplifies the following:

* The attacker-victim framework, which allows to simulate a victim on another CPU core
* Eviction set creation
* The primitives for hardware and software accesses and timing them
* Collection of access-timing measurements
* Recipe-based accesses

The corresponding experiment is implemented in `sw/basic/attacker_basic.c`, 
which is extended with inline-comments to help the user.

## Initialization

* [Program the FPGA](./program_fpga.md)
* [Handle CPU Assignments](./cpu_assignments.md)

## Execution

```
$ make
$ ./app_basic
```

## Expected Results

The execution will first set thresholds, then construct an eviction set. 
Later, it will perform access timing measurements, and print them as a table, shown below.

```
 [+] Thresholds Configured
        HW_L2 : 310
        HW_LLC: 297     SW_LLC: 78
        HW_RAM: 323     SW_RAM: 164
        HW_THR: 320     SW_THR: 121

 # | EA | HW ADDR     SW ADDR           |Err|Loop |Cycle|
---|----|-------------------------------|---|-----|-----|
 0 |  0 | 0x3f009830 0x00007faf80260c00 | 0 |   0 | 332 |
 0 |  1 | 0x3f00d030 0x00007faf80340c00 | 0 |   0 | 325 |
 0 |  2 | 0x3f013030 0x00007faf804c0c00 | 0 |   0 | 458 |
 0 |  3 | 0x3f017830 0x00007faf805e0c00 | 0 |   0 | 336 |
 0 |  4 | 0x3f01a830 0x00007faf806a0c00 | 0 |   0 | 337 |
 0 |  5 | 0x3f01e030 0x00007faf80780c00 | 0 |   0 | 328 |
 0 |  6 | 0x3f022030 0x00007faf80880c00 | 0 |   0 | 331 |
 0 |  7 | 0x3f026830 0x00007faf809a0c00 | 0 |   0 | 328 |
 0 |  8 | 0x3f02b830 0x00007faf80ae0c00 | 0 |   0 | 325 |
 0 |  9 | 0x3f02f030 0x00007faf80bc0c00 | 0 |   0 | 333 |
 0 | 10 | 0x3f031030 0x00007faf80c40c00 | 0 |   0 | 329 |

 [+] EV Set Constructed 
        Size: 11
        Time: 0.2 ms

 [+] Access Timing Measurements are Taken 

 # | Action     | Median | Average | Std Dev 
 0 | FLUSH      |        |         |
 1 | SW_READ    |    176 |   183.5 |    50.0
 2 | SW_READ    |     38 |    38.7 |     0.9
 3 | FLUSH      |        |         |
 4 | HW_WRITE   |      0 |     0.0 |     0.0
 5 | SW_READ    |     82 |    82.3 |     2.3
 6 | FLUSH      |        |         |
 7 | HW_READ    |    333 |   337.0 |    18.9
 8 | FLUSH      |        |         |
 9 | HW_WRITE   |      0 |     0.0 |     0.0
10 | HW_READ    |    303 |   303.6 |     3.1
```