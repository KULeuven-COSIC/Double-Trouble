#ifndef CACHE_INFO_H
#define CACHE_INFO_H

// SOCKET_COUNT  2
// CORE_COUNT   28
// L1_SIZE      (   64 *    8 *         64  ) //  32     KiB
// L2_SIZE      ( 1024 *   16 *         64  ) //   1     MiB
// LLC_SIZE     ( 2048 *   11 *         64  ) //   1.375 MiB

#define BLOCK_OFFSET      6 // 64B cache lines

////////////////////////////////////////////////////////////////////////////////
// Per-core L1D
#define L1_WAYS           8
#define L1_INDEX_BITS     (BLOCK_OFFSET + 6) // 64 sets
#define L1_PERIOD         (1 << L1_INDEX_BITS)
#define L1_SET_INDEX(x)   ({uint64_t index = ( x & ~((1 << BLOCK_OFFSET)-1)) & ((1 << L1_INDEX_BITS)-1);index;})

////////////////////////////////////////////////////////////////////////////////
// Per-core L2
#define L2_WAYS           16
#define L2_INDEX_BITS     (BLOCK_OFFSET + 10) // 1024 sets
#define L2_PERIOD         (1 << L2_INDEX_BITS)
#define L2_SET_INDEX(x)   ({uint64_t index = ( x & ~((1 << BLOCK_OFFSET)-1)) & ((1 << L2_INDEX_BITS)-1);index;})

////////////////////////////////////////////////////////////////////////////////
// Shared non-inclusive LLC

#define LLC_SLICES        8
#define LLC_WAYS          11
#define LLC_INDEX_BITS    (BLOCK_OFFSET + 11) // 2048 sets per slice
#define LLC_PERIOD        (1 << LLC_INDEX_BITS)
#define LLC_SET_INDEX(x)  ({uint64_t index = ( x & ~((1 << BLOCK_OFFSET)-1)) & ((1 << LLC_INDEX_BITS)-1);index;})

#define CD_WAYS           12

#define SMALLPAGE_PERIOD    (1 << 12)

#endif