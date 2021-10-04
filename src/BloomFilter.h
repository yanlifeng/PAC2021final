//
// Created by 赵展 on 2021/9/27.
//

#ifndef ST_BARCODEMAP_MAIN_BLOOMFILTER_H
#define ST_BARCODEMAP_MAIN_BLOOMFILTER_H
#include "common.h"

class BloomFilter {
public:
    BloomFilter();
    bool push(uint64 key);
    bool get(uint64 key);

private:

    uint64* hashtable;
    uint64 size;


    uint64 HashTableMax = 5e6;

    bool push_mod(uint64 key);
    bool get_mod(uint64 key);

    const uint64 Bloom_MOD = 73939133;

};


#endif //ST_BARCODEMAP_MAIN_BLOOMFILTER_H
