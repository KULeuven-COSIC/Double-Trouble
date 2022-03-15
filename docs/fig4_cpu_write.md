# CPU Write

<p align="center" width="100%">
    <img width="500" src="./figure4.png"> 
</p>

This experiment provides supporting evidence for Fig 4c. It is implemented in `sw/figure4/cpu_write.c`.  

## Initialization

* [Program the FPGA](./program_fpga.md)
* [Handle CPU Assignments](./cpu_assignments.md)

## Execution

```
$ make cpu_wr
```

## Expected Results

```
 What happens to lines in the LLC when written by a CPU core?
 [+] HW access time says        319 (L2)
 [+] SW access time says         38 (L2)
 [+] Other core time says       150 (L2)
```

We conclude that the line moves to the L2 of the writing core.

_These findings support (Fig. 4c)._