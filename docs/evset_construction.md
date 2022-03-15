# Eviction Set Construction

<p align="center" width="100%">
    <img width="500" src="./evset_construction.png"> 
</p>

This code is used to evaluate the eviction set construction performance for 
various configurations, which are:

* Non-default DDIO way settings
* Huge or small pages
* Different levels of stress
* Options of congruence checks integrated into the eviction set construction

This analysis is used in the paper for the evaluations in Section 8.1.1, and to 
construct Figure 8 (given above).

The corresponding experiment is implemented in `sw/evset_construction/main.c`. 

## Initialization

* [Program the FPGA](./program_fpga.md)
* [Install the MSR Tools](./install_msr_tools.md)
* [Handle CPU Assignments](./cpu_assignments.md)

For the CPU assignments of this specific example, you should consider the stress application. For this purpose, you can change line 18-19 in `main.c` :

```
#define ATTACKER_CORE 0
#define STRESS_COMMAND "taskset --cpu-list 2,4,6,8,10,12,14,18,20,22,24,26,28,30 stress -m %c & sleep 2"
```

Considering the attacker-victim models, we assume that the attacker controls her own CPUs, while the victim's CPUs are not under her control. Hence, we prefer to not associate the stress app with the CPUs of the corresponding attacker's app, while associating it with all the other CPUs.

## Execution

```
make
./app <stress_level> <page_type> <congruence_check_type> <DDIO_way_count>
```

The corresponding arguments are as follows:

### <stress_level>

For the stress, the app will invoke the `stress` application with passing the `<stress_level>` argument to it, as `stress -m <stress_level>`, if the argument is non-zero.

Be careful with too big numbers as it can freeze your computer. It is advised to consider the target platform's CPU count when selecting this value.

### <page_type>

`H` for huge pages, `S` for small.

### <congruence_check_type>

`N` for no checks, `S` for single check, `M` for multiple checks.

### <DDIO_way_count>

`2` is the minimum and default configuration. The maximum is determined by the associativity of your LLC cache. For example, 11 is the maximum in an 11-way set associative cache.

## Expected Results

For each execution, the app constructs an eviction set and prints how long it took to do so. In addition, it applies a congruence test to report how many addresses were erroneously determined to be congruent.

```
 [+] An eviction set is constructed:

 Evset Elem | HW ADDR     SW ADDR           |Retries|Access Time|
 -----------|-------------------------------|-------|-----------|
          0 | 0x01831000 0x00002aaac0c40000 |     2 |       322 |
          1 | 0x0183c000 0x00002aaac0f00000 |     2 |       324 |
          2 | 0x0184d800 0x00002aaac1360000 |     0 |       322 |
          3 | 0x01857000 0x00002aaac15c0000 |     0 |       325 |
          4 | 0x01866000 0x00002aaac1980000 |     0 |       328 |
          5 | 0x01871800 0x00002aaac1c60000 |     0 |       321 |
          6 | 0x0187c800 0x00002aaac1f20000 |     0 |       325 |
          7 | 0x0188c000 0x00002aaac2300000 |     0 |       326 |
          8 | 0x01896800 0x00002aaac25a0000 |     0 |       323 |
          9 | 0x018a7800 0x00002aaac29e0000 |     0 |       329 |
         10 | 0x018bd000 0x00002aaac2f40000 |     0 |       328 |

     Construction time: 0.19 ms

 [ ] The eviction set is tested (in SW):

     Number of false positives: 0 
```

The collected results are evaluated in the Section 8.1.1 of paper, and summarised in Figure 8.
