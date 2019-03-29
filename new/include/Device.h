#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "utils.h"

class Device
{
public:
    virtual int write(uptr_t addr, const void* src, int len) = 0;
    virtual int read(uptr_t addr, void* dest, int len) = 0;
    virtual const char* name() const = 0;
};

#endif
