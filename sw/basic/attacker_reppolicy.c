#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#include "mmap.h"
#include "cache_info_pac.h"
#include "fpga_utils.h"
#include "statistics.h"
#include "eviction_hw.h"
#include "policy_check.h"

#define ASSERT(x) assert(x != -1)

// Print measurements ?
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
  int SW_THR,        SW_LLC, SW_RAM;
  int HW_THR, HW_L2, HW_LLC, HW_RAM;
  int seed = time (NULL); srand (seed);
  
  #define EVS_PRINT 0
  #define TEST_COUNT 10000
  int timing[200][TEST_COUNT]; uint8_t action[2000];

  //////////////////////////////////////////////////////////////////////////////
  // Macros
  #include "macros.h"
  
  //////////////////////////////////////////////////////////////////////////////
  //                      ACTUAL EXPERIMENT STARTS HERE                       //
  //////////////////////////////////////////////////////////////////////////////

  #include "policy_test.h"

  policy_check_printheader();

  int policy_index;
  // int repeat = 0;
  for (policy_index=0; policy_index<POLICY_TEST_COUNT; policy_index++) {

    ////////////////////////////////////////////////////////////////////////////
    // Construct a new EV set for each test

    // Generate TARGET randomly
    int target_index = 0;
  
new_target:
  target_index = (rand() % 64)*8;
  uint64_t TARGET = (uint64_t)&shared_mem[target_index];

    double time; int evs_len; int nb_faulty;

    // Determine HW and SW Thresholds dynamically
    CONFIGURE_THRESHOLDS_HW();
    CONFIGURE_THRESHOLDS_SW();

    uint64_t eviction_set_sw[EV_SIZE]; 
    uint64_t eviction_set_hw[EV_SIZE];

    int flags = 0;
    flags |= FPGA_CREATE_EVS_HUGE;
    // flags |= FPGA_CREATE_EVS_SMALL;
    flags |= FPGA_CREATE_EVS_EN_TEST;
    flags |= FPGA_CREATE_EVS_EN_TEST_MULTI;
    // flags |= FPGA_CREATE_EVS_EN_TEST_ONLYFIRST;
    

    time      = evs_create(target_index, HW_THR-3, WAIT, EV_SIZE, flags);
    evs_len   = evs_get(eviction_set_sw, eviction_set_hw, EVS_PRINT);
    nb_faulty = evs_verify(target_index, eviction_set_sw, HW_THR-3, evs_len, EVS_PRINT);  

    if (evs_len<EV_SIZE || nb_faulty!=0) {goto new_target;}
  
    // ////////////////////////////////////////////////////////////////////////////
    // // Clean up and prepare for test

    SW_READ(); FLUSH_A(TARGET); BUSY_WAIT(); // warm up TLB 
    // Ensure eviction set is not cached in the beginning
    for (i=0; i < EV_SIZE; i++){ m=0; FLUSH_A((uint64_t*)eviction_set_sw[i]); }
    
    ////////////////////////////////////////////////////////////////////////////
    // Now do the test for the given test index

    policy_test(
      eviction_set_sw,
      eviction_set_hw,
      pti, policy_index, HW_THR-3);
  }

  //////////////////////////////////////////////////////////////////////////////
  //                       ACTUAL EXPERIMENT ENDS HERE                        //
  //////////////////////////////////////////////////////////////////////////////

  // Shut Down
  *syncronization = -1; sleep(1); fpga_close();
}

