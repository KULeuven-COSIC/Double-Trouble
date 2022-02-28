# Eviction Candidate

This code determines whether DDIO reads and writes are recorded by the cache replacement policy, i.e., whether they change the eviction candidate. 

There are three parts of the experiment. In each, first a picked TARGET address is set as the eviction candidate, and before evicting:
1. No further access is peformed to the TARGET
2. Read accesses are peformed to the TARGET
3. Write accesses are peformed to the TARGET

Naturally, in the first part, the TARGET should still be kept as the eviction candidate, so it should be evicted by the next congruent address. The second and third parts help determining if read or write accesses change the candidate, based on that a recently accessed cache line would not be the eviction candidate in (a derivate of) LRU replacement policy. 

The obtained results (given below) support the claim that read accesses by the secondary device do not change the eviction candidate, while write accesses do.

The corresponding experiment is implemented in `sw/basic/attacker_evscandid.c`.

## Initialization

* [Program the FPGA](./program_fpga.md)
* [Handle CPU Assignments](./cpu_assignments.md)

## Execution

```
$ make evscandid
$ ./app_evscandid
```

## Expected Results

An example execution (with 1000 iterations) yields the following results.
Clearly, write accesses to the eviction candidate lead to a state where it is not the eviction candidate anymore.
On the other hand, read accesses do not affect the eviction candidate, as if there was no action on the candidate address. This behavior persists even though the read operation is repeated 10 times.

```
 [+] Experiment results

        NO ACTION | Target is evicted: 1000/1000
        10 READS  | Target is evicted: 1000/1000
        10 WRITES | Target is evicted:    2/1000
```