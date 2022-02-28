# Reduced Eviction

This code implements the eviction with reduced eviction, as described in Section 6.2 of the paper.

It creates random bits, indicating whether the victim accesses target address or not.
The attacker monitors victim's activity and determines whether the victim has performed an access.

Note that the attacker uses only 6 congruent addresses, so she does not need to prime the whole cache set.

The corresponding experiment is implemented in `sw/basic/attacker_evictionreduced.c`. 

## Initialization

* [Program the FPGA](./program_fpga.md)
* [Handle CPU Assignments](./cpu_assignments.md)

## Execution

```
$ make evsred
$ ./app_evsred
```

## Expected Results

First, the experiment constructs a small eviction set. 
In the example below, there are only six congruent addresses, while there are 11 ways in the set.

Later, it performs `TEST_COUNT` tests. In the example below, 1999 out of 2000 tests were successful.

```
 [+] Thresholds Configured  
        HW_L2 : 314
        HW_LLC: 297     SW_LLC: 90
        HW_RAM: 325     SW_RAM: 182
        HW_THR: 322     SW_THR: 136
        
 # | EA | HW ADDR     SW ADDR           |Err|Loop |Cycle: Accum. : Delta |
---|----|-------------------------------|---|-----|-----:--------:-------|
 0 |  0 | 0x3f00d03b 0x00007f43c0340ec0 | 0 |   0 | 336 :      0 :     0 |
 0 |  1 | 0x3f01303b 0x00007f43c04c0ec0 | 0 |   0 | 333 :      0 :     0 |
 0 |  2 | 0x3f01783b 0x00007f43c05e0ec0 | 0 |   0 | 334 :      0 :     0 |
 0 |  3 | 0x3f01a83b 0x00007f43c06a0ec0 | 0 |   0 | 344 :      0 :     0 |
 0 |  4 | 0x3f01e03b 0x00007f43c0780ec0 | 0 |   0 | 336 :      0 :     0 |
 0 |  5 | 0x3f02203b 0x00007f43c0880ec0 | 0 |   0 | 335 :      0 :     0 |

 [+] EV Set Constructed 
        Size: 6
        Time: 0.1 ms

 [+] Experiment is completed
        
        Success 1999/2000
```