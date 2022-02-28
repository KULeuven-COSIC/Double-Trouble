////////////////////////
// SW MEMORY OPERATIONS
////////////////////////
#define SW_READ()   ({                                            \
  timeSW = reload((void*)TARGET); })

#define SW_READ_A(x)  ({                                          \
  timeSW = reload((void*)x);})

#define SW_READ_A_BENCHMARK()   ({                                            \
  timing[m++][t] = reload((void*)TARGET); })

#define SW_READ_A_NO_MEASURE(x)  ({                                          \
  memread((void*)x);})

#define SW_WRITE()  ({                                            \
  dwrite((void*)TARGET);})

#define SW_WRITE_A(x)  ({                                            \
  dwrite((void*)x); })

////////////////////////
// HW MEMORY OPERATIONS
////////////////////////
#define HW_READ_SH(x)  ({                                         \
  hw_read_addr = ((x - shared_mem)*8 + shared_mem_iova) / CL(1);  \
  fpga_write(CSR_TARGET_ADDRESS, hw_read_addr );                  \
  fpga_write(CSR_CONTROL, FPGA_READ);                             \
  while (fpga_read(CSR_CONTROL)==0);                              \
  timing[m][t] = fpga_read(CSR_TIMING);                           \
  timeHW = timing[m][t];                           \
  action[m++] = 2;                                                })

#define HW_READ_A(x)  ({                                             \
  HW_READ_SH((uint64_t *)x);                                     })

#define HW_READ()  ({                                             \
  HW_READ_SH((uint64_t *)TARGET);                                     })

#define HW_WRITE_SH(x, y)  ({                                     \
  hw_read_addr = ((x - shared_mem)*8 + shared_mem_iova) / CL(1);  \
  fpga_write(CSR_TARGET_ADDRESS, hw_read_addr );                  \
  fpga_write(CSR_CONTROL, y);                                     \
  while (fpga_read(CSR_CONTROL)==0);                              })

#define HW_WRITE()  ({                                            \
  HW_WRITE_SH((uint64_t *)TARGET, FPGA_WRITE_BLOCK);                  })

#define HW_WRITE_A(x)  ({                                            \
  HW_WRITE_SH((uint64_t*)x, FPGA_WRITE_BLOCK);                  })

#define HW_WRITE_UPD()  ({                                        \
  HW_WRITE_SH((uint64_t *)TARGET, FPGA_WRITE_UPDATE);                 })

#define HW_WRITE_UPD_A(x)  ({                                        \
  HW_WRITE_SH(x, FPGA_WRITE_UPDATE);                 })

////////////////////////////
// VICTIM MEMORY OPERATIONS
////////////////////////////

#define VICTIM_READ(x)   ({                                            \
  *syncronization_params = (volatile uint64_t)x;                  \
  *syncronization = 1;                                            \
  while(*syncronization==1);                                      \
  timing[m][t] = *syncronization_params;                          \
  timeVictim = timing[m][t];\
  action[m++] = 4;                                                })

#define VICTIM_READ_SILENT()   ({                                 \
  *syncronization_params = (volatile uint64_t)TARGET;             \
  *syncronization = 1;                                            \
  while(*syncronization==1);                                      \
  action[m++] = 4;                                                })

#define VICTIM_WRITE(x)   ({                                      \
  *syncronization_params = (volatile uint64_t)x;                  \
  *syncronization = 2;                                            \
  while(*syncronization==2);                                      \
  timing[m][t] = *syncronization_params;                          \
  action[m++] = 5;                                                })

#define VICTIM_WRITE_SILENT()   ({                                \
  *syncronization_params = (volatile uint64_t)TARGET;             \
  *syncronization = 2;                                            \
  while(*syncronization==2);                                      \
  action[m++] = 5;                                                })

///////////////////
// FLUSH AND EVICT
///////////////////
#define FLUSH()   ({                                              \
  flush((void*)shared_mem);                                       \
  action[m++] = 10;                                               })

#define FLUSH_A(x)  ({                                            \
  flush((void*)x);                                                \
  action[m++] = 10;                                               })

#define VICTIM_FLUSH(x)   ({                                      \
  *syncronization_params = (volatile uint64_t)x;                  \
  *syncronization = 3;                                            \
  while(*syncronization==3);                                      \
  action[m] = 20;                                                 })

#define VICTIM_WAIT()  ({                                         \
  *syncronization = 5;                                            \
  while(*syncronization==5);                                      \
  action[m++] = 22;                                               })

//////////
// EXTRAS
//////////
#define BUSY_WAIT() ({                                            \
  for (i = 30000; i>0; i--)                                       \
    asm volatile("nop;");                                         \
  action[m++] = 30;                                               })

#define SEPARATOR()  ({                                           \
  action[m++] = 99;                                               })


///////////////////
// Detect platform
///////////////////
#define DETECT_PLATFORM() ({})
/*
#define DETECT_PLATFORM() ({\
  FILE *fp = fopen("/proc/cpuinfo", "r");\
  assert(fp != NULL);\
  size_t n = 0;\
  char *line = NULL;\
  while (getline(&line, &n, fp) > 0) {\
    if (strstr(line, "model name")) {\
      if (strstr(line, "Platinum 8180")) {\
        printf(" Platform: Xeon Platinum 8180 \n\t       + Arria 10 PAC \n"); break;\
      }\
      else {\
        printf(" Platform: Xeon Platinum 8280 \n\t     + Stratix 10 PAC \n"); break;\
      } \
    }\
  }\
  free(line); fclose(fp);\
  printf("============================== \n\n");\
})
*/


///////////////////////////////
// CONFIGURE THRESHOLDS FOR HW
///////////////////////////////
#define CONFIGURE_THRESHOLDS_HW() ({\
    for (t=0; t<TEST_COUNT; t++) {\
      m=0;\
      FLUSH_A(TARGET);/* time 0*/\
      HW_WRITE();\
      HW_READ(); /*time 1: LLC*/\
      FLUSH_A(TARGET); /* time 2*/\
      SW_WRITE();\
      HW_READ(); /* time 3: L2*/\
      FLUSH_A(TARGET); /* time 4*/\
      HW_READ(); /* time 5: DRAM*/\
    }\
    qsort(timing[1], TEST_COUNT, sizeof(int), compare); qsort(timing[3], TEST_COUNT, sizeof(int), compare); qsort(timing[5], TEST_COUNT, sizeof(int), compare);\
    HW_LLC = timing[1][(int) 0.01*TEST_COUNT]; \
    HW_L2 = timing[3][(int) 0.01*TEST_COUNT]; \
    HW_RAM = timing[5][(int) 0.01*TEST_COUNT]-2;\
    HW_THR = timing[5][(int) 0.01*TEST_COUNT]-5;\
})


///////////////////////////////
// CONFIGURE THRESHOLDS FOR SW
///////////////////////////////
#define CONFIGURE_THRESHOLDS_SW() ({\
    for (t=0; t<TEST_COUNT; t++) {\
      m=0;\
      FLUSH_A(TARGET);/* time 0*/\
      HW_WRITE();\
      SW_READ_A_BENCHMARK(); /*time 1: LLC*/\
      FLUSH_A(TARGET); /* time 2*/\
      SW_READ_A_BENCHMARK(); /* time 3: DRAM*/\
      FLUSH_A(TARGET); /* time 4*/\
      VICTIM_READ(TARGET); /* time 5*/\
      SW_READ_A_BENCHMARK(); /* time 6: Remote L2*/\
    }\
    qsort(timing[1], TEST_COUNT, sizeof(int), compare); \
    qsort(timing[3], TEST_COUNT, sizeof(int), compare); \
    qsort(timing[6], TEST_COUNT, sizeof(int), compare); \
    SW_LLC = timing[1][(int) 0.01*TEST_COUNT]; \
    SW_RAM = timing[3][(int) 0.01*TEST_COUNT]+10; \
    SW_RL2 = timing[6][(int) 0.001*TEST_COUNT]-10; \
    SW_THR = (SW_RAM + SW_LLC)/2;\
})



#define ER_HW(a,b) ({\
  HW_WRITE_A(base[a]);\
  HW_WRITE_A(base[b]);\
})

#define ER_SW(a,b) ({\
  SW_WRITE_A(base[a]);\
  SW_WRITE_A(base[b]);\
})

#define ER_HW_ROBUST(a,b) ({\
  HW_WRITE_A(base[a]);\
  HW_WRITE_A(base[a]);\
  HW_WRITE_A(base[a]);\
  HW_WRITE_A(base[b]);\
})

#define ER_SW_ROBUST(a,b) ({\
  SW_WRITE_A(base[a]);\
  SW_WRITE_A(base[a]);\
  SW_WRITE_A(base[a]);\
  SW_WRITE_A(base[b]);\
})


#define HELPER_READ()   ({                           \
  *syncronization_params = (volatile uint64_t)TARGET;        \
  *syncronization = 101;                                  \
  while(*syncronization==101);                           \
  timeHelper = *syncronization_params;              })

#define HELPER_WRITE()   ({                           \
  *syncronization_params = (volatile uint64_t)TARGET;        \
  *syncronization = 102;                                  \
  while(*syncronization==102);                           \
  timeHelper = *syncronization_params;              })
