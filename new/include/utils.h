#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef unsigned int uint;
typedef unsigned long ulong;
typedef long long ll;
typedef unsigned long long ull;

typedef ll ptr_t;
typedef ull uptr_t;

#define doLog(prefix, fmt, ...)\
    fprintf(stderr, "%s: " fmt "\n", prefix, ##__VA_ARGS__)

#define logMessage(fmt, ...)\
    doLog("[ Message ]", fmt, ##__VA_ARGS__)

#define logError(fmt, ...)\
    doLog("[  Error  ]", fmt, ##__VA_ARGS__)

#define ones(s, e) (((~0ull) << (63-e)) >> (63-e+s))
#define subBits(x, s, e) ((x >> s) & ones(s,e))

#endif
