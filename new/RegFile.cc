#include "RegFile.h"
#include "Register.hh"

RegFile::RegFile(uint n): regfile(nullptr), n(n)
{
    regfile = new Register64[n];
}

RegFile::~RegFile()
{
    delete[] regfile;
}

Register64& RegFile::at(uint idx)
{
    if(unlikely(idx < 0 || idx >= n))
    {
        logError("RegFile::at: idx(%d) out of range [0,%d)", idx, n);
        throw std::out_of_range("RegFile::at");
    }
    return regfile[idx];
}
const Register64& RegFile::at(uint idx) const
{
    if(unlikely(idx < 0 || idx >= n))
    {
        logError("RegFile::at: idx(%d) out of range [0,%d)", idx, n);
        throw std::out_of_range("RegFile::at");
    }
    return regfile[idx];
}

Register64& RegFile::operator[] (uint idx)
{
    return regfile[idx];
}
const Register64& RegFile::operator[] (uint idx) const
{
    return regfile[idx];
}