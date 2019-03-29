#ifndef __RAM_H__
#define __RAM_H__

#include "Devicce.h"

class RAM: public Device
{
public:
    int write(uptr_t addr, const void* src, int len) override;
    int read(uptr_t addr, void* dest, int len) override;
    const char* name() const override;
};

#endif
