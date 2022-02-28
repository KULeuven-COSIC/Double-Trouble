#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <inttypes.h>



// PERFORMANCE COUNTERS
#define L1_HIT 0x5301d1;
#define L1_MISS 0x5308d1;
#define L2_HIT 0x5302d1;
#define L2_MISS 0x5310d1;
#define L3_HIT 0x5304d1;
#define L3_MISS 0x5320d1;
//#define SNP_HIT 0x5301b7;

// Other experiment 
#define XNSP_MISS 0x5301d2;
#define XNSP_HIT 0x5302d2;
#define XNSP_HITM 0x5304d2;
#define XNSP_NONE 0x5308d2;
#define LOCAL_DRAM 0x5301d3;
#define REMOTE_DRAM 0x5302d3;
#define REMOTE_HITM 0x5304d3;
#define REMOTE_FWD 0x5308d3;


static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
               int cpu, int group_fd, unsigned long flags)
{
   int ret;

   ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                  group_fd, flags);
   return ret;
}

struct read_format {
  uint64_t nr;
  struct {
    uint64_t value;
    uint64_t id;
  } values[];
};
