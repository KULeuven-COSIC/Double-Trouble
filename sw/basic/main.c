#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sched.h>
#include <numa.h>

// CPU    | CORE 
// 0 / 16 |  0  
// 2 / 18 |  2  
// 4 / 20 |  4  
// 6 / 22 |  6  
// 8 / 24 |  8  
// 0 / 26 | 10  
// 2 / 28 | 12  
// 4 / 30 | 14  

#define ATTACKER_CORE         0
#define ATTACKER_HELPER_CORE  2
#define VICTIM_CORE           4

#define ATTACKER_HELPER 1

#include <assert.h>
#define ASSERT(x) assert(x != -1)

#include "mmap.h"
uint64_t *shared_mem;
uint64_t *evict_mem;
volatile uint64_t *syncronization;
volatile uint64_t *syncronization_params;

void set_core (int core, char *print_info);

void attacker();
void attacker_helper();
void victim(); 


int main(void)
{

  set_core(ATTACKER_CORE, "Attacker");
  ASSERT(map_shared_mem(&shared_mem, SHARED_MEM_SIZE));
  ASSERT(map_syncronization_mem(&syncronization));
  ASSERT(map_syncronization_mem(&syncronization_params));

  *shared_mem = 1;
  *syncronization = 0;

  printf("\n============================== \n");

  if (fork() == 0) {

    #if ATTACKER_HELPER == 1
      if (fork() == 0){
        set_core(VICTIM_CORE, "Victim  ");
        victim();
      }
      else {
        set_core(ATTACKER_HELPER_CORE, "Helper  ");
        attacker_helper();
      }
    #else 
      set_core(VICTIM_CORE, "Victim  ");
      victim();
    #endif
  } 
  else {
    
    int t, i;
    for (t=0; t<40000; t++)
      for (i=30000; i>0; i--)
        asm volatile("nop;");    

    set_core(ATTACKER_CORE, "Attacker");

    attacker();
    ASSERT(unmap_shared_mem(shared_mem, SHARED_MEM_SIZE));
    ASSERT(unmap_syncronization_mem(syncronization));
    ASSERT(unmap_syncronization_mem(syncronization_params));
  }
  return 0;
}

void set_core (int core, char *print_info) {
  
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
  int cpu  = sched_getcpu();
  int node = numa_node_of_cpu(cpu);
  printf(" %s CPU: %2d, Node: %1d\n", print_info, cpu, node);
}
