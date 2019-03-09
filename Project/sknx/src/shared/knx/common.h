#ifndef KNX_COMMON_H
#define KNX_COMMON_H

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <set>
#include <vector>
#include <queue>
#include <utility>
#include <ctime>

extern "C" {
#include <polarssl/ctr_drbg.h>
#include <polarssl/entropy.h>
#include <sban/common.h>
}

#define _DEBUG

#define ENABLE_BENCHMARKS
#define BENCHMARK_KEYEXCHANGE
//#define BENCHMARK_NODECOUNTER
//#define BENCHMARK_NETWORKING

#include "debug.h"
#include "timing.h"

#ifdef _MIOSIX
#include "miosix.h"
#include <interfaces/endianness.h>

#else

#define toBigEndian16(x)   ( (uint16_t)((x) << 8) | ((x) >> 8) )
#define fromBigEndian16(x) toBigEndian16(x)

#endif

#endif /* KNX_COMMON_H */
