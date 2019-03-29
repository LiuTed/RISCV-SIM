#ifndef __RAM_H__
#define __RAM_H__

#include "Devicce.h"

class RAM: public Device
{
public:
    int write(uaddr_t addr, const void* src, int len) override;
    int read(uaddr_t addr, void* dest, int len) override;
    const char* name() const override;
};

#endif
