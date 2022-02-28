#define BUSY_WAIT() ({                                                      \
  for (z=20000; z>0; z--)                                                   \
    asm volatile("nop;");                                                   })
          
#define FLUSH(x)  ({                                                        \
  flush((void*)x);                                                          \
  action[m++] = 1;                                                          })
          
#define SW_READ(x)   ({                                                     \
  timing[m][t] = reload((void*)x);                                          \
  action[m++] = 2;                                                          })
          
#define SW_WRITE(x)  ({                                                     \
  timing[m][t] = dwrite((void*)x);                                          \
  action[m++] = 3;                                                          })

#define HW_READ(x)  ({                                                      \
  hw_read_addr = ((x - (uint64_t)shared_mem) + shared_mem_iova) / CL(1);    \
  fpga_write(CSR_TARGET_ADDRESS, hw_read_addr);                             \
  fpga_write(CSR_CONTROL, FPGA_READ);                                       \
  while (fpga_read(CSR_CONTROL)==0);                                        \
  timing[m][t] = fpga_read(CSR_TIMING);                                     \
  action[m++] = 4;                                                          })

#define HW_READ_A(x)  ({                                                    \
  fpga_write(CSR_TARGET_ADDRESS, x);                                        \
  fpga_write(CSR_CONTROL, FPGA_READ);                                       \
  while (fpga_read(CSR_CONTROL)==0);                                        \
  timing[m][t] = fpga_read(CSR_TIMING);                                     \
  action[m++] = 4;                                                          })

#define HW_WRITE(x)  ({                                                     \
  hw_read_addr = ((x - (uint64_t)shared_mem) + shared_mem_iova) / CL(1);    \
  fpga_write(CSR_TARGET_ADDRESS, hw_read_addr);                             \
  fpga_write(CSR_CONTROL, FPGA_WRITE_BLOCK);                                \
  while (fpga_read(CSR_CONTROL)==0);                                        \
  action[m++] = 5;                                                          })

#define HW_WRITE_A(x)  ({                                                   \
  fpga_write(CSR_TARGET_ADDRESS, x);                                        \
  fpga_write(CSR_CONTROL, FPGA_WRITE_BLOCK);                                \
  while (fpga_read(CSR_CONTROL)==0);                                        \
  action[m++] = 5;                                                          })

#define EVICT(x)  ({                                                        \
  for (z=0; z<x; z++) {                                                     \
    reload((void*)evset_sw[z]);                                             \
    reload((void*)evset_sw[z]);                                             \
    reload((void*)evset_sw[z]);                                             \
  }                                                                         \
  action[m++] = 6;                                                          })

#define EVICT_L2()  ({                                                      \
  for (z=0; z<L2_EVS_LEN; z++)                                              \
    reload((void*)evset_sw_l2[z]);                                          \
  for (z=0; z<L2_EVS_LEN; z++)                                              \
    reload((void*)evset_sw_l2[z]);                                          \
  for (z=0; z<L2_EVS_LEN; z++)                                              \
    reload((void*)evset_sw_l2[z]);                                          \
  action[m++] = 7;                                                          })

#define SEPARATOR()  ({                                                     \
  action[m++] = 99;                                                         })

#define VICTIM_READ(x)   ({                                                 \
  *syncronization_params = (volatile uint64_t)x;                            \
  *syncronization = 1;                                                      \
  while(*syncronization==1);                                                })
