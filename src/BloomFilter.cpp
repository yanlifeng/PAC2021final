//
// Created by 赵展 on 2021/9/27.
//

#include "BloomFilter.h"

BloomFilter::BloomFilter(){
    hashtable = new uint64[HashTableMax];
    memset(hashtable,0x00,sizeof(uint64)*HashTableMax);
}

bool BloomFilter::push(uint64 key){
//    push_mod(key);
//    push_xor(key);
    push_Classification(key);
    return false;
}

bool BloomFilter::get(uint64 key){
    bool fg = true;
//    fg = fg&&get_mod(key);
//    fg = fg&&get_xor(key);
    fg = fg&& get_Classification(key);
    return fg;
}

bool BloomFilter::push_mod(uint64 key) {
    uint32 mapkey = key%Bloom_MOD;
    hashtable[mapkey>>5] = hashtable[mapkey>>5]|((uint64)1<<(mapkey&0x1f));
}

bool BloomFilter::get_mod(uint64 key) {
    uint32 mapkey = key%Bloom_MOD;
    return  (hashtable[mapkey>>5]&((uint64)1<<(mapkey&0x1f)))!=0;
}

bool BloomFilter::push_xor(uint64 key) {
    uint32 mapkey = (key>>32)^(key&0xffffffff);
    hashtable[mapkey>>5] = hashtable[mapkey>>5]|((uint64)1<<(mapkey&0x1f));
}

bool BloomFilter::get_xor(uint64 key) {
    uint32 mapkey = (key>>32)^(key&0xffffffff);
    return (hashtable[mapkey>>5] = hashtable[mapkey>>5]|((uint64)1<<(mapkey&0x1f)))!=0;
}

bool BloomFilter::push_Classification(uint64 key){
    uint32 mapkey = key&0xffffff;
    hashtable[mapkey>>5] = hashtable[mapkey>>5]|((uint64)1<<(mapkey&0x1f));
}
bool BloomFilter::get_Classification(uint64 key){
    uint32 mapkey = key&0xffffff;
    return (hashtable[mapkey>>5] = hashtable[mapkey>>5]|((uint64)1<<(mapkey&0x1f)))!=0;
}