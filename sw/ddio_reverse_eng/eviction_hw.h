#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

#include "fpga_utils.h"

// On flipflop
#define WAIT       200
#define THRESHOLD  320
#define NUMDDIOWAYS  2

#define LLC_WAYS   11
#define EV_DESIRED   31
#define EVFLAGS   (FPGA_CREATE_EVS_HUGE  | FPGA_CREATE_EVS_WITH_TEST)
//                 FPGA_CREATE_EVS_SMALL | FPGA_CREATE_EVS_WOUT_TEST

#define EVS_VERBOSE 1
#define EVS_NONVERB 0

#define ACCURACY_TEST       0
#define PERF_SWEEP_TEST     1
#define PERF_CHECK_TEST     2
#define POLICY_TEST         3
#define FAST_EVICTION_TEST  4


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
  
#define FLSH(x)   ({                                          \
  flush((void*)x);                                            })


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
  fpga_write(CSR_EVS_ADDR,       EV_DESIRED     );

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

  if (printFLAG) {
    printf("Check variables:\n");
    printf(" Debug Reg    0x%" PRIx64 "\n", debug);
    printf(" Wrap Count   %d\n", counter_wrap);
    printf(" EVS Len      %d\n", evs_count);
  }
    
  if (evs_count < 11)
    return 1;

  if (counter_wrap >= wrap_count)
    return 1;

  return 0;
}


int evs_get(uint64_t* eset_sw, uint64_t* eset_hw, int printFLAG) {
  
  uint64_t debug = fpga_read(CSR_DEBUG0);
  int evs_mem_size = (debug & 0xFF);
  int evs_mem_index = 0;
  int evs_len;

  int cnt_total_prev = 0;

  if (printFLAG) {
    printf(
" # | EA | HW ADDR     SW ADDR           |Err|Loop |Cycle: Accum. : Delta |\n"\
"---|----|-------------------------------|---|-----|-----:--------:-------|\n");
  }

  do {    
    fpga_write(CSR_EVS_ADDR, evs_mem_index);

    uint64_t tmp  = fpga_read(CSR_DEBUG1);
    int mem_addr  = (int)((tmp & 0xFF00000000000000) >> 56);  //  8-bit
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
      printf("%2d | %2d | 0x%08lx 0x%016lx | %1d | %3d | %3d : %6d : %5d |\n", 
        mem_addr,
        cnt_evs,      
        eset_hw[cnt_evs],
        eset_sw[cnt_evs],
        cnt_fails,
        cnt_loop,
        cnt_memop,
        cnt_total,
        cnt_total - cnt_total_prev);
    }

    cnt_total_prev = cnt_total;

    evs_mem_index++;
  } while(evs_mem_index < evs_mem_size);

  return evs_len;
}

#if NUMDDIOWAYS == 2

int evs_verify(int target_index, uint64_t* eset_sw, int len, int printFLAG) {
  uint64_t hw_addr;
  
  int nb_fault = 0;
  
  bool hwEvicted = false;
  
  // Test whether reduced set is sound
  int i;
  for (i=1; i < len; i++) {
    int fault = 0;

    // First Test
    HW_WR(&shared_mem[target_index]);
    HW_WR((uint64_t*)eset_sw[0]);
    HW_WR((uint64_t*)eset_sw[0]);
    HW_WR((uint64_t*)eset_sw[i]);
    HW_RD(&shared_mem[target_index]);
    hwEvicted = ((int)fpga_read(CSR_TIMING) > THRESHOLD);    

    if (!hwEvicted) {
      fault++;
      if(printFLAG) printf("Fault1 at %d - 0x%016lx\n", i, eset_sw[i]);
    }  

    // Second Test
    if(hwEvicted) {
      HW_WR((uint64_t*)eset_sw[0]);
      HW_WR(&shared_mem[target_index]);
      HW_WR(&shared_mem[target_index]);
      HW_WR((uint64_t*)eset_sw[i]);
      HW_RD((uint64_t*)eset_sw[0]);
      hwEvicted = ((int)fpga_read(CSR_TIMING) > THRESHOLD);    
    } 

    if (!hwEvicted) {
      fault++;
      if(printFLAG) printf("Fault2 at %d - 0x%016lx\n", i, eset_sw[i]);
    }  

    // Third Test
    if(hwEvicted) {
      HW_WR((uint64_t*)eset_sw[i]);
      HW_WR(&shared_mem[target_index]);
      HW_WR(&shared_mem[target_index]);
      HW_WR((uint64_t*)eset_sw[0]);
      HW_RD((uint64_t*)eset_sw[i]);
      hwEvicted = ((int)fpga_read(CSR_TIMING) > THRESHOLD);    
    }

    if (!hwEvicted) {
      fault++;
      if(printFLAG) printf("Fault3 at %d - 0x%016lx\n", i, eset_sw[i]);
    }  

    if (fault!=0)
      nb_fault++;
  }

  return nb_fault;
}

#else

int evs_verify(int target_index, uint64_t* eset_sw, int len, int printFLAG) {
  uint64_t hw_addr;
  
  int nb_fault = 0;
  
  bool hwEvicted = false;
  
  // Test whether reduced set is sound
  int i,f;
  for (i=NUMDDIOWAYS-1; i < len; i++) {
    int fault = 0;

    // First Test
    HW_WR(&shared_mem[target_index]);
    for (f=0; f<NUMDDIOWAYS-1; f++) {
      HW_WR((uint64_t*)eset_sw[f]);
      HW_WR((uint64_t*)eset_sw[f]);
    }
    HW_WR((uint64_t*)eset_sw[i]);
    HW_RD(&shared_mem[target_index]);
    hwEvicted = ((int)fpga_read(CSR_TIMING) > THRESHOLD);    

    if (!hwEvicted) {
      fault++;
      if(printFLAG) printf("Fault1 at %d - 0x%016lx\n", i, eset_sw[i]);
    }  

    if (fault!=0)
      nb_fault++;
  }

  return nb_fault;
}

#endif

void evs_statistics(int *cnt_fail, int *cnt_fail_first) { 

  int evs_mem_size = (fpga_read(CSR_DEBUG0) & 0xFF);
  int evs_mem_index = 0;

  *cnt_fail_first = 0;
  *cnt_fail = 0;

  do {
    fpga_write(CSR_EVS_ADDR, evs_mem_index);
    
    uint64_t tmp  = fpga_read(CSR_DEBUG1);
    int mem_addr  = (int)((tmp & 0xFF00000000000000) >> 56); //  8-bit
    int cnt_total = (int)((tmp & 0x00FFFFF000000000) >> 36); // 20-bit
    int cnt_loop  = (int)((tmp & 0x0000000FF0000000) >> 28); //  8-bit
    int cnt_fails = (int)((tmp & 0x000000000FF00000) >> 20); //  8-bit
    int cnt_memop = (int)((tmp & 0x00000000000FFF00) >>  8); // 12-bit    
    int cnt_evs   = (int)((tmp & 0x00000000000000FF)      ); //  8-bit

    if(cnt_fails!=0) {
      if (cnt_evs==1) {
        *cnt_fail_first++;
        *cnt_fail++;
      }
      else if (cnt_evs>1) {
        *cnt_fail++;
      }
    }
    
    evs_mem_index++;
  } while(evs_mem_index < evs_mem_size);
}

double evs_check_threshold(int target_index) {
  uint64_t hw_addr, sw_addr;

  sw_addr = (uint64_t)&shared_mem[target_index];
  hw_addr = addr_sw_to_hw(sw_addr);

  fpga_write(CSR_TARGET_ADDRESS, hw_addr);

  FLSH(&shared_mem[target_index]);
  HW_WR(&shared_mem[target_index]);
  HW_RD(&shared_mem[target_index]);
  printf("Timing: %d\n", (int)fpga_read(CSR_TIMING));

  FLSH(&shared_mem[target_index]);
  HW_RD(&shared_mem[target_index]);
  printf("Timing: %d\n", (int)fpga_read(CSR_TIMING));

  FLSH(&shared_mem[target_index]);
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
