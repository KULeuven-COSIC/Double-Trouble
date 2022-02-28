#ifndef STATISTICS_H
#define STATISTICS_H

#include <math.h>

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

// Comparison routine for sorting the tests
int compare(const void * a, const void * b) {
  return ( *(uint64_t*)a - *(uint64_t*)b );
}

void print_statistics_header(void) {
  printf(" # | Operation      | Median | Average | Variance | Std Dev \n"NC);
  printf("---|----------------|--------|---------|----------|---------\n"NC);
}

int print_action(uint8_t action){
  switch(action){
    
    case  0 : printf("BUSY_WAIT      | "); return 0;
    case  1 : printf("FLUSH          | "); return 0;
    case  2 : printf("SW_READ        | "); return 1;
    case  3 : printf("SW_WRITE       | "); return 1;
    case  4 : printf("HW_READ        | "); return 1;
    case  5 : printf("HW_WRITE       | "); return 0;
    case  6 : printf("EVICT          | "); return 0;
    case  7 : printf("EVICT L2       | "); return 0;
    case 99 : printf("---------------| "); return 0;
    default : printf("               | "); return 0;
  }
}

void print_statistics(uint8_t action, int* buffer, int buffer_len) {

  int i;
  float average, variance, std_deviation, sum = 0, sum1 = 0;

  //  Compute the sum of all elements
  for (i = 0; i < buffer_len; i++) {
    sum = sum + buffer[i];
  }
  average = sum / (float)buffer_len;

  // Compute variance  and standard deviation
  for (i = 0; i < buffer_len; i++) {
    sum1 = sum1 + pow((buffer[i] - average), 2);
  }
  variance = sum1 / (float)buffer_len;
  std_deviation = sqrt(variance);

  qsort(buffer, buffer_len, sizeof(int), compare);

  if( print_action(action) ) {

    printf("%6d | ", buffer[buffer_len/2]);
    printf("%7.1f | ", average);
    printf("%8.1f | ", variance);
    printf("%7.1f\n"NC, std_deviation);
  }
  else {
    printf("       |         |          |\n"NC);
  }
}

#endif