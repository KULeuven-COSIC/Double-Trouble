# DDIO Replacement Policy

This code performs various access patterns with DDIO lines, checks the cache contents after these accesses, and compares them with the expected contents for 
different replacement policies. 

In this experiment, we considered only the default 2-Way DDIO configuration. 

The corresponding experiment is implemented in `sw/basic/attacker_reppolicy.c`. 
The expected cache contents are implemented in `sw/basic/scripts/policy.py`, 
which takes LRU, RRRIP, and variants of QUAD LRU policies into account.
The script dumps its output into `sw/basic/policy_test.h`.

## Initialization

* [Program the FPGA](./program_fpga.md)
* [Handle CPU Assignments](./cpu_assignments.md)

## Execution

```
$ make reppolicy
$ ./app_reppolicy
```

## Expected Results

The experiment produces a table as follows, with the following interpretation of the columns:

1. Indicates the executed access pattern, with several congruent addresses (each represented by a letter)
2. Indicates the cache contents found after the performed access patterns.
3. Indicates how reliably the same cache contents are found
4. 5, 6, 7, and 8 indicate the expected cache contents according to the replacement policy, and if the observed contents match with them (in color, not visible below).

```
Access | CC | Reliability  | LRU | RRIP | QUAD1 | QUAD2 | QUAD3 |
ABC    | BC | 0.99700      | CB  | CB   | AC    | CB    | BC    |
ACB    | BC | 1.00000      | BC  | BC   | AB    | BC    | CB    |
BAC    | AC | 0.99800      | CA  | CA   | BC    | CA    | AC    |
BCA    | AC | 1.00000      | AC  | AC   | BA    | AC    | CA    |
CAB    | AB | 0.99900      | BA  | BA   | CB    | BA    | AB    |
CBA    | AB | 1.00000      | AB  | AB   | CA    | AB    | BA    |
AABC   | BC | 0.99900      | CB  | AC   | AC    | AC    | CA    |
AACB   | BC | 0.99600      | BC  | AB   | AB    | AB    | BA    |
ABAC   | AC | 0.99900      | AC  | AC   | AC    | AC    | CA    |
ABBC   | BC | 0.99900      | CB  | CB   | CB    | CB    | BC    |
ABCA   | AC | 1.00000      | CA  | CA   | AC    | CA    | AC    |
...
```