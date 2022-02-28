#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "mmap.h"
#include "cache_info_pac.h"
#include "fpga_utils.h"
#include "statistics.h"
#include "eviction_hw.h"

#define ASSERT(x) assert(x != -1)

// Print measurements?
#define VERBOSE 1
#define WRITE_LOGS 1

// Buffers
uint64_t shared_mem_iova;
extern volatile uint64_t *shared_mem;
extern volatile uint64_t *syncronization;
extern volatile uint64_t *syncronization_params;

void attacker() {  

  //////////////////////////////////////////////////////////////////////////////
  // Connect to FPGA

  ASSERT(fpga_connect_to_accel(AFU_ACCEL_UUID));
  fpga_share_buffer(shared_mem, SHARED_MEM_SIZE, &shared_mem_iova);


  //////////////////////////////////////////////////////////////////////////////
  // Variables
  
  uint64_t hw_read_addr, sw_read_addr;
  int t, m, i;
  int HW_THR, HW_RAM, HW_L2, HW_LLC, SW_LLC, SW_RAM, SW_THR;
  int seed = time (NULL); srand (seed);

  int target_index = 0;
  uint64_t TARGET;
  
  #define TEST_COUNT 10000
  int timing[200][TEST_COUNT]; uint8_t action[2000];


  //////////////////////////////////////////////////////////////////////////////
  // Macros
  #include "macros.h"


  //////////////////////////////////////////////////////////////////////////////
  // Pick a target memory address, for which an eviction set will be created

  int attempts=0;
new_target:
  target_index = (rand() % 64)*8;
  TARGET = (uint64_t)&shared_mem[target_index];


  //////////////////////////////////////////////////////////////////////////////
  // Explore memory access thresholds

  CONFIGURE_THRESHOLDS_HW();
  CONFIGURE_THRESHOLDS_SW();
  printf(GREEN "\n [+] Thresholds Configured"NC\
               "\n\tHW_L2 : %u"\
               "\n\tHW_LLC: %u\tSW_LLC: %u"\
               "\n\tHW_RAM: %u\tSW_RAM: %u"\
               "\n\tHW_THR: %u\tSW_THR: %u\n",
                HW_L2,        
                HW_LLC,     SW_LLC,       
                HW_RAM,     SW_RAM,       
                HW_THR,     SW_THR);
  

  //////////////////////////////////////////////////////////////////////////////
  // Construct EV with hardware accelerator
  
  #define EVS_PRINT 1
  #define EV_SIZE 11

  uint64_t evset_sw[EV_SIZE]; 
  uint64_t evset_hw[EV_SIZE];

  int flags = 0;
  flags |= FPGA_CREATE_EVS_HUGE;
  // flags |= FPGA_CREATE_EVS_SMALL;
  flags |= FPGA_CREATE_EVS_EN_TEST;
  flags |= FPGA_CREATE_EVS_EN_TEST_MULTI;
  // flags |= FPGA_CREATE_EVS_EN_TEST_ONLYFIRST;
  
  double time      = evs_create(target_index, HW_THR-3, WAIT, EV_SIZE, flags);
  int    evs_len   = evs_get(evset_sw, evset_hw, EVS_PRINT);
  int    nb_faulty = evs_verify(target_index, evset_sw, HW_THR-3, evs_len, EVS_PRINT);  

  if (evs_len<EV_SIZE || nb_faulty!=0) {
    printf(RED"\n [+] EV Set Construction Failed!\n");

    attempts++;
    if (attempts>10) {
      printf(RED"\n [+] Quit After Too Many Failed Evset Construction\n");
      goto quit;
    }
    goto new_target;
  }

  printf(GREEN"\n [+] EV Set Constructed "NC\
              "\n \tSize: %u\n\tTime: %.1f ms\n", EV_SIZE, time);


  //////////////////////////////////////////////////////////////////////////////
  // Clean up and prepare for experiments

  // Warm up TLB 
  SW_READ(); 
  FLUSH_A(TARGET); 
  BUSY_WAIT(); 

  // Ensure eviction set is not cached in the beginning
  for (i=0; i < EV_SIZE; i++) { m=0; FLUSH_A((uint64_t*)evset_sw[i]); } 


  /////////////////////////////////////////////////////////////////////////////
  // Recipe based access demonstration
  
  // Prepare recipes
  // The first  recipe accesses evset[0], evset[0], evset[1]
  // The second recipe accesses evset[4], evset[4], evset[5]
  int recipe0[] = {0,0,1};  evs_recipe_prepare(recipe0, 3, 0);
  int recipe1[] = {4,4,5};  evs_recipe_prepare(recipe1, 3, 1);
  
  // Perform write accesses with the recipe0
  evs_recipe_write(0);
  evs_recipe_write(0);

  // Perform read accesses with the recipes
  // Each recipe read returns how many of the reads are executed slow  
  int cnt_slow;
  cnt_slow = evs_recipe_read(0); printf("Slow R0 access count: %d\n", cnt_slow);
  cnt_slow = evs_recipe_read(1); printf("Slow R1 access count: %d\n", cnt_slow);

  // Because of the above write accesses to recipe0, 
  // we expect that recipe0 reads are fast, while recipe1 reads are slow.
  
  /////////////////////////////////////////////////////////////////////////////
  // Access time measurements

  // The following code takes access timing measurements with the functions
  // from macros.h, and stores them `timing[m][t]` array. 
  // Later, they will be printed as a table, if VERBOSE macro is set.
  
  // Ensure eviction set is not cached in the beginning
  for (i=0; i < EV_SIZE; i++) { m=0; FLUSH_A((uint64_t*)evset_sw[i]); } 

  // Dummy example
  for (t=0; t<TEST_COUNT; t++) {
    m=0;

    FLUSH_A(TARGET);    
    SW_READ_A(TARGET);  // CPU's RAM Access
    SW_READ_A(TARGET);  // CPU's L1 Access

    FLUSH_A(TARGET);
    HW_WRITE_A(TARGET);
    SW_READ_A(TARGET);  // CPU's LLC Access

    FLUSH_A(TARGET);
    HW_READ_A(TARGET);  // HW's RAM Access

    FLUSH_A(TARGET);
    HW_WRITE_A(TARGET);
    HW_READ_A(TARGET);  // HW's LLC Access
  }


  //////////////////////////////////////////////////////////////////////////////
  // Prepare for shutdown: close the FPGA, and kill the victim app
quit:
  *syncronization = -1; 
  sleep(1); 
  fpga_close();


  //////////////////////////////////////////////////////////////////////////////
  // Print measurements

#if VERBOSE == 1
  printf(GREEN"\n [+] Access Timing Measurements are Taken \n\n"NC);

  print_statistics_header();
  for(i=0; i<m; i++)
    print_statistics(timing[i], TEST_COUNT, action[i], i);
#endif


  //////////////////////////////////////////////////////////////////////////////
  // Write measurements to a log file

#if WRITE_LOGS == 1
  // Write statistics to log file
  FILE *fp_stat = fopen ("./log/statistix.log","w");
  for(t=0; t<m; t++) {
    fprintf(fp_stat, "%2d ", t);
    write_statistics(timing[t], TEST_COUNT, fp_stat, action[t]);
  }
  fclose(fp_stat);

  // Write measurements to log file
  FILE * fp;
  fp = fopen ("./log/measurements.log","w");
  for (i=0; i<m; i++) {
    for(t=0; t<TEST_COUNT; t++){
      fprintf (fp, "%d ", timing[i][t]);
    }
    fprintf (fp, "\n");
  }
  fclose (fp);
#endif

}
