/* 
 * Derived from code of Jo Van Bulck
 * https://github.com/jovanbulck/sgx-tutorial-space18
 *
 * In turn adapted from: Yarom, Yuval, and Katrina Falkner. 
 * "Flush+ reload: a high resolution, low noise, L3 cache side-channel attack." 
 * 23rd USENIX Security Symposium (USENIX Security 14). 2014.
 */

#include <stdlib.h>
#include <stdint.h>

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
    : "=&a" (time) // output
    : "r" (adrs) // input
    : "ecx", "edx", "esi"); // clobber registers

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
    "mfence\n"
    :
    : "D" (adrs)
    : "rax");
}

inline int memread(void *v) {
  int rv = 0;
  asm volatile("mov (%1), %0": "+r" (rv): "r" (v):);
  return rv;
}


uint64_t rdtsc() {
  uint64_t a, d;
  asm volatile("mfence");
  asm volatile("rdtscp" : "=a"(a), "=d"(d) :: "rcx");
  a = (d << 32) | a;
  asm volatile("mfence");
  return a;
}


// ---------------------------------------------------------------------------
uint64_t rdtsc_begin() {
  uint64_t a, d;
  asm volatile ("mfence\n\t"
    "CPUID\n\t"
    "RDTSCP\n\t"
    "mov %%rdx, %0\n\t"
    "mov %%rax, %1\n\t"
    "mfence\n\t"
    : "=r" (d), "=r" (a)
    :
    : "%rax", "%rbx", "%rcx", "%rdx");
  a = (d<<32) | a;
  return a;
}

// ---------------------------------------------------------------------------
uint64_t rdtsc_end() {
  uint64_t a, d;
  asm volatile("mfence\n\t"
    "RDTSCP\n\t"
    "mov %%rdx, %0\n\t"
    "mov %%rax, %1\n\t"
    "CPUID\n\t"
    "mfence\n\t"
    : "=r" (d), "=r" (a)
    :
    : "%rax", "%rbx", "%rcx", "%rdx");
  a = (d<<32) | a;
  return a;
}
