#ifndef STATISTICS_H
#define STATISTICS_H

#define _GNU_SOURCE

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

#endif