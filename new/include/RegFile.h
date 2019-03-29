#ifndef __REGFILE_H__
#define __REGFILE_H__

#include "Register.h"

class RegFile
{
    Register64 *regfile;
    int n;
public:
    RegFile(int n);
    ~RegFile();
    Register64& at(int idx);
    const Register64& at(int idx) const;
    Register64& operator[] (int idx);
    const Register64& operator[] (int idx) const;
};

#endif
