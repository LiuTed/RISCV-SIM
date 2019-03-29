#ifndef __BUS_H__
#define __BUS_H__

#include <map>
#include <utility>
#include "utils.h"

class Device;

class Bus
{
    std::map<int, Device*> devList;
public:
    virtual int write(int device, uptr_t addr, const void* src, int len);
    virtual int read(int device, uptr_t addr, void* dest, int len);
    virtual int addDevice(Device* dev);
    virtual int removeDevice(int device);
    virtual const std::map<int, Device*>& getDeviceList() const
    {
        return devList;
    }

};

#endif
