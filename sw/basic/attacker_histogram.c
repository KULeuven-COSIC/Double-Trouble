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

// Print measurements ?
#define VERBOSE 1
#define WRITE_LOGS 1 // Needed for histogram extraction with python script

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
  int timing[200][TEST_COUNT]; uint8_t action[200];

  //////////////////////////////////////////////////////////////////////////////
  // Macros
  #include "macros.h"

  // Generate TARGET randomly
  int target_index = (rand() % 64)*8;
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
  // Clean up and prepare
  SW_READ_A(TARGET); FLUSH_A(TARGET); BUSY_WAIT(); // warm up TLB 

  //////////////////////////////////////////////////////////////////////////////
  //                   ACTUAL EXPERIMENT STARTS HERE                          //
  //////////////////////////////////////////////////////////////////////////////

  for (t=0; t<TEST_COUNT; t++) {
    m=0;

    FLUSH_A   (TARGET);
    SW_READ_A (TARGET);
    FLUSH_A   (TARGET);
    HW_WRITE_A(TARGET);
    SW_READ_A (TARGET);
    SW_READ_A (TARGET);
    
    FLUSH_A   (TARGET);
    HW_READ_A (TARGET);
    FLUSH_A   (TARGET);
    HW_WRITE_A(TARGET);
    HW_READ_A (TARGET);
    FLUSH_A   (TARGET);
    SW_READ_A (TARGET);
    HW_READ_A (TARGET);
  }

  //////////////////////////////////////////////////////////////////////////////
  //                   ACTUAL EXPERIMENT ENDS HERE                            //
  //////////////////////////////////////////////////////////////////////////////

  // Shut Down
  *syncronization = -1; sleep(1); fpga_close();

  //////////////////////////////////////////////////////////////////////////////
  // Print measurements

  #if VERBOSE == 1
  printf("\n\n");

  print_statistics_header();
  for(i=0; i<m; i++)
    print_statistics(timing[i], TEST_COUNT, action[i], i);

  #endif

  //////////////////////////////////////////////////////////////////////////////
  // Create log files

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
