# CPU Association of Attacker and Victim Threads

For our experiments, we use the following testbed.

The application starts in `main()`, and then forks into threads implemented in the `attacker()`, `attacker_helper()` and `victim()` functions.

We use `sched_setaffinity()` to associate each fork to a specific CPU/core. This allows to experiment with cross-core and cross-socket attackers.

To handle this assingment proficiently, we should know the mapping of CPUs to cores and sockets. For this purpose we can check `lscpu --all --extended` command, which prints CPU information in a table. An example is given below, which is obtained on our server, using two sockets of an Intel Xeon Silver 4208.

For the table below, CPUs 0 and 16 are multithreaded siblings on physical core 0. Even and odd numbered logical CPUs can be seen to be tied to different sockets (i.e., NUMA nodes).

Therefore, for a cross-core attacker experiment, we can assign attacker to CPU 0, attacker's helper to CPU 2, and victim to CPU 4. This way, all work on different cores on the same socket.

To handle these assigments, we use macros (at the top of `main.c` of each experiment) as follows:

```
#define ATTACKER_CORE         0
#define ATTACKER_HELPER_CORE  2
#define VICTIM_CORE           4
```
___

The output of `lscpu --all --extended` on our server with two sockets of Intel Xeon Silver 4208 looks as follows:

```
CPU NODE SOCKET CORE L1d:L1i:L2:L3 ONLINE
0   0    0      0    0:0:0:0       yes
1   1    1      1    1:1:1:1       yes
2   0    0      2    2:2:2:0       yes
3   1    1      3    3:3:3:1       yes
4   0    0      4    4:4:4:0       yes
5   1    1      5    5:5:5:1       yes
6   0    0      6    6:6:6:0       yes
7   1    1      7    7:7:7:1       yes
8   0    0      8    8:8:8:0       yes
9   1    1      9    9:9:9:1       yes
10  0    0      10   10:10:10:0    yes
11  1    1      11   11:11:11:1    yes
12  0    0      12   12:12:12:0    yes
13  1    1      13   13:13:13:1    yes
14  0    0      14   14:14:14:0    yes
15  1    1      15   15:15:15:1    yes
16  0    0      0    0:0:0:0       yes
17  1    1      1    1:1:1:1       yes
18  0    0      2    2:2:2:0       yes
19  1    1      3    3:3:3:1       yes
20  0    0      4    4:4:4:0       yes
21  1    1      5    5:5:5:1       yes
22  0    0      6    6:6:6:0       yes
23  1    1      7    7:7:7:1       yes
24  0    0      8    8:8:8:0       yes
25  1    1      9    9:9:9:1       yes
26  0    0      10   10:10:10:0    yes
27  1    1      11   11:11:11:1    yes
28  0    0      12   12:12:12:0    yes
29  1    1      13   13:13:13:1    yes
30  0    0      14   14:14:14:0    yes
31  1    1      15   15:15:15:1    yes
```