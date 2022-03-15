#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include "fpga_utils.h"

#define WAIT        5
#define LLC_WAYS   11

#define EVS_VERBOSE 1
#define EVS_NONVERB 0

#define ACCURACY_TEST       0
#define PERF_SWEEP_TEST     1
#define PERF_CHECK_TEST     2
#define POLICY_TEST         3
#define FAST_EVICTION_TEST  4

#define WITH_SECOND_TEST    0

////////////////////////////////////////////////////////////////////////////////
// Basic macro definitions to make the code self contained

#define HW_RD(x)  ({                                         \
  hw_addr = ((x - shared_mem)*8 + shared_mem_iova) / CL(1);  \
  fpga_write(CSR_TARGET_ADDRESS, hw_addr);                   \
  fpga_write(CSR_CONTROL, FPGA_READ);                        \
  while (fpga_read(CSR_CONTROL)==0);                         })

#define HW_WR(x)  ({                                         \
  hw_addr = ((x - shared_mem)*8 + shared_mem_iova) / CL(1);  \
  fpga_write(CSR_TARGET_ADDRESS, hw_addr);                   \
  fpga_write(CSR_CONTROL, FPGA_WRITE_BLOCK);                 \
  while (fpga_read(CSR_CONTROL)==0);                         })
  
#define FLSH(x)   ({                                         \
  flush((void*)x);                                           })


double evs_create(int target_index, int threshold, int wait, int ddio_way_count, int wrap_count, int multitest_count, int flags) {

  // Get the target's HW address
  uint64_t addr_sw_target = (uint64_t)&shared_mem[target_index];
  uint64_t addr_hw_target = addr_sw_to_hw(addr_sw_target);

  // Get the search space's HW address
  uint64_t addr_sw_search = (uint64_t)&shared_mem[0];
  uint64_t addr_hw_search = addr_sw_to_hw(addr_sw_search);
  
  //////////////////////////////////////////////////////////////////////////////
  // Configure the HW
  fpga_write(CSR_TARGET_ADDRESS, addr_hw_target );
  fpga_write(CSR_SEARCH_ADDRESS, addr_hw_search );
  fpga_write(CSR_DDIOCOUNT,      (((multitest_count<<16) | (wrap_count/16)<<8) | ddio_way_count));
  fpga_write(CSR_THRESH,         threshold      );
  fpga_write(CSR_WAIT,           wait           );
  fpga_write(CSR_EVS_ADDR,       LLC_WAYS       );

  // A dummy operation to have the TLB initialized
  fpga_write(CSR_CONTROL, FPGA_READ);
  while (fpga_read(CSR_CONTROL)==0);

  //////////////////////////////////////////////////////////////////////////////
  // Start eviction set creation

  struct timespec tstart={0,0}, tend={0,0};  

  clock_gettime(CLOCK_MONOTONIC, &tstart);
  
  fpga_write(CSR_CONTROL, flags);
  while (fpga_read(CSR_CONTROL)==0);
    
  clock_gettime(CLOCK_MONOTONIC, &tend);
  
  double timespan = ((double)tend.tv_sec  *1.0e3 + tend.tv_nsec  *1.0e-6) -
                    ((double)tstart.tv_sec*1.0e3 + tstart.tv_nsec*1.0e-6) ;
  
  // Return time
  return timespan;
}

int evs_check_error(int flags, int wrap_count, int printFLAG) {

  uint64_t debug = fpga_read(CSR_DEBUG0);
  
  int counter_wrap = ((debug>>32) & 0xFF) * 16;
  int evs_count  = (int)(debug & 0xFF);

  // if (printFLAG) {
  //   printf("Check variables:\n");
  //   printf(" Debug Reg    0x%" PRIx64 "\n", debug);
  //   printf(" Wrap Count   %d\n", counter_wrap);
  //   printf(" EVS Len      %d\n", evs_count);
  // }
    
  if (evs_count < 11)
    return 1;

  if (counter_wrap >= wrap_count)
    return 1;

  return 0;
}

int evs_get(uint64_t* eset_sw, uint64_t* eset_hw, int printFLAG) {
  
  uint64_t debug = fpga_read(CSR_DEBUG0);
  int evs_count = (int)(debug & 0xFF);
  int evs_mem_index = 0;
  int evs_len;

  if (printFLAG) {
    printf(
" Evset Elem | HW ADDR     SW ADDR           |Retries|Access Time|\n"\
" -----------|-------------------------------|-------|-----------|\n");
  }

  do {    
    fpga_write(CSR_EVS_ADDR, evs_mem_index);

    uint64_t tmp  = fpga_read(CSR_DEBUG1);
    int cnt_total = (int)((tmp & 0x00FFFFF000000000) >> 36);  // 20-bit
    int cnt_loop  = (int)((tmp & 0x0000000FF0000000) >> 28);  //  8-bit
    int cnt_fails = (int)((tmp & 0x000000000FF00000) >> 20);  //  8-bit
    int cnt_memop = (int)((tmp & 0x00000000000FFF00) >>  8);  // 12-bit    
    int cnt_evs   = (int)((tmp & 0x00000000000000FF)      );  //  8-bit
    
    uint64_t evs_hw_addr = fpga_read(CSR_EVS_DATA);
    eset_hw[cnt_evs] = evs_hw_addr;
    eset_sw[cnt_evs] = addr_hw_to_sw(evs_hw_addr);

    evs_len = cnt_evs;

    if (printFLAG) {
      printf(" %10d | 0x%08lx 0x%016lx | %5d | %9d |\n", 
        cnt_evs,      
        eset_hw[cnt_evs],
        eset_sw[cnt_evs],
        cnt_fails,
        cnt_memop);
    }

    evs_mem_index++;
  } while(evs_mem_index < evs_count);

  return evs_len+1;
}

int evs_verify(int target_index, int threshold, uint64_t* eset_sw, int len, int printFLAG) {
  uint64_t hw_addr;
  
  int nb_fault = 0;
  
  bool hwEvicted = false;
  
  // Test whether reduced set is sound
  int i,j;
  for (i=1; i<len; i++) {
    int fault = 0;

    FLSH(&shared_mem[target_index]);
    for (j=0; j<len; j++)
      FLSH(eset_sw[j]);

    // First Test
    HW_WR(&shared_mem[target_index]);
    HW_WR((uint64_t*)eset_sw[0]);
    HW_WR((uint64_t*)eset_sw[0]);
    HW_WR((uint64_t*)eset_sw[i]);
    HW_RD(&shared_mem[target_index]);
    hwEvicted = ((int)fpga_read(CSR_TIMING) > threshold);    

    if (!hwEvicted) {
      fault++;
      if(printFLAG) printf("     Address %2d - 0x%016lx fails at 1. congruence test\n", i, eset_sw[i]);
    }  

#if WITH_SECOND_TEST
    FLSH(&shared_mem[target_index]);
    for (j=0; j<len; j++)
      FLSH(eset_sw[j]);
    
    // Second Test
    if(hwEvicted) {
      HW_WR((uint64_t*)eset_sw[0]);
      HW_WR(&shared_mem[target_index]);
      HW_WR(&shared_mem[target_index]);
      HW_WR((uint64_t*)eset_sw[i]);
      HW_RD((uint64_t*)eset_sw[0]);
      hwEvicted = ((int)fpga_read(CSR_TIMING) > threshold);    
    } 

    if (!hwEvicted) {
      fault++;
      if(printFLAG) printf("     Address %2d - 0x%016lx fails at 2. congruence test\n", i, eset_sw[i]);
    }  
#endif

    FLSH(&shared_mem[target_index]);
    for (j=0; j<len; j++)
      FLSH(eset_sw[j]);

    // Third Test
    if(hwEvicted) {
      HW_WR((uint64_t*)eset_sw[i]);
      HW_WR(&shared_mem[target_index]);
      HW_WR(&shared_mem[target_index]);
      HW_WR((uint64_t*)eset_sw[0]);
      HW_RD((uint64_t*)eset_sw[i]);
      hwEvicted = ((int)fpga_read(CSR_TIMING) > threshold);    
    }

    if (!hwEvicted) {
      fault++;
      if(printFLAG) printf("     Address %2d - 0x%016lx fails at 3. congruence test\n", i, eset_sw[i]);
    }  
    
    if (fault!=0)
      nb_fault++;
  }

  return nb_fault;
}

void evs_check_threshold(int target_index) {
  uint64_t hw_addr, sw_addr;

  sw_addr = (uint64_t)&shared_mem[target_index];
  hw_addr = addr_sw_to_hw(sw_addr);

  fpga_write(CSR_TARGET_ADDRESS, hw_addr);

  // HW Read fom LLC
  FLSH(&shared_mem[target_index]);
  HW_WR(&shared_mem[target_index]);
  HW_RD(&shared_mem[target_index]);
  int time_llc = (int)fpga_read(CSR_TIMING);

  // HW Read fom RAM
  FLSH(&shared_mem[target_index]);
  HW_RD(&shared_mem[target_index]);
  int time_ram = (int)fpga_read(CSR_TIMING);

  printf("Time LLC: %d\n", time_llc);
  printf("Time RAM: %d\n", time_ram);
  
  // FILE *fptr;
  // fptr = fopen("./log/exec.log", "a");
  // fprintf(fptr, "Time LLC %d / Time RAM %d \n", time_llc, time_ram);
  // fclose(fptr);

  FLSH(&shared_mem[target_index]);
}

int evs_find_threshold(int target_index) {

  #define THRESHOLD_TEST_COUNT 1000

  uint64_t hw_addr, sw_addr;

  sw_addr = (uint64_t)&shared_mem[target_index];
  hw_addr = addr_sw_to_hw(sw_addr);

  // fpga_write(CSR_TARGET_ADDRESS, hw_addr);

  int timing[2][THRESHOLD_TEST_COUNT], t;
  for (t=0; t<THRESHOLD_TEST_COUNT; t++) {
      
    // HW Read fom LLC
    FLSH(&shared_mem[target_index]);
    HW_WR(&shared_mem[target_index]);
    HW_RD(&shared_mem[target_index]);
    timing[0][t] = (int)fpga_read(CSR_TIMING);

    // HW Read fom RAM
    FLSH(&shared_mem[target_index]);
    HW_RD(&shared_mem[target_index]);
    timing[1][t] = (int)fpga_read(CSR_TIMING);
  }
  qsort(timing[0], THRESHOLD_TEST_COUNT, sizeof(int), compare);
  qsort(timing[1], THRESHOLD_TEST_COUNT, sizeof(int), compare);
  int timeLLC = timing[0][(int)0.50*THRESHOLD_TEST_COUNT];
  int timeRAM = timing[1][(int)0.50*THRESHOLD_TEST_COUNT];
  // int timeTHR = timeLLC;
  // int timeTHR = (2*timeLLC+3*timeRAM)/5;
  // int timeTHR = (2*timeLLC + timeRAM)/3;
  int timeTHR = (timeLLC + 3*timeRAM)/4;
  // int timeTHR = 320;

  printf("\n [ ] HW's Data Access Thresholds:\n");
  printf("     Time to Access LLC: %d\n", timeLLC);
  printf("     Time to Access RAM: %d\n", timeRAM);
  printf("     Time to Access THR: %d\n", timeTHR);

  return timeTHR;
}


void evs_recipe_prepare(int* recipe, int recipe_length, int recipe_index) {
  
  uint64_t recipehex = (uint64_t)-1;

  int i;
  for (i=0; i<recipe_length; i++) {
    uint64_t mask;
    mask = 0x1F;
    recipehex ^= mask << (i*5);
    mask = recipe[i] & 0x1F;
    recipehex |= mask << (i*5);
  }
 
  fpga_write(CSR_EVS_ADDR,   recipe_index);
  fpga_write(CSR_EVS_RECIPE, recipehex   );
}

void evs_recipe_write(int recipe_index) {
  
  uint64_t mask = ((recipe_index & 0x1F) << 7) | FPGA_RECIPE_WRITE;

  fpga_write(CSR_CONTROL, mask);
  while (fpga_read(CSR_CONTROL)==0);
}

int evs_recipe_read(int recipe_index) {
  
  uint64_t mask = ((recipe_index & 0x1F) << 7) | FPGA_RECIPE_READ;

  fpga_write(CSR_CONTROL, mask);
  while (fpga_read(CSR_CONTROL)==0);
  return fpga_read(CSR_EVS_RECIPE_CNT);
}
