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

#define ASSERT(x) assert(x != -1)

/////////////////////////////////////
// Determine experiment

uint64_t shared_mem_iova;
extern volatile uint64_t *shared_mem;
extern volatile uint64_t *syncronization;
extern volatile uint64_t *syncronization_params;

////////////////////////////////////////////////////////////////////////////////

void attacker() {  

  // Connect to FPGA
  ASSERT(fpga_connect_to_accel(AFU_ACCEL_UUID));
  fpga_share_buffer(shared_mem, SHARED_MEM_SIZE, &shared_mem_iova);

  //////////////////////////////////////////////////////////////////////////////
  // Variables
  int HW_THR, HW_RAM, HW_L2, HW_LLC, SW_LLC, SW_RAM, SW_THR;
  int seed = time (NULL); srand (seed);
  
  //////////////////////////////////////////////////////////////////////////////
  // Variables
  uint64_t hw_read_addr, sw_read_addr;
  int t, m, i, j;
  int target_index = 0;
  
  #define TEST_COUNT 10000
  int timing[20][TEST_COUNT]; uint8_t action[20];
  uint64_t* base; uint8_t hw_count, sw_count; int hw_it;
  int it = 0; int correct = 0; int delta1 = 0; int timeNow; int timeHW; int timeSW; double timespan;

  //////////////////////////////////////////////////////////////////////////////
  // Macros
  #include "macros.h"

  // Generate TARGET randomly
  int attempts=0;
new_target: 
  while (target_index < 24) {target_index = (rand() % 64)*8;} // Somehow indices 0, 8, 16 do not terminate
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

  //////////////////////////////////////////////////////////////////////////////
  // Construct EV in HW
  double time; int evs_len; int nb_faulty;
  #define EVS_PRINT 0
  #define EV_SIZE 11

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
    if (attempts>0) {
      printf(RED"\n [+] Quit After Too Many Failed Evset Construction\n");
      goto quit;
    }
    goto new_target;
  }

  printf(GREEN"\n [+] EV Set Constructed "NC\
              "\n \tSize: %u\n\tTime: %.1f ms\n", EV_SIZE, time);

  //////////////////////////////////////////////////////////////////////////////

  SW_READ(); FLUSH_A(TARGET); BUSY_WAIT(); // warm up TLB 
  for (i=0; i < EV_SIZE; i++){ FLUSH_A((uint64_t*)eviction_set_sw[i]); } // Ensure eviction set is not cached in the beginning

  printf(GREEN"\n [+] Experiment results:\n"NC);

  ///////////////////////////////////////
  // Formal Experiment for Section 3.2.2
  ///////////////////////////////////////
  #define EXP_COUNT TEST_COUNT

  m=0;
  FLUSH_A(TARGET); BUSY_WAIT();

  /////////////
  // NO ACTION
  /////////////
  int targetEvicted = 0;
  for (it = 0; it < EXP_COUNT; it++){
    m=0;

    FLUSH_A(TARGET); FLUSH_A(eviction_set_sw[0]); FLUSH_A(eviction_set_sw[1]);
    
    // Access TARGET
    HW_WRITE_A((uint64_t*) TARGET); 

    // Access X1 (= first congruent address)
    for (i=0; i<3; i++) 
      HW_WRITE_A((uint64_t*) eviction_set_sw[0]);

    // NO ACTION
    
    // Access X2 (= first congruent address)
    HW_WRITE_A((uint64_t*)eviction_set_sw[1]);

    HW_READ_A((uint64_t*) TARGET); 
    if (timing[m-1][t] > HW_THR) targetEvicted++;

  }

  printf("\n\tNO ACTION | Target is evicted: %5u/%u\n", targetEvicted, EXP_COUNT);

  //////////////
  // 20 READS
  //////////////
  targetEvicted = 0;
  for (it = 0; it < EXP_COUNT; it++){
    m=0;

    FLUSH_A(TARGET); FLUSH_A(eviction_set_sw[0]); FLUSH_A(eviction_set_sw[1]);
    
    // Access TARGET
    HW_WRITE_A((uint64_t*) TARGET); 

    // Access X1 (= first congruent address)
    for (i=0; i<3; i++)
      HW_WRITE_A((uint64_t*) eviction_set_sw[0]);

    // Read TARGET 10 times
    for (i=0; i<10; i++) 
      HW_READ_A((uint64_t*) TARGET);
    
    // Access X2 (= first congruent address)
    HW_WRITE_A((uint64_t*) eviction_set_sw[1]);

    HW_READ_A((uint64_t*) TARGET); 
    if (timing[m-1][t] > HW_THR) targetEvicted++;

  }

  printf("\t10 READS  | Target is evicted: %5u/%u\n", targetEvicted, EXP_COUNT);

  //////////////
  // FEW WRITES
  //////////////
  targetEvicted = 0;
  for (it = 0; it < EXP_COUNT; it++){
    m=0;

    FLUSH_A(TARGET); FLUSH_A(eviction_set_sw[0]); FLUSH_A(eviction_set_sw[1]);
    
    // Access TARGET
    HW_WRITE_A((uint64_t*) TARGET); 

    // Access X1 (= first congruent address)
    for (i=0; i<3; i++) 
      HW_WRITE_A((uint64_t*) eviction_set_sw[0]);

    // Write TARGET 10 times
    for (i=0; i<10; i++) 
      HW_WRITE_A((uint64_t*) TARGET);
    
    // Access X2 (= first congruent address)
    HW_WRITE_A((uint64_t*) eviction_set_sw[1]);

    HW_READ_A((uint64_t*) TARGET); 
    if (timing[m-1][t] > HW_THR) targetEvicted++;

  }

  printf("\t10 WRITES | Target is evicted: %5u/%u\n", targetEvicted, EXP_COUNT);

quit:
  // Shut Down
  *syncronization = -1; 
  sleep(1); 
  fpga_close();
}
