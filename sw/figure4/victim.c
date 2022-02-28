#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <stdlib.h>

#include "mmap.h"
#include "cache_info_pac.h"

#define ASSERT(x) assert(x != -1)

uint64_t *shared_mem;
uint64_t *evict_mem;
extern volatile uint64_t *syncronization;
extern volatile uint64_t *syncronization_params;

void victim() {

  //////////////////////////////////////////////////////////////////////////////
  // Prepare variables for test cache access times

  int timing;
  int i,j;

  #define FENCE asm volatile ("mfence\n\t lfence\n\t");

  //////////////////////////////////////////////////////////////////////////////

  while(1) {

    if (*syncronization == -1) {
      break;
    }
    else if (*syncronization == 1) {
      // printf("Victim reads 0x%0lx\n ", *syncronization_params);

      // Perf
      timing = reload((void*)*syncronization_params);
      // End
      FENCE
      *syncronization_params = timing;
      FENCE
      *syncronization = 0;
    }
    else if (*syncronization == 2) {
      // printf("Victim writes 0x%0lx\n ", *syncronization_params);
      timing = dwrite((void*)*syncronization_params);
      FENCE
      *syncronization_params = timing;
      FENCE
      *syncronization = 0;
    }
    else if (*syncronization == 3) {
      // printf("Victim flushes\n");
      flush((void*)*syncronization_params);
      FENCE
      *syncronization_params = 0;
      FENCE
      *syncronization = 0;
    }
    else if (*syncronization == 5) {
      // printf("Victim waits\n");
      for (i = 1000; i>0; i--)
        asm volatile("nop;");
      FENCE
      *syncronization_params = 0;
      FENCE
      *syncronization = 0;
    }
  }

  exit(EXIT_SUCCESS);
}