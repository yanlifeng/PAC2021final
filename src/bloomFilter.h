//
// Created by ylf9811 on 2021/9/28.
//

#ifndef PAC2022_BLOOMFILTER_H
#define PAC2022_BLOOMFILTER_H


#include "common.h"
#include <iostream>
class BloomFilter {
public:
    BloomFilter();
    bool push(uint64 key);
    bool get(uint64 key);
    bool push_mod(uint64 key);
    bool get_mod(uint64 key);

    bool push_xor(uint64 key);
    bool get_xor(uint64 key);

    bool push_Classification(uint64 key);
    bool get_Classification(uint64 key);

    bool push_wang(uint64 key);
    bool get_wang(uint64 key);

public:

    uint64* hashtable;
    uint64* hashtableClassification;
    uint64 size;


    const static uint32 HashTableMax = 1ll<<26;



    const uint64 Bloom_MOD = 73939133;

};

#endif //PAC2022_BLOOMFILTER_H
