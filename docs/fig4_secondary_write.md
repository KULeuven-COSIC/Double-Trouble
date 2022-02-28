# Secondary Write 

<p align="center" width="100%">
    <img width="500" src="./figure4.png"> 
</p>

This experiment provides supporting evidence for Fig 4e. It is implemented in `sw/figure4/sec_write.c`.  

## Initialization

* [Program the FPGA](./program_fpga.md)
* [Handle CPU Assignments](./cpu_assignments.md)

## Execution

```
$ make sec_wr
```

## Expected Results

```
What happens to lines in L2 when written by the secondary device?
[+] HW access time says        303 (LLC)
[+] SW access time says         88 (LLC)
[+] Other core time says        86 (LLC)
```
We conclude that the line moves to the LLC.

_These findings support (Fig. 4e)._
