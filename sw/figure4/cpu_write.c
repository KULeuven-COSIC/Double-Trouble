#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "mmap.h"
#include "cache_info_pac.h"
#include "colors.h"
#include "fpga_utils.h"
#include "eviction_hw.h"

#define ASSERT(x) assert(x != -1)

/////////////////////////////////////
// Print measurements?
#define VERBOSE 0
#define WRITE_LOGS 1

// Buffers
uint64_t shared_mem_iova;
extern volatile uint64_t *shared_mem;
extern volatile uint64_t *syncronization;
extern volatile uint64_t *syncronization_params;
////////////////////////////////////////////////////////////////////////////////

void attacker() {  

  //////////////////////////////////////////////////////////////////////////////
  // Connect to FPGA
  ASSERT(fpga_connect_to_accel(AFU_ACCEL_UUID));
  fpga_share_buffer(shared_mem, SHARED_MEM_SIZE, &shared_mem_iova);

  //////////////////////////////////////////////////////////////////////////////
  // Variables
  uint64_t hw_read_addr, sw_read_addr;
  int t, m, i;
  int HW_THR, HW_RAM, HW_L2, HW_LLC, SW_LLC, SW_RAM, SW_THR, SW_RL2;
  int seed = time (NULL); srand (seed);
  
  #define TEST_COUNT 10000
  int timing[200][TEST_COUNT]; uint8_t action[2000];
  int timeHW; int timeSW; int timeVictim; int timeHelper;

  //////////////////////////////////////////////////////////////////////////////
  // Macros
  #include "macros.h"

  // Platform Detection
  DETECT_PLATFORM();

  // Generate TARGET randomly
  int target_index = 0;
  int attempts=0;
new_target:
  target_index = (rand() % 64)*8;
  uint64_t TARGET = (uint64_t)&shared_mem[target_index];

  // Determine HW and SW Thresholds dynamically
  CONFIGURE_THRESHOLDS_HW();
  CONFIGURE_THRESHOLDS_SW();
  printf(GREEN"\n [+] Thresholds Configured  "NC"\n\tHW_L2:  %u\tSW_RL2: %u\n\tHW_LLC: %u\tSW_LLC: %u\n\tHW_RAM: %u\tSW_RAM: %u\n\tHW_THR: %u\tSW_THR: %u\n",
                                                     HW_L2,      SW_RL2,       HW_LLC,     SW_LLC,       HW_RAM,     SW_RAM,       HW_THR,     SW_THR);
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Construct EV in HW
  double time; int evs_len; int nb_faulty;
  #define EVS_PRINT 0

  uint64_t eviction_set_sw[EV_SIZE]; uint64_t eviction_set_hw[EV_SIZE];

  time      = evs_create(target_index, HW_THR-3, WAIT);
  evs_len   = evs_get(eviction_set_sw, eviction_set_hw, EVS_PRINT);
  nb_faulty = evs_verify(target_index, eviction_set_sw, evs_len, EVS_PRINT);  

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

  /////////////////////////////////////
  // Clean up and prepare
  SW_READ(); FLUSH_A(TARGET); BUSY_WAIT(); // warm up TLB 
  for (i=0; i < EV_SIZE; i++){ m=0; FLUSH_A((uint64_t*)eviction_set_sw[i]); } // Ensure eviction set is not cached in the beginning
  int j;


  /////////////////////////////////////////////////////////////////////////////
  //                   ACTUAL EXPERIMENT STARTS HERE                         //
  /////////////////////////////////////////////////////////////////////////////
  #define ITERATIONS 1000

  printf("\n What happens to lines in the LLC when written by a CPU core?\n");

  ///// Where is the line from HW PoV? /////
  for (t=0; t < ITERATIONS; t++){
    m=0;
    HW_WRITE();                    // Flush target line
    SW_WRITE();                    // Read target line from Core A

    HW_READ();                     // Read target line from FPGA
  }

  qsort(timing[0], ITERATIONS, sizeof(int), compare);
  timeHW = timing[0][ITERATIONS/2];
  printf(BLUE" [+] HW access time says \t%3u (%s)\n"NC, timeHW, timeHW >= HW_RAM ? "RAM" : (timeHW < HW_L2 ? "LLC" : "L2"));

  ///// Where is the line from SW PoV? /////
  for (t=0; t < ITERATIONS; t++){
    m=0;
    HW_WRITE();                    // Flush target line
    SW_WRITE();                    // Read target line from Core A

    SW_READ_A_BENCHMARK();         // Read target line from Core A
  }

  qsort(timing[0], ITERATIONS, sizeof(int), compare);
  timeSW = timing[0][ITERATIONS/2];
  printf(BLUE" [+] SW access time says \t%3u (%s)\n"NC, timeSW, timeSW >= SW_RAM ? "RAM" :
                                                               (timeSW < SW_LLC  ? "L2"  :
                                                               (timeSW >= SW_RL2 ? "Remote L2" :
                                                                                   "LLC")));

  ///// Where is the line from other core PoV? /////
  for (t=0; t < ITERATIONS; t++){
    m=0;
    HW_WRITE();                    // Flush target line
    SW_WRITE();                    // Read target line from Core A

    VICTIM_READ(TARGET);           // Read target line from Core B
  }

  qsort(timing[0], ITERATIONS, sizeof(int), compare);
  timeVictim = timing[0][ITERATIONS/2];
  printf(BLUE" [+] Other core time says \t%3u (%s)\n\n"NC, timeVictim,  timeVictim >=  SW_RAM ? "RAM" :
                                                                     (timeVictim <  SW_LLC ? "L2"  :
                                                                     (timeVictim >= SW_RL2 ? "Remote L2" :
                                                                                             "LLC")));
  
  /////////////////////////////////////////////////////////////////////////////
  //                   ACTUAL EXPERIMENT ENDS HERE                           //
  /////////////////////////////////////////////////////////////////////////////

  // Shut Down
quit: 
  *syncronization = -1; 
  sleep(1); 
  fpga_close();

}
