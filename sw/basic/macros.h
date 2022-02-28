////////////////////////
// SW MEMORY OPERATIONS
////////////////////////

#define SW_READ()   ({                                            \
  timing[m][t] = reload((void*)TARGET);                           \
  action[m++] = 0;                                                })

#define SW_READ_A(x)  ({                                          \
  timing[m][t] = reload((void*)x);                                \
  action[m++] = 0;                                                })

#define SW_WRITE()  ({                                            \
  dwrite((void*)TARGET);                                          \
  action[m++] = 1;                                                })

#define SW_WRITE_A(x)  ({                                         \
  dwrite((void*)x);                                               \
  action[m++] = 1;                                                })

////////////////////////
// HW MEMORY OPERATIONS
////////////////////////

#define HW_READ_SH(x)  ({                                         \
  hw_read_addr = ((x - shared_mem)*8 + shared_mem_iova) / CL(1);  \
  fpga_write(CSR_TARGET_ADDRESS, hw_read_addr );                  \
  fpga_write(CSR_CONTROL, FPGA_READ);                             \
  while (fpga_read(CSR_CONTROL)==0);                              \
  timing[m][t] = fpga_read(CSR_TIMING);                           \
  action[m++] = 2;                                                })

#define HW_READ_A(x)  ({                                          \
  HW_READ_SH((uint64_t *)x);                                      })

#define HW_READ()  ({                                             \
  HW_READ_SH((uint64_t *)TARGET);                                 })

#define HW_WRITE_SH(x, y)  ({                                     \
  hw_read_addr = ((x - shared_mem)*8 + shared_mem_iova) / CL(1);  \
  fpga_write(CSR_TARGET_ADDRESS, hw_read_addr );                  \
  fpga_write(CSR_CONTROL, y);                                     \
  while (fpga_read(CSR_CONTROL)==0);                              \
  timing[m][t] = 0;                                               \
  action[m++] = 3;                                                })

#define HW_WRITE()  ({                                            \
  HW_WRITE_SH((uint64_t *)TARGET, FPGA_WRITE_BLOCK);              })

#define HW_WRITE_A(x)  ({                                         \
  HW_WRITE_SH((uint64_t*)x, FPGA_WRITE_BLOCK);                    })

#define HW_WRITE_HWA(x)  ({                                       \
  fpga_write(CSR_TARGET_ADDRESS, x);                              \
  fpga_write(CSR_CONTROL, FPGA_WRITE_BLOCK);                      \
  while (fpga_read(CSR_CONTROL)==0);                              \
  timing[m][t] = 0;                                               \
  action[m++] = 3;                                                })

////////////////////////////
// VICTIM MEMORY OPERATIONS
////////////////////////////

#define VICTIM_READ(x)   ({                                       \
  *syncronization_params = (volatile uint64_t)x;                  \
  *syncronization = 1;                                            \
  while(*syncronization==1);                                      \
  timing[m][t] = *syncronization_params;                          \
  action[m++] = 4;                                                })

#define VICTIM_WRITE(x)   ({                                      \
  *syncronization_params = (volatile uint64_t)x;                  \
  *syncronization = 2;                                            \
  while(*syncronization==2);                                      \
  timing[m][t] = *syncronization_params;                          \
  action[m++] = 5;                                                })

////////////////////////////
// HELPER MEMORY OPERATIONS
////////////////////////////

#define HELPER_READ(x)   ({                                       \
  action[m] = 30;                                                 \
  *syncronization_params = (volatile uint64_t)x;                  \
  *syncronization = 101;                                          \
  while(*syncronization==101);                                    \
  timeHelper = *syncronization_params;                            })

#define HELPER_WRITE(x)   ({                                      \
  action[m] = 31;                                                 \
  *syncronization_params = (volatile uint64_t)x;                  \
  *syncronization = 102;                                          \
  while(*syncronization==102);                                    \
  timing[m++][t] = *syncronization_params;                        })

///////////////////
// FLUSH AND EVICT
///////////////////

#define FLUSH()   ({                                              \
  SEPARATOR();                                                    \
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

///////////////////////////////
// CONFIGURE THRESHOLDS FOR HW
///////////////////////////////

#define CONFIGURE_THRESHOLDS_HW() ({                                                                                  \
  hw_read_addr = (((uint64_t*)TARGET - shared_mem)*8 + shared_mem_iova) / CL(1);                                      \
  fpga_write(CSR_TARGET_ADDRESS, hw_read_addr);                                                                       \
  for (t=0; t<TEST_COUNT; t++) {                                                                                      \
    flush((void*)TARGET);                                                                                             \
    fpga_write(CSR_CONTROL, FPGA_WRITE_BLOCK); while(fpga_read(CSR_CONTROL)==0);                                      \
    fpga_write(CSR_CONTROL, FPGA_READ);        while(fpga_read(CSR_CONTROL)==0); timing[0][t]=fpga_read(CSR_TIMING);  \
    flush((void*)TARGET);                                                                                             \
    dwrite((void*)TARGET);                                                                                            \
    fpga_write(CSR_CONTROL, FPGA_READ);        while(fpga_read(CSR_CONTROL)==0); timing[1][t]=fpga_read(CSR_TIMING);  \
    flush((void*)TARGET);                                                                                             \
    fpga_write(CSR_CONTROL, FPGA_READ);        while(fpga_read(CSR_CONTROL)==0); timing[2][t]=fpga_read(CSR_TIMING);  \
  }                                                                                                                   \
  qsort(timing[0], TEST_COUNT, sizeof(int), compare);                                                                 \
  qsort(timing[1], TEST_COUNT, sizeof(int), compare);                                                                 \
  qsort(timing[2], TEST_COUNT, sizeof(int), compare);                                                                 \
  HW_LLC = timing[0][(int) 0.05*TEST_COUNT];                                                                          \
  HW_L2  = timing[1][(int) 0.05*TEST_COUNT];                                                                          \
  HW_RAM = timing[2][(int) 0.05*TEST_COUNT]-2;                                                                        \
  HW_THR = timing[2][(int) 0.05*TEST_COUNT]-5;                                                                        })
  
///////////////////////////////
// CONFIGURE THRESHOLDS FOR SW
///////////////////////////////

#define CONFIGURE_THRESHOLDS_SW() ({                              \
  for (t=0; t<TEST_COUNT; t++) {                                  \
    m=0;                                                          \
    flush((void*)TARGET);                                         \
    HW_WRITE();                 /* time 0: */                     \
    SW_READ();                  /* time 1: LLC */                 \
    flush((void*)TARGET);                                         \
    SW_READ();                  /* time 2: DRAM */                \
  }                                                               \
  qsort(timing[1], TEST_COUNT, sizeof(int), compare);             \
  qsort(timing[2], TEST_COUNT, sizeof(int), compare);             \
  SW_LLC = timing[1][(int) 0.01*TEST_COUNT];                      \
  SW_RAM = timing[2][(int) 0.01*TEST_COUNT];                      \
  SW_THR = (SW_RAM + SW_LLC)/2;                                   })
