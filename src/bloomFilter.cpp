//
// Created by ylf9811 on 2021/9/28.
//

#include "bloomFilter.h"

BloomFilter::BloomFilter() {
    bits.reset();
}

void BloomFilter::Set(uint64 barCode) {
    ll idx1 = Hash1(barCode);
    ll idx2 = Hash2(barCode);
    uint32 idx3 = Hash3(barCode);

//    int idx4 = Hash4(barCode);
    bits[idx1] = 1;
    bits[idx2] = 1;
    bits[idx3] = 1;
//    bits[idx4] = 1;
}

bool BloomFilter::Get(uint64 barCode) {
    ll idx1 = Hash1(barCode);
    ll idx2 = Hash2(barCode);
//    return bits[idx1] && bits[idx2];

    uint32 idx3 = Hash3(barCode);

    return bits[idx1] && bits[idx2] && bits[idx3];
//    return bits[idx3];
//    int idx4 = Hash4(barCode);
//    int res = bits[idx1] && bits[idx2] && bits[idx3] && bits[idx4];
//    return res;
}

uint64 hash(uint64 key) {
    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return key;
}

ll BloomFilter::Hash1(uint64 barCode) {
    return hash(barCode) % 4099995643ll;
}

ll BloomFilter::Hash2(uint64 barCode) {
    return barCode % 4000000063ll;
}

uint32 BloomFilter::Hash3(uint64 barCode) {
    barCode = (~barCode) + (barCode << 18); // key = (key << 18) - key - 1;
    barCode = barCode ^ (barCode >> 31);
    barCode = barCode * 21; // key = (key + (key << 2)) + (key << 4);
    barCode = barCode ^ (barCode >> 11);
    barCode = barCode + (barCode << 6);
    barCode = barCode ^ (barCode >> 22);
    return (uint32) barCode;
}

int BloomFilter::Hash4(uint64 barCode) {
    return barCode % 1050001397;
}