/* 
 * Derived from code of Jo Van Bulck
 * https://github.com/jovanbulck/sgx-tutorial-space18
 *
 * In turn adapted from: Yarom, Yuval, and Katrina Falkner. 
 * "Flush+ reload: a high resolution, low noise, L3 cache side-channel attack." 
 * 23rd USENIX Security Symposium (USENIX Security 14). 2014.
 */

#include "cache_utils.h"

int reload(void *adrs) {
  volatile unsigned long time;

  asm volatile (
    "mfence\n"
    "rdtscp\n"
    "lfence\n"
    "mov %%eax, %%esi\n"
    "mov (%1), %%eax\n"
    "rdtscp\n"
    "sub %%esi, %%eax\n"
    : "=&a" (time)
    : "r" (adrs)
    : "ecx", "edx", "esi");

  return (int) time;
}

int dwrite(void *adrs) {
  volatile unsigned long time;

  asm volatile (
    "mfence\n\t"
    "lfence\n\t"
    "rdtsc\n\t"
    "lfence\n\t"
    "movl %%eax, %%esi\n\t"
    "movl $10, (%1)\n\t"
    "lfence\n\t"
    "rdtsc\n\t"
    "subl %%esi, %%eax \n\t"
    : "=a" (time)
    : "c" (adrs)
    : "%rsi", "%rdx");

  return (int) time;
}

void flush(void *adrs) {
  asm volatile (  
    "mfence\n"
    "lfence\n\t"
    "clflush 0(%0)\n"
    "clflush 0(%0)\n"
    "mfence\n"
    :
    : "D" (adrs)
    : "rax");
}

inline int memread(void *v) {
  int rv;
  asm volatile("mov (%1), %0": "+r" (rv): "r" (v):);
  return rv;
}

void prefetch(void* p) {
  asm volatile ("prefetcht1 %0" : : "m" (p));
}

void longnop() {
  asm volatile (
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
}