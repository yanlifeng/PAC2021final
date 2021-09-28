//
// Created by ylf9811 on 2021/9/28.
//

#ifndef PAC2022_BLOOMFILTER_H
#define PAC2022_BLOOMFILTER_H


#include <iostream>
#include <bitset>

#include "common.h"

class BloomFilter {
public:
    std::bitset<100000010> bits;
public:
    BloomFilter();

    void Set(uint64 barCode);

    bool Get(uint64 barCode);

    int Hash1(uint64 barCode);

    int Hash2(uint64 barCode);

    int Hash3(uint64 barCode);

    int Hash4(uint64 barCode);
};


#endif //PAC2022_BLOOMFILTER_H
