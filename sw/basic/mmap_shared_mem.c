#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "mmap.h"

int map_shared_mem(uint64_t** addr, int len) {
  
  // map shared memory to process address space
  
  if (len > 2 * MB)
    *addr = (uint64_t*)mmap(NULL, len, SPROTECTION, SFLAGS_1G, 0, 0);
  else if (len > 4 * KB)
    *addr = (uint64_t*)mmap(NULL, len, SPROTECTION, SFLAGS_2M, 0, 0);
  else
    *addr = (uint64_t*)mmap(NULL, len, SPROTECTION, SFLAGS_4K, 0, 0);
  
  if (*addr == MAP_FAILED) {
    if (errno == ENOMEM) {
      if (len > 2 * MB)
        printf("Could not allocate buffer (no free 1 GiB huge pages)\n");
      if (len > 4 * KB)
        printf("Could not allocate buffer (no free 2 MiB huge pages)\n");
      else
        printf("Could not allocate buffer (out of memory)\n");
    }
    printf("mmap failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int unmap_shared_mem(uint64_t *addr, int len) {
  
  if (len > 2 * MB)
    len = (len + (1 * GB - 1)) & (~(1 * GB - 1));
  else if (len > 4 * KB)
    len = 2 * MB;

  // un-map
  if (munmap((void*)addr, len) == -1) {
    printf("munmap failed");
    return -1;
  }
  return 0;
}