#ifndef CACHE_H
#define CACHE_H

// Processor name    : Intel(R) Xeon(R) Platinum 8180
// Packages(sockets) : 2
// Cores             : 56
// Processors(CPUs)  : 112
// Cores per package : 28
// Threads per core  : 2

// https://en.wikichip.org/wiki/intel/xeon_platinum/8180
// https://en.wikichip.org/wiki/intel/xeon_platinum
// https://en.wikichip.org/wiki/intel/microarchitectures/skylake_(server)

// LLC: Non-inclusive victim cache
// Mesh Architecture


#define SOCKET_COUNT       2
#define CORE_COUNT        28

// Per Core           
//                        | sets | ways | cline-size (in bytes)
#define L1_SIZE           (   64 *    8 *         64  ) //  32     KiB
#define L2_SIZE           ( 1024 *   16 *         64  ) //   1     MiB
#define LLC_SIZE          ( 2048 *   11 *         64  ) //   1.375 MiB
#define DDIO_SIZE         ( 2048 *    2 *         64  ) // 256     KiB

// Per Socket
#define LLC_SIZE_SOCKET   ( LLC_SIZE  * CORE_COUNT    ) //  38.5   MiB	
#define DDIO_SIZE_SOCKET  ( DDIO_SIZE * CORE_COUNT    ) //   7     MiB

////////////////////////////////////////////////////////////////////////////////

int reload(void *adrs);
int dwrite(void *adrs);
void flush(void *adrs);
int memread(void *v);
void prefetch(void* p);
void longnop();

#endif


