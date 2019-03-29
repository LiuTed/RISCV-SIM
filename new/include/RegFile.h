#ifndef __REGFILE_H__
#define __REGFILE_H__

#include "Register.hh"

class RegFile
{
    Register64 *regfile;
    uint n;
public:
    RegFile(uint n);
    ~RegFile();
    Register64& at(uint idx);
    const Register64& at(uint idx) const;
    Register64& operator[] (uint idx);
    const Register64& operator[] (uint idx) const;
};

#define r0 0
#define ra 1
#define sp 2
#define gp 3
#define a0 10
#define a1 11
#define f0 32
#define fa1 43
#define pc 64
#define epc 65

#endif
