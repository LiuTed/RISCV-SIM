#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "utils.h"

class Device
{
public:
    // return latency if success, or negative integer if failed
    virtual int write(uaddr_t addr, const void* src, int len) = 0;
    // return latency if success, or negative integer if failed
    virtual int read(uaddr_t addr, void* dest, int len) = 0;
    // name is used for debug and statistics
    virtual const char* name() const = 0;
};

#endif
