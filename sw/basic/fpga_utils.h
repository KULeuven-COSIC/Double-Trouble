#ifndef FPGA_UTILS_H
#define FPGA_UTILS_H

#include <opae/fpga.h>
#include <uuid/uuid.h>
#include <stdint.h>

// State from the AFU's JSON file, extracted using OPAE's afu_json_mgr script
#include "fpga_info.h"

#define USER_CSR_BASE   32
#define CACHELINE_BYTES 64
#define CL(x) ((x) * CACHELINE_BYTES)

#define FPGA_CREATE_EVS_SMALL             0b1000000
#define FPGA_CREATE_EVS_HUGE              0b0000000
#define FPGA_CREATE_EVS_EN_TEST_ONLYFIRST 0b0100000
#define FPGA_CREATE_EVS_EN_TEST_MULTI     0b0010000
#define FPGA_CREATE_EVS_EN_TEST           0b0001000
#define FPGA_WRITE_NONBL                  0b0000001
#define FPGA_WRITE_BLOCK                  0b0000010
#define FPGA_READ                         0b0000100
#define FPGA_WRITE_UPDATE                 0b0000110
#define FPGA_RECIPE_WRITE                 0b0000101
#define FPGA_RECIPE_READ                  0b0000111

#define FPGA_CREATE_EVS_WITH_TEST         FPGA_CREATE_EVS_EN_TEST
#define FPGA_CREATE_EVS_WOUT_TEST         0

#define CSR_CONTROL            0
#define CSR_TARGET_ADDRESS     1
#define CSR_TIMING             1
#define CSR_SEARCH_ADDRESS     2
#define CSR_THRESH             3
#define CSR_STRIDE             4
#define CSR_EVS_RECIPE_CNT     4
#define CSR_EVS_ADDR           5
#define CSR_EVS_DATA           5
#define CSR_DEBUG0             6
#define CSR_EVS_RECIPE         6
#define CSR_WAIT               7
#define CSR_DEBUG1             7


fpga_handle accel_handle;
uint64_t wsid;

// Search for an accelerator matching the requested UUID and connect to it.
//
int fpga_connect_to_accel(const char *accel_uuid)
{
  fpga_properties filter = NULL;
  fpga_guid guid;
  fpga_token accel_token;
  uint32_t num_matches;
  fpga_result r;

  // Set up a filter that will search for an accelerator
  fpgaGetProperties(NULL, &filter);
  fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);

  // Add the desired UUID to the filter
  uuid_parse(accel_uuid, guid);
  fpgaPropertiesSetGUID(filter, guid);

  // Do the search across the available FPGA contexts
  num_matches = 1;
  fpgaEnumerate(&filter, 1, &accel_token, 1, &num_matches);

  // Not needed anymore
  fpgaDestroyProperties(&filter);

  if (num_matches < 1)
  {
    fprintf(stderr, "Accelerator %s not found!\n", accel_uuid);
    return -1;
  }

  // Open accelerator
  r = fpgaOpen(accel_token, &accel_handle, 0);
  assert(FPGA_OK == r);

  // Done with token
  fpgaDestroyToken(&accel_token);

  return 0;
}

// Allocate a buffer in I/O memory, shared with the FPGA.
//
static volatile void* fpga_alloc_buffer(ssize_t size,
                                        uint64_t *io_addr)
{
  fpga_result r;
  volatile void* buf;

  r = fpgaPrepareBuffer(accel_handle, size, (void**)&buf, &wsid, 0);
  if (FPGA_OK != r) return NULL;

  // Get the physical address of the buffer in the accelerator
  r = fpgaGetIOAddress(accel_handle, wsid, io_addr);
  assert(FPGA_OK == r);

  return buf;
}

// Allocate a buffer in I/O memory, shared with the FPGA.
//
static void fpga_share_buffer(volatile void* buf,
                              ssize_t   size,
                              uint64_t *io_addr)
{
  fpga_result r;

  r = fpgaPrepareBuffer(
      accel_handle, 
      size, 
      (void**)&buf, 
      &wsid, 
      FPGA_BUF_PREALLOCATED // | FPGA_BUF_READ_ONLY
    );

    if(r!=FPGA_OK) {
      printf("Error on fpgaPrepareBuffer() : ");
      if(r==FPGA_NO_MEMORY)     printf("FPGA_NO_MEMORY\n");
      if(r==FPGA_INVALID_PARAM) printf("FPGA_INVALID_PARAM\n");
      if(r==FPGA_EXCEPTION)     printf("FPGA_EXCEPTION\n");
    }
    assert(FPGA_OK == r);
  
  // Get the physical address of the buffer in the accelerator
  r = fpgaGetIOAddress(accel_handle, wsid, io_addr);
  assert(FPGA_OK == r);
}

static void fpga_close() {
  fpgaReleaseBuffer(accel_handle, wsid);
  fpgaClose(accel_handle);
}

static void fpga_write(uint32_t idx, uint64_t v) {
  fpgaWriteMMIO64(accel_handle, 0, 8*(USER_CSR_BASE+idx), v);
}

static uint64_t fpga_read(uint32_t idx) {
  uint64_t read;
  fpgaReadMMIO64(accel_handle, 0, 8*(USER_CSR_BASE+idx), &read);
  return read;
}

////////////////////////////////////////////////////////////////////////////////
// Address conversion

extern uint64_t shared_mem_iova;
extern volatile uint64_t *shared_mem;
uint64_t addr_hw_to_sw(uint64_t hw_address) {
  uint64_t sw_address = hw_address; 
  sw_address *= CL(1);
  sw_address -= shared_mem_iova;
  sw_address += (uint64_t)shared_mem;
  return sw_address;
}

uint64_t addr_sw_to_hw(uint64_t sw_address) {
  uint64_t hw_address = sw_address; 
  hw_address -= (uint64_t)shared_mem;
  hw_address += shared_mem_iova;
  hw_address /= CL(1);
  return hw_address;
}

#endif
