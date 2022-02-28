#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "mmap.h"

int map_syncronization_mem(volatile uint64_t** addr) {
  
  int len = sizeof *addr;
  
  *addr = (volatile uint64_t*)mmap(NULL, len, SPROTECTION, SFLAGS_4K, 0, 0);
  
  if (*addr == MAP_FAILED) {
    printf("mmap failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int unmap_syncronization_mem(volatile uint64_t *addr) {
  
  int len = sizeof *addr;
  
  // un-map
  if (munmap((void*)addr, len) == -1) {
    printf("munmap failed");
    return -1;
  }
  return 0;
}