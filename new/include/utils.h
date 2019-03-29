#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdexcept>

typedef unsigned int uint;
typedef unsigned long ulong;
typedef long long ll;
typedef unsigned long long ull;

typedef ll saddr_t;
typedef ull addr_t;
typedef ull uaddr_t;
#define addr_diff(a, b) ((ll)((a)-(b)))
#define addr_diff(x) ((ll)(x))

#define doLog(prefix, fmt, ...)\
    fprintf(stderr, "%s: " fmt "\n", prefix, ##__VA_ARGS__)

#define logMessage(fmt, ...)\
    doLog("[ Message ]", fmt, ##__VA_ARGS__)

#define logError(fmt, ...)\
    doLog("[  Error  ]", fmt, ##__VA_ARGS__)

#ifdef ENABLE_DEBUG_LOG
#define logDebug(fmt, ...)\
    doLog("[  Debug  ]", fmt, ##__VA_ARGS__)
#else
#define logDebug(...)
#endif

#define ones(s, e) (((~0ull) >> (64 - (e))) & ((~0ull) << (s)))
// [s, e) start from 0
#define subBits(x, s, e) ((x) & ones(s, e))

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#endif
