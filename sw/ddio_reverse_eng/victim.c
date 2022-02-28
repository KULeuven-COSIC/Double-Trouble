#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <stdlib.h>

#include "mmap.h"
#include "cache_utils.h"

#define ASSERT(x) assert(x != -1)

extern volatile uint64_t *shared_mem;
extern volatile uint64_t *syncronization;
extern volatile uint64_t *syncronization_params;
extern char *evict_mem;


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
      timing = reload((void*)*syncronization_params);
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
    else if (*syncronization == 4) {
      // printf("Victim evicts\n");
      for (j=0; j<3; j++)
        for (i=0; i<L2_SIZE; i += 64)
          memread((void*)(&evict_mem[i]));
      FENCE
      *syncronization_params = 0;
      FENCE
      *syncronization = 0;
    }

    // nanosleep(&t, NULL);
  }

  printf("Victim Quits\n");
  
  exit(EXIT_SUCCESS);
}