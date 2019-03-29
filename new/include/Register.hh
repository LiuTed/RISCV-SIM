#ifndef __REGISTER_H__
#define __REGISTER_H__

#include "utils.h"
#include <initializer_list>

union Register64
{
    int64_t i64;
    struct
    {
        int32_t i64l;
        int32_t i64h;
    };

    //const as set these variant will not affect the higher bits
    const int32_t i32;
    struct
    {
        int16_t i32l;
        int16_t i32h;
    };

    const int16_t i16;
    struct
    {
        int8_t i16l;
        int8_t i16h;
    };

    const uint8_t i8;

    float f32;
    double f64;

    int test(int idx) const
    {
        return !!(i64 & (1ll << idx));
    }
    int set(int idx)
    {
        i64 |= (1ll << idx);
        return 0;
    }
    int clear(int idx)
    {
        i64 &= ~(1ll << idx);
        return 0;
    }
    //highest significant bit first
    int64_t sub(const std::initializer_list<int>& bits) const
    {
        int64_t res = 0;
        for(auto& idx: bits)
        {
            res <<= 1;
            res |= test(idx);
        }
        return res;
    }
    //left-close-right-open interval
    int64_t sub(const std::initializer_list<std::initializer_list<int> >& bits) const
    {
        int64_t res = 0;
        for(const auto& pair: bits)
        {
            if(pair.size() == 1) //single bit
            {
                res <<= 1;
                res |= test(*pair.begin());
            }
            else if(pair.size() == 2) // bit range [s,e)
            {
                auto iter = pair.begin();
                int s = *iter;
                int e = *++iter;
                res <<= (e-s);
                res |= (subBits(i64, s, e) >> s);
            }
            else if(pair.size() == 3) // [s, e) with gap
            {
                auto iter = pair.begin();
                int s = *iter++;
                int e = *iter++;
                int gap = *iter;
                for(int i = s; i < e; i += gap)
                {
                    res <<= 1;
                    res |= test(i);
                }
            }
            else
            {
                logError("Register64::sub: illegal pairs");
            }
        }
        return res;
    }
    Register64(): i64(0) {}
};

#endif 
