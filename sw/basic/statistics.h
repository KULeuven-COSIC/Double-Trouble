#ifndef STATISTICS_H
#define STATISTICS_H

#define _GNU_SOURCE
#include <math.h>
#include <stdlib.h>

// Print Colors
#define BLACK   "\033[0;30m"     
#define DGRAY   "\033[1;30m"
#define LGRAY   "\033[0;37m"     
#define WHITE   "\033[1;37m"
#define RED     "\033[0;31m"     
#define LRED    "\033[1;31m"
#define GREEN   "\033[0;32m"     
#define LGREEN  "\033[1;32m"
#define ORANGE   "\033[0;33m"     
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
  printf(ORANGE" # | Action     | Median | Average | Std Dev \n"NC);
}

void print_action(uint8_t action){
  switch(action){
    case 0:
      printf(BLUE"SW_READ    ");
      break;
    case 1:
      printf(BLUE"SW_WRITE   ");
      break;
    case 2:
      printf(GREEN"HW_READ    ");
      break;
    case 3:
      printf(GREEN"HW_WRITE   ");
      break;
    case 4:
      printf(CYAN"V_READ     ");
      break;
    case 5:
      printf(CYAN"V_WRITE    ");
      break;
    case 10:
      printf(RED"FLUSH      ");
      break;
    case 11:
      printf(RED"EVICT_L1_T ");
      break;
    case 12:
      printf(RED"EVICT_L2_T ");
      break;
    case 13:
      printf(RED"EVICT_LLC_T");
      break;
    case 14:
      printf(RED"EVICT_DIR  ");
      break;
    case 15:
      printf(RED"EVICT_FULL ");
      break;
    case 16:
      printf(RED"CLWB       ");
      break;
    case 17:
      printf(RED"PREFETCH   ");
      break;
    case 20:
      printf(CYAN"V_FLUSH     ");
      break;
    case 21:
      printf(RED"V_EVICT_FULL_L2");
      break;
    case 22:
      printf(CYAN"V_WAIT     ");
      break;
    case 24:
      printf(RED"V_EVICT_L1_TARGETED");
      break;
    case 25:
      printf(RED"V_EVICT_L2_TARGETED");
      break;
    case 26:
      printf(RED"V_EVICT_LLC_TARGETED");
      break;
    case 27:
      printf(RED"V_EV_DIR   ");
      break;
    case 30:
      printf(RED"BUSY_WAIT  ");
      break;
    case 31:
      printf(GREEN"HW_READ_LST");
      break;
    case 32:
      printf(GREEN"HW_WRT_LST ");
      break;
    case 99:
      printf(DGRAY"-----------");
      break;
  }
}

void write_action(uint8_t action, FILE* fp){
  switch(action){
    case 0:
      fprintf(fp, "SW_READ  | ");
      break;
    case 1:
      fprintf(fp, "SW_WRITE | ");
      break;
    case 2:
      fprintf(fp, "HW_READ  | ");
      break;
    case 3:
      fprintf(fp, "HW_WRITE | ");
      break;
    case 4:
      fprintf(fp, "V_READ   | ");
      break;
    case 5:
      fprintf(fp, "V_WRITE  | ");
      break;
    case 10:
      fprintf(fp, "FLUSH");
      break;
    case 11:
      fprintf(fp, "EVICT_L1_TARGETED");
      break;
    case 12:
      fprintf(fp, "EVICT_L2_TARGETED");
      break;
    case 13:
      fprintf(fp, "EVICT_LLC_TARGETED");
      break;
    case 14:
      fprintf(fp, "EVICT_DIRECTORY");
      break;
    case 20:
      fprintf(fp, "V_FLUSH");
      break;
    case 21:
      fprintf(fp, "V_EVICT_FULL_L2");
      break;
    case 31:
      fprintf(fp, "HW_READ_LST");
      break;
  }
}


void print_statistics(int* buffer, int buffer_len, uint8_t action, int index) {

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

  printf("%2d "DGRAY"| ", index);
  print_action(action);
 
  if (action < 10) { // Only these actions have timings
    printf(DGRAY"|"WHITE" %6d "DGRAY"| ", buffer[buffer_len/2]);
    printf("%7.1f | ", average);
    //printf("%8.1f | ", variance);
    printf("%7.1f\n"NC, std_deviation);
  } 
  else if (action == 99) {
    printf(DGRAY"|--------|---------|--------\n"NC);
  }  
  else {
    printf(DGRAY"|        |         |\n"NC);
  }
}

void write_statistics(int* buffer, int buffer_len, FILE *fp, uint8_t action) {

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

  write_action(action, fp);
  if (action < 10) { // Only these actions have timings
    fprintf(fp, "Med %4d  | ", buffer[buffer_len/2]);
    fprintf(fp,"Avg %9.2f  | ", average);
    //printf("Var %9.2f  | ", variance);
    fprintf(fp,"Std %9.2f\n", std_deviation);
  } else {
    fprintf(fp, "        |         |\n");
  }
}

#endif