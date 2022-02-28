#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

////////////////////////////////////////////////////////////////////////////////
// SIZES

#define KB 1024
#define MB (1024 * KB)
#define GB (1024UL * MB)

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif
#ifndef MAP_HUGE_SHIFT
#define MAP_HUGE_SHIFT 26
#endif

#define MAP_2M_HUGEPAGE (0x15 << MAP_HUGE_SHIFT) /* 2 ^ 0x15 = 2M */
#define MAP_1G_HUGEPAGE (0x1e << MAP_HUGE_SHIFT) /* 2 ^ 0x1e = 1G */

////////////////////////////////////////////////////////////////////////////////

#define SHARED_MEM_SIZE (256*MB)
#define EVICT_MEM_SIZE  (  8*MB)

////////////////////////////////////////////////////////////////////////////////
// DEFINITIONS FOR SHARED MEMORIES

#define SPROTECTION (PROT_READ | PROT_WRITE)
#define SFLAGS_4K (MAP_SHARED | MAP_ANONYMOUS)
#define SFLAGS_2M (MAP_2M_HUGEPAGE | SFLAGS_4K  | MAP_HUGETLB)
#define SFLAGS_1G (MAP_1G_HUGEPAGE | SFLAGS_4K  | MAP_HUGETLB)

int map_shared_mem  (volatile uint64_t** addr, int len);
int unmap_shared_mem(volatile uint64_t*  addr, int len);

int map_syncronization_mem  (volatile uint64_t** addr);
int unmap_syncronization_mem(volatile uint64_t*  addr);

////////////////////////////////////////////////////////////////////////////////
// DEFINITIONS FOR EVICTION MEMORIES

#define EPROTECTION (PROT_READ | PROT_WRITE)
#define EFLAGS_4K (MAP_SHARED | MAP_ANONYMOUS)
#define EFLAGS_2M (MAP_2M_HUGEPAGE | EFLAGS_4K  | MAP_HUGETLB)
#define EFLAGS_1G (MAP_1G_HUGEPAGE | EFLAGS_4K  | MAP_HUGETLB)

int map_evict_mem  (char** addr, int len);
int unmap_evict_mem(char*  addr, int len);

#endif
