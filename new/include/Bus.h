#ifndef __BUS_H__
#define __BUS_H__

#include <map>
#include <utility>
#include "utils.h"

class Device;

// bus is used to manage devices and raise write/read requests
// still not sure about the schedule policy
// currently we assume this bus will not cause blocking
class Bus
{
    std::map<int, Device*> devList;
public:
    /* raise an write request
    return the bus latency + device latency,
    or negative integer if failed
    */
    virtual int write(int device, uaddr_t addr, const void* src, int len);
    // raise a read request, return value same as write
    virtual int read(int device, uaddr_t addr, void* dest, int len);
    virtual int addDevice(Device* dev);
    virtual int removeDevice(int device);
    virtual const std::map<int, Device*>& getDeviceList() const
    {
        return devList;
    }

};

#endif
