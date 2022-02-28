#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <numa.h>

#define __USE_GNU
#include <sched.h>

#define ATTACKER_CORE  2
#define VICTIM_CORE    8

#include <assert.h>
#define ASSERT(x) assert(x != -1)

#include "mmap.h"

volatile uint64_t *shared_mem;
volatile uint64_t *syncronization;
volatile uint64_t *syncronization_params;
char              *evict_mem;

extern void victim();
extern void attacker(int);

int set_core (int core, char *print_info);

int main(int argc,char* argv[])
{
  int core = set_core(ATTACKER_CORE, "Attacker");
  ASSERT(map_shared_mem(&shared_mem, SHARED_MEM_SIZE));
  ASSERT(map_syncronization_mem(&syncronization));
  ASSERT(map_syncronization_mem(&syncronization_params));

  *syncronization = 0;

  if (fork() == 0) {
    set_core(VICTIM_CORE, "Victim  ");

    victim();
  }
  else {
    core = set_core(ATTACKER_CORE, "Attacker");
    
    attacker(core);

    ASSERT(unmap_shared_mem(shared_mem, SHARED_MEM_SIZE));

    ASSERT(unmap_syncronization_mem(syncronization));
    ASSERT(unmap_syncronization_mem(syncronization_params));
  }
  return 0;
}

int set_core (int core, char *print_info) {

  //  get the number of cpu's
  // int numcpu = sysconf( _SC_NPROCESSORS_ONLN );
  // printf("numcpu %d\n", numcpu);

  // Define your cpu_set bit mask.
  cpu_set_t my_set;

  // Initialize it all to 0, i.e. no CPUs selected.
  CPU_ZERO(&my_set);

  // set the bit that represents core.
  CPU_SET(core, &my_set);

  // Set affinity of tihs process to the defined mask
  sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

  // get our CPU
  core  = sched_getcpu();
  int node = numa_node_of_cpu(core);
  printf("%s CPU: %2d, Node: %1d\n", print_info, core, node);

  return core;
}
