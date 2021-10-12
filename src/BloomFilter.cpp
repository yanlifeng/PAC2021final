//
// Created by 赵展 on 2021/9/27.
//

#include "BloomFilter.h"

BloomFilter::BloomFilter(){
//    hashtable = new uint64[HashTableMax];
    hashtable = (uint64*)malloc(HashTableMax*sizeof(uint64));
    hashtableClassification = (uint64*)malloc(HashTableMax*sizeof(uint64));
    memset(hashtable,0,sizeof(uint64)*HashTableMax);
    memset(hashtableClassification,0,sizeof(uint64)*HashTableMax);
//    std::cout << "size is " << HashTableMax << std::endl;
}

bool BloomFilter::push(uint64 key){
    push_mod(key);
//    push_xor(key);
//    push_wang(key);
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
    uint32 a = key&0xffffffff;
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    a = (a>>6)&0x3fff;
    // 6 74
    uint32 mapkey = (key >> 32)|(a << 18);
    hashtable[mapkey>>6] |= (1ll<<(mapkey&0x3f));
    return false;
}

bool BloomFilter::get_mod(uint64 key) {
    uint32 a = key&0xffffffff;
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    a = (a>>6)&0x3fff;
    uint32 mapkey = (key >> 32)|(a << 18);
    return hashtable[mapkey>>6]&(1ll<<(mapkey&0x3f));
}

bool BloomFilter::push_xor(uint64 key) {
    uint32 mapkey = (key>>32)^(key&0xffffffff);
//    uint32 mapkey = (key>>18)&0xffffffff;// 神奇 ，效果很棒，过滤完100亿
    hashtable[(mapkey>>6)]|=(1ll<<(mapkey&0x3f));
//    std::cout << "map is ok!!!" << '\n';
    return false;
}

bool BloomFilter::get_xor(uint64 key) {
    uint32 mapkey = (key>>32)^(key&0xffffffff);
//    uint32 mapkey = (key>>18)&0xffffffff;
    return hashtable[mapkey>>6]&(1ll<<(mapkey&0x3f));
}

bool BloomFilter::push_Classification(uint64 key){
    uint64 mapkey = key&0xffffffff;
    hashtableClassification[mapkey>>6] |= (1ll<<(mapkey&0x3f));
    return false;
}
bool BloomFilter::get_Classification(uint64 key){
    uint64 mapkey = key&0xffffffff;
    return hashtableClassification[mapkey>>6]&(1ll<<(mapkey&0x3f));
}

bool BloomFilter::push_wang(uint64 key){
    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);
    uint32 mapkey = key&0xffffffff;
    hashtable[mapkey>>6] |= (1ll<<(mapkey&0x3f));
    return false;
}
bool BloomFilter::get_wang(uint64 key){
    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);
    uint32 mapkey = key&0xffffffff;
    return hashtable[mapkey>>6]&(1ll<<(mapkey&0x3f));
}