#ifndef POLICY_H
#define POLICY_H

#include "fpga_utils.h"

#define EV_SIZE 11

#define TEST_LEN 1000
#define TEST_MODE 0 // 0 for DDIO, 1 for DDIO+

struct policyTestInfo{
  uint8_t accessPattern[7];
  uint8_t accessPatternLen;
  uint8_t expectedCacheContent_LRU[2];
  uint8_t expectedCacheContent_RRIP[2];
  uint8_t expectedCacheContent_QUAD1[2];
  uint8_t expectedCacheContent_QUAD2[2];
  uint8_t expectedCacheContent_QUAD3[2];
};

#define HWWRITE(x)  ({                       \
  fpga_write(CSR_TARGET_ADDRESS, x);         \
  fpga_write(CSR_CONTROL, FPGA_WRITE_BLOCK); \
  while (fpga_read(CSR_CONTROL)==0);         })

#define HWREAD(x)  ({                        \
  fpga_write(CSR_TARGET_ADDRESS, x);         \
  fpga_write(CSR_CONTROL, FPGA_READ);        \
  while (fpga_read(CSR_CONTROL)==0);         \
  fpga_read(CSR_TIMING);                     })

#define SWWRITE(x)  ({                       \
  dwrite((void*)x);                          })

#define SWFLSH(x)  ({                        \
  flush((void*)x);                           })

// Print Colors
#define BLACK   "\033[0;30m"     
#define DGRAY   "\033[1;30m"
#define LGRAY   "\033[0;37m"     
#define WHITE   "\033[1;37m"
#define RED     "\033[0;31m"     
#define LRED    "\033[1;31m"
#define GREEN   "\033[0;32m"     
#define LGREEN  "\033[1;32m"
#define ORANGE 	"\033[0;33m"     
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[0;34m"     
#define LBLUE   "\033[1;34m"
#define PURPLE  "\033[0;35m"     
#define LPURPLE "\033[1;35m"
#define CYAN    "\033[0;36m"     
#define LCYAN   "\033[1;36m"
#define NC      "\033[0m"

int mostFreq(uint8_t vet[], int dim, int* freq) {
  int i, j, count;
  int most = 0;
  uint8_t temp, elem;

  for(i = 0; i < dim; i++) {
    temp = vet[i];
    count = 1;
    for(j = i + 1; j < dim; j++) {
      if(vet[j] == temp) {
        count++;
      }
    }
    if (most < count) {
      most = count;
      elem = vet[i];
    }
  }

  *freq = most;

  return elem;
}

int policy_test_ddio(uint64_t* evs_sw,
                    uint64_t* evs_hw,
                    uint8_t   accessPatternLen,
                    uint8_t*  accessPattern,
                    int       threshold,
                    int*      freq) {
  int i, t, res;
  uint8_t test[TEST_LEN];

  for (t=0; t<TEST_LEN; t++) {

    // Clean up first
    for (i=0; i < EV_SIZE; i++) { 
      HWWRITE(evs_hw[i]);
      HWWRITE(evs_hw[i]);
      HWWRITE(evs_hw[i]);
    }
    for (i=0; i < EV_SIZE; i++)
      SWFLSH(evs_sw[i]);

    // Perform the given access pattern
    for(i=0; i<accessPatternLen; i++) {
      if      (accessPattern[i] == 'A') HWWRITE(evs_hw[0]);
      else if (accessPattern[i] == 'B') HWWRITE(evs_hw[1]);
      else if (accessPattern[i] == 'C') HWWRITE(evs_hw[2]);    
    }

    // Check the check contents now
    int checkA, checkB, checkC;
    checkA = HWREAD(evs_hw[0]);
    checkB = HWREAD(evs_hw[1]);
    checkC = HWREAD(evs_hw[2]);
    
    // printf("checkA %d\n", checkA);
    // printf("checkB %d\n", checkB);
    // printf("checkC %d\n", checkC);

    res = 0;
    if (checkA < threshold) res |= 0b001;
    if (checkB < threshold) res |= 0b010;
    if (checkC < threshold) res |= 0b100;
    test[t] = res;
  }

  res = mostFreq(test, TEST_LEN, freq);
  return res;
}

int policy_test_ddiop(uint64_t* evs_sw,
                      uint64_t* evs_hw,
                      uint8_t   accessPatternLen,
                      uint8_t*  accessPattern,
                      int       threshold,
                      int*      freq) {
  int i, t, res;
  uint8_t test[TEST_LEN];

  for (t=0; t<TEST_LEN; t++) {

    // Clean up first
    for (i=0; i < EV_SIZE; i++) {
      SWWRITE(evs_sw[i]);
      HWWRITE(evs_hw[i]);
      HWWRITE(evs_hw[i]);
      HWWRITE(evs_hw[i]);
    }
    for (i=0; i < EV_SIZE; i++)
      SWFLSH(evs_sw[i]);

    int isAin=0, isBin=0, isCin=0;

    // Perform the given access pattern
    for(i=0; i<accessPatternLen; i++) {
      if      (accessPattern[i] == 'A' && isAin==0) {SWWRITE(evs_sw[0]);HWWRITE(evs_hw[0]);}
      else if (accessPattern[i] == 'A' && isAin==1) {HWWRITE(evs_hw[0]);}
      else if (accessPattern[i] == 'B' && isBin==0) {SWWRITE(evs_sw[1]);HWWRITE(evs_hw[1]);}
      else if (accessPattern[i] == 'B' && isBin==1) {HWWRITE(evs_hw[1]);}
      else if (accessPattern[i] == 'C' && isCin==0) {SWWRITE(evs_sw[2]);HWWRITE(evs_hw[2]);}
      else if (accessPattern[i] == 'C' && isCin==1) {HWWRITE(evs_hw[2]);}

      // Clean up first
      int j;
      for (j=3; j<EV_SIZE; j++) {
        HWWRITE(evs_hw[j]);
        HWWRITE(evs_hw[j]);
        HWWRITE(evs_hw[j]);
      }

      // Check the contents now
      int checkA, checkB, checkC;
      checkA = HWREAD(evs_hw[0]);
      checkB = HWREAD(evs_hw[1]);
      checkC = HWREAD(evs_hw[2]);

      if (checkA < threshold) isAin = 1; else isAin = 0;
      if (checkB < threshold) isBin = 1; else isBin = 0;
      if (checkC < threshold) isCin = 1; else isCin = 0;
    }

    // Check the contents now
    int checkA, checkB, checkC;
    checkA = HWREAD(evs_hw[0]);
    checkB = HWREAD(evs_hw[1]);
    checkC = HWREAD(evs_hw[2]);
    
    // printf("checkA %d\n", checkA);
    // printf("checkB %d\n", checkB);
    // printf("checkC %d\n", checkC);

    res = 0;
    if (checkA < threshold) res |= 0b001;
    if (checkB < threshold) res |= 0b010;
    if (checkC < threshold) res |= 0b100;
    test[t] = res;
  }

  res = mostFreq(test, TEST_LEN, freq);
  return res;
}

void policy_check_printheader() {
  printf("Access | CC | Reliability  | LRU | RRIP | QUAD1 | QUAD2 | QUAD3 |\n");
}

void policy_test( uint64_t* evs_sw,
                  uint64_t* evs_hw,
                  struct policyTestInfo *pti,
                  int                    index,
                  int                    threshold) {
  
  #define WRITE_LRU_ERROR  1
  #define WRITE_RRIP_ERROR 1
  #define WRITE_QUAD1_ERROR 1
  #define WRITE_QUAD2_ERROR 1
  #define WRITE_QUAD3_ERROR 1
  
  int cachecontent;  
  int test_LRU, test_RRIP, test_QUAD;

  int i, freq;
  char str[64] = "";

  // Do the test

#if TEST_MODE == 0
  cachecontent = policy_test_ddio(
    evs_sw,
    evs_hw,
    pti[index].accessPatternLen, 
    pti[index].accessPattern,
    threshold,
    &freq); 
#else
  cachecontent = policy_test_ddiop(
    evs_sw,
    evs_hw,
    pti[index].accessPatternLen, 
    pti[index].accessPattern,
    threshold,
    &freq); 
#endif 

  // Print the access pattern
  
  for(i=0; i<7; i++) {
    if      (pti[index].accessPattern[i] == 'A') { printf("A"); }
    else if (pti[index].accessPattern[i] == 'B') { printf("B"); }
    else if (pti[index].accessPattern[i] == 'C') { printf("C"); }
    else                                         { printf(" "); }
  }
  printf("| ");

  // Print the cache contents
  if(cachecontent & 0b001) { printf("A"); }
  if(cachecontent & 0b010) { printf("B"); }
  if(cachecontent & 0b100) { printf("C"); }
  printf(" | ");

  // Print reliability
  printf("%.5f", (double)(freq)/(double)TEST_LEN);
  printf("      | ");

  // Print comparison with LRU expected cache contents
  for(i=0; i<2; i++) {

    if(pti[index].expectedCacheContent_LRU[i] == 'A')
      if (cachecontent & 0b001) { printf(GREEN"%c"NC, 'A'); }
      else                      { printf(RED"%c"NC,   'A'); }
    
    if(pti[index].expectedCacheContent_LRU[i] == 'B')
      if (cachecontent & 0b010) { printf(GREEN"%c"NC, 'B'); }
      else                      { printf(RED"%c"NC,   'B'); }

    if(pti[index].expectedCacheContent_LRU[i] == 'C')
      if (cachecontent & 0b100) { printf(GREEN"%c"NC, 'C'); }
      else                      { printf(RED"%c"NC,   'C'); }
  }
  printf("  | ");

  // Print comparison with RRIP expected cache contents
  for(i=0; i<2; i++) {

    if(pti[index].expectedCacheContent_RRIP[i] == 'A')
      if (cachecontent & 0b001) { printf(GREEN"%c"NC, 'A'); }
      else                      { printf(RED"%c"NC,   'A'); }
    
    if(pti[index].expectedCacheContent_RRIP[i] == 'B')
      if (cachecontent & 0b010) { printf(GREEN"%c"NC, 'B'); }
      else                      { printf(RED"%c"NC,   'B'); }

    if(pti[index].expectedCacheContent_RRIP[i] == 'C')
      if (cachecontent & 0b100) { printf(GREEN"%c"NC, 'C'); }
      else                      { printf(RED"%c"NC,   'C'); }
  }
  printf("   | ");

  // Print comparison with QUAD1 expected cache contents
  for(i=0; i<2; i++) {

    if(pti[index].expectedCacheContent_QUAD1[i] == 'A')
      if (cachecontent & 0b001) { printf(GREEN"%c"NC, 'A'); }
      else                      { printf(RED"%c"NC,   'A'); }
    
    if(pti[index].expectedCacheContent_QUAD1[i] == 'B')
      if (cachecontent & 0b010) { printf(GREEN"%c"NC, 'B'); }
      else                      { printf(RED"%c"NC,   'B'); }

    if(pti[index].expectedCacheContent_QUAD1[i] == 'C')
      if (cachecontent & 0b100) { printf(GREEN"%c"NC, 'C');  }
      else                      { printf(RED"%c"NC,   'C');  }
  }
  printf("    | ");

  // Print comparison with QUAD2 expected cache contents
  for(i=0; i<2; i++) {

    if(pti[index].expectedCacheContent_QUAD2[i] == 'A')
      if (cachecontent & 0b001) { printf(GREEN"%c"NC, 'A'); }
      else                      { printf(RED"%c"NC,   'A'); }
    
    if(pti[index].expectedCacheContent_QUAD2[i] == 'B')
      if (cachecontent & 0b010) { printf(GREEN"%c"NC, 'B'); }
      else                      { printf(RED"%c"NC,   'B'); }

    if(pti[index].expectedCacheContent_QUAD2[i] == 'C')
      if (cachecontent & 0b100) { printf(GREEN"%c"NC, 'C'); }
      else                      { printf(RED"%c"NC,   'C'); }
  }
  printf("    | ");

  // Print comparison with QUAD2 expected cache contents
  for(i=0; i<2; i++) {

    if(pti[index].expectedCacheContent_QUAD3[i] == 'A')
      if (cachecontent & 0b001) { printf(GREEN"%c"NC, 'A'); }
      else                      { printf(RED"%c"NC,   'A'); }
    
    if(pti[index].expectedCacheContent_QUAD3[i] == 'B')
      if (cachecontent & 0b010) { printf(GREEN"%c"NC, 'B'); }
      else                      { printf(RED"%c"NC,   'B'); }

    if(pti[index].expectedCacheContent_QUAD3[i] == 'C')
      if (cachecontent & 0b100) { printf(GREEN"%c"NC, 'C'); }
      else                      { printf(RED"%c"NC,   'C'); }
  }

  printf("    |");
  printf("\n");
}

#endif
