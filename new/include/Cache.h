#ifndef __CACHE_H__
#define __CACHE_H__

#include "Device.h"

class Cache: public Device
{
public:
    int write(uptr_t addr, const void* src, int len) override;
    int read(uptr_t addr, void* dest, int len) override;
    const char* name() const override;
};

#endif
