//
// Created by 赵展 on 2021/9/27.
//

#include "BloomFilter.h"

BloomFilter::BloomFilter(){
    hashtable = new uint64[HashTableMax];
    memset(hashtable,0x00,sizeof(uint64)*HashTableMax);
}

bool BloomFilter::push(uint64 key){
    push_mod(key);
    return false;
}

bool BloomFilter::push_mod(uint64 key) {
    uint32 mapkey = key%Bloom_MOD;
    hashtable[mapkey>>64] = hashtable[mapkey>>64]|((uint64)1<<(mapkey&0xffffff));
}

bool BloomFilter::get(uint64 key){
    bool fg = true;
    fg &= get_mod(key);
    return fg;
}
bool BloomFilter::get_mod(uint64 key) {
    uint32 mapkey = key%Bloom_MOD;
    return  hashtable[mapkey>>64]&((uint64)1<<(mapkey&0xffffff));
}
