#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <numa.h>
#include <time.h>
#include <inttypes.h>

#define __USE_GNU
#include <sched.h>

#include <assert.h>
#define ASSERT(x) assert(x != -1)

#include "mmap.h"
#include "fpga_utils.h"
#include "cache_info.h"
#include "cache_utils.h"
#include "statistics.h"
#include "eviction_hw.h"


#define ATTACKER_CORE  0
#define STRESS_COMMAND "taskset --cpu-list 2,4,6,8,10,12,14,18,20,22,24,26,28,30 stress -m %c & sleep 2"

#define MSRTOOLS_PATH "~/tools/intel/msr-tools/"
#define DEFAULT_D 2


////////////////////////////////////////////////////////////////////////////////
// Function Declarations

int setDDIO(int count);

int set_core (int core, char *print_info);

////////////////////////////////////////////////////////////////////////////////

uint64_t shared_mem_iova;
volatile uint64_t *shared_mem;

int main(int argc,char* argv[])
{

  set_core(ATTACKER_CORE, "Attacker");
  ASSERT(map_shared_mem(&shared_mem, SHARED_MEM_SIZE));

  //////////////////////////////////////////////////////////////////////////////
  // Get the command line arguments

  char arg_stress = '0'; if (argc > 1) arg_stress = argv[1][0];
  char arg_page   = 'H'; if (argc > 2) arg_page   = argv[2][0];
  char arg_config = 'M'; if (argc > 3) arg_config = argv[3][0];
  int  arg_D      =  2 ; if (argc > 4) arg_D = (int)strtol(argv[4], NULL, 10);

    
  printf("Args: Stress %c Page %c Config %c D %d\n", 
    arg_stress, arg_page, arg_config, arg_D);
  
  //////////////////////////////////////////////////////////////////////////////
  // Connect to FPGA

  ASSERT(fpga_connect_to_accel(AFU_ACCEL_UUID));
  fpga_share_buffer(shared_mem, SHARED_MEM_SIZE, &shared_mem_iova);

  printf("  VA : 0x%0lx\n", (uint64_t)shared_mem              );
  printf("  PA : 0x%0lx\n", (uint64_t)shared_mem_iova         );
  printf(" CLA : 0x%0lx\n", (uint64_t)shared_mem_iova / CL(1) );

  //////////////////////////////////////////////////////////////////////////////
  // Define variables

  int seed = time (NULL); srand (seed);

  uint8_t action[200];
  FILE *fptr;
  int z, m, i;
  #include "macros.h"

  for (i=0; i<40000; i++)
    BUSY_WAIT();

  //////////////////////////////////////////////////////////////////////////////
  // Test with creating eviciton set with different configurations

  // Each experiment with different target_index'es
  int target_index = 0;
  int threshold = evs_find_threshold(target_index);

  int flags = 0;
  int wrap_limit;

  // Invoke stress app

  char cmd_stress[80];
  sprintf(cmd_stress, STRESS_COMMAND, arg_stress);

  if (arg_stress != '0') {
    (void)!system(cmd_stress);
    printf("Stress with command: %s\n", cmd_stress);
  }
  else
    printf("Continue with no stress\n");

  // Apply page configuration according to arguments
  if (arg_page == 'S')  {flags |= FPGA_CREATE_EVS_SMALL; wrap_limit =  10*16;}
  else                  {flags |= FPGA_CREATE_EVS_HUGE;  wrap_limit = 100*16;}
  // default: huge pages

  // Apply test configuration according to arguments
  if      (arg_config == 'S')   flags |= FPGA_CREATE_EVS_EN_TEST;
  else if (arg_config == 'M') { flags |= FPGA_CREATE_EVS_EN_TEST;
                                flags |= FPGA_CREATE_EVS_EN_TEST_MULTI; }
  // default: no test at evset construction

  int multitest_count = 2;
  int retry_limit = 3;
  double time;
  int fail;

retry:
  // Apply the configuration according to arguments
  if (arg_D!=DEFAULT_D)
    if (!setDDIO(arg_D)) goto error_ddio;
  for (i=0; i<40000; i++) BUSY_WAIT();

  // Generate eviction set
  time = evs_create(target_index, threshold, WAIT, arg_D, wrap_limit, multitest_count, flags);
  fail = evs_check_error(flags, wrap_limit, EVS_VERBOSE);

  if (fail != 1 && time != -1) {

    if (arg_stress != '0') {
      (void)!system("killall stress");
      for (i=0; i<40000; i++) BUSY_WAIT();
    }

    uint64_t evset_sw[LLC_WAYS];
    uint64_t evset_hw[LLC_WAYS];

    int evs_len = evs_get(evset_sw, evset_hw, EVS_VERBOSE);

    // Revert to 2 DDIO ways
    if (arg_D!=DEFAULT_D)
      if (!setDDIO(2)) goto error_ddio;
    for (i=0; i<40000; i++) BUSY_WAIT();

    int nb_faults_Old_WF = 
      evs_verify(target_index, threshold, evset_sw, evs_len, EVS_VERBOSE);

    printf("\nStress %c Page %c Checks %c D %d  \n\nTime %.2f ms. Error Count: %d \n",
      arg_stress, arg_page, arg_config, arg_D,
      time, nb_faults_Old_WF);
  }
  else {
    if (retry_limit-- > 0) {
      printf("Attempt failed. Retry.\n");
      goto retry;
    }
    else 
      goto error_const;
      
  }

goto finalize;

////////////////////////////////////////////////////////////////////////////////
// Errors
  
error_ddio:
  printf("Error: Requested DDIO configuration failed!\n");
  goto finalize;
error_const:
  printf("Error: EvSet Construction failed!\n");

////////////////////////////////////////////////////////////////////////////////
// Finalize

  (void)!system("killall stress");

finalize:

  // Restore the DDIO way configuration
  if (arg_D!=DEFAULT_D)
    assert(setDDIO(2));

  fpga_close();
  printf("\nFPGA Closed.\n");


  ASSERT(unmap_shared_mem(shared_mem, SHARED_MEM_SIZE));
  
  return (fail != 1 && time != -1) ? 0 : -1;
}

int set_core (int core, char *print_info) {

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
  // printf("%s CPU: %2d, Node: %1d\n", print_info, core, node);

  return core;
}

int setDDIO(int count) {

  int requestedConfig=0, observedConfig;

  // Calculate the hex value for the number of DDIO ways requestes

  int i;
  for (i=0; i<count; i++)
    requestedConfig |= (0x1<<(LLC_WAYS-1)) >> i;

  // Write the msr value with the requested DDIO way configuration
  char wrmsr_cmd[128];
  sprintf(wrmsr_cmd, "%s/wrmsr 0xc8b 0x%x", MSRTOOLS_PATH, requestedConfig);
  assert(system(wrmsr_cmd) == 0);
  // printf("%s\n", wrmsr_cmd);

  // Read the msr value after the configuration
  char rdmsr_cmd[128];
  sprintf(rdmsr_cmd, "%s/rdmsr 0xc8b", MSRTOOLS_PATH);
  FILE *cmd = popen(rdmsr_cmd, "r");
  char result[24]={0x0};
  while (fgets(result, sizeof(result), cmd)!=NULL);
  pclose(cmd);
  observedConfig = (int)strtol(result, NULL, 16);

  // Return with a comparison of the written and read configurations

  return (observedConfig==requestedConfig);
}
