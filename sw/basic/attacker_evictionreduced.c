#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#include "mmap.h"
#include "cache_info_pac.h"
#include "fpga_utils.h"
#include "statistics.h"
#include "eviction_hw.h"

#define ASSERT(x) assert(x != -1)

// Print measurements?
#define VERBOSE 0

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
  
  #define TEST_COUNT 10000
  int timing[200][TEST_COUNT]; uint8_t action[2000];

  //////////////////////////////////////////////////////////////////////////////
  // Macros
  #include "macros.h"

  // Generate TARGET randomly
  int target_index = 0;
  int attempts=0;
new_target:
  target_index = (rand() % 64)*8;
  uint64_t TARGET = (uint64_t)&shared_mem[target_index];

  // Determine HW and SW Thresholds dynamically
  CONFIGURE_THRESHOLDS_HW();
  CONFIGURE_THRESHOLDS_SW();
  printf(GREEN "\n [+] Thresholds Configured  "NC\
               "\n\tHW_L2 : %u"\
               "\n\tHW_LLC: %u\tSW_LLC: %u"\
               "\n\tHW_RAM: %u\tSW_RAM: %u"\
               "\n\tHW_THR: %u\tSW_THR: %u\n",
                HW_L2,        
                HW_LLC,     SW_LLC,       
                HW_RAM,     SW_RAM,       
                HW_THR,     SW_THR);
  
  //////////////////////////////////////////////////////////////////////////////
  // Construct EV in HW
  double time; int evs_len; int nb_faulty;
  #define EVS_PRINT 1
  #define EV_SIZE 6

  uint64_t eviction_set_sw[EV_SIZE]; uint64_t eviction_set_hw[EV_SIZE];

  int flags = 0;
  flags |= FPGA_CREATE_EVS_HUGE;
  // flags |= FPGA_CREATE_EVS_SMALL;
  flags |= FPGA_CREATE_EVS_EN_TEST;
  flags |= FPGA_CREATE_EVS_EN_TEST_MULTI;
  // flags |= FPGA_CREATE_EVS_EN_TEST_ONLYFIRST;
  
  time      = evs_create(target_index, HW_THR-3, WAIT, EV_SIZE, flags);
  evs_len   = evs_get(eviction_set_sw, eviction_set_hw, EVS_PRINT);
  nb_faulty = evs_verify(target_index, eviction_set_sw, HW_THR-3, evs_len, EVS_PRINT);  

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
  // Clean up and prepare
  SW_READ(); FLUSH_A(TARGET); BUSY_WAIT(); // warm up TLB 
  // Ensure eviction set is not cached in the beginning
  for (i=0; i < EV_SIZE; i++){ m=0; FLUSH_A((uint64_t*)eviction_set_sw[i]); } 

  //////////////////////////////////////////////////////////////////////////////
  //                    ACTUAL EXPERIMENT STARTS HERE                         //
  //////////////////////////////////////////////////////////////////////////////

  // Ensure eviction set is not cached in the beginning
  for (i=0; i < EV_SIZE; i++) {m=0; FLUSH_A((uint64_t*)eviction_set_sw[i]);} 
  FLUSH_A(TARGET);

  int success = 0, msg;

  // Dummy example
  for (t=0; t<TEST_COUNT; t++) {
    m=0;
  
    // Reset
    FLUSH_A(TARGET);
    FLUSH_A(eviction_set_sw[0]);
    FLUSH_A(eviction_set_sw[1]);
    FLUSH_A(eviction_set_sw[2]);
    FLUSH_A(eviction_set_sw[3]);

    // Line 1
    HW_WRITE_HWA(eviction_set_hw[0]);
    HW_WRITE_HWA(eviction_set_hw[1]);
    // Line 2
    SW_WRITE_A(eviction_set_sw[0]);
    SW_WRITE_A(eviction_set_sw[1]);
    // Line 3
    HW_WRITE_HWA(eviction_set_hw[0]);
    HW_WRITE_HWA(eviction_set_hw[1]);
    // Line 4
    SW_READ_A(TARGET);
    // Line 5
    msg = rand() % 2; 
    if (msg)
      VICTIM_READ(TARGET);
    // Line 6
    HW_WRITE_HWA(eviction_set_hw[2]);
    HW_WRITE_HWA(eviction_set_hw[3]);
    // Line 7
    SW_READ_A(TARGET);
    if      ( msg && timing[m-1][t]>SW_THR) success++;
    else if (!msg && timing[m-1][t]<SW_THR) success++;
    // Line 8
    SW_WRITE_A(eviction_set_sw[2]);
    SW_WRITE_A(eviction_set_sw[3]);
    // Line 9
    HW_WRITE_HWA(eviction_set_hw[2]);
    HW_WRITE_HWA(eviction_set_hw[3]);

    // Line 10
    msg = rand() % 2;
    if (msg)
      VICTIM_READ(TARGET);
    // Line 11
    HW_WRITE_HWA(eviction_set_hw[0]);
    HW_WRITE_HWA(eviction_set_hw[1]);
    // Line 12
    SW_READ_A(TARGET);
    if      ( msg && timing[m-1][t]>SW_THR) success++;
    else if (!msg && timing[m-1][t]<SW_THR) success++;
    // Line 13
    SW_WRITE_A(eviction_set_sw[0]);
    SW_WRITE_A(eviction_set_sw[1]);
    // Line 14
    HW_WRITE_HWA(eviction_set_hw[0]);
    HW_WRITE_HWA(eviction_set_hw[1]);
  }

  printf(GREEN"\n [+] Experiment is completed\n"NC);

  printf("\n\tSuccess %d/%d\n", success, 2*TEST_COUNT);

  //////////////////////////////////////////////////////////////////////////////
  //                    ACTUAL EXPERIMENT ENDS HERE                           //
  //////////////////////////////////////////////////////////////////////////////
quit:
  // Shut Down
  *syncronization = -1;
  sleep(1);
  fpga_close();

  //////////////////////////////////////////////////////////////////////////////
  // Print measurements

  #if VERBOSE == 1
  printf("\n\n");

  print_statistics_header();
  for(i=0; i<m; i++)
    print_statistics(timing[i], TEST_COUNT, action[i], i);
  #endif
}
