//
// Created by ylf9811 on 2021/9/28.
//

#include "bloomFilter.h"

BloomFilter::BloomFilter() {
    bits.reset();
}

void BloomFilter::Set(uint64 barCode) {
    int idx1 = Hash1(barCode);
//    int idx2 = Hash2(barCode);
//    int idx3 = Hash3(barCode);
//    int idx4 = Hash4(barCode);
    bits[idx1] = 1;
//    bits[idx2] = 1;
//    bits[idx3] = 1;
//    bits[idx4] = 1;
}

bool BloomFilter::Get(uint64 barCode) {
    int idx1 = Hash1(barCode);
    return bits[idx1];
//    int idx2 = Hash2(barCode);
//    int idx3 = Hash3(barCode);
//    int idx4 = Hash4(barCode);
//    printf("now query %d %d %d %d\n", idx1, idx2, idx3, idx4);
//    int res = bits[idx1] && bits[idx2] && bits[idx3] && bits[idx4];
////    printf("query ans is %d\n", res);
//    return res;
}

int BloomFilter::Hash1(uint64 barCode) {
    return barCode % 99997871;
}

int BloomFilter::Hash2(uint64 barCode) {
    return barCode % 85001171;
}

int BloomFilter::Hash3(uint64 barCode) {
    return barCode % 100000007;
}

int BloomFilter::Hash4(uint64 barCode) {
    return barCode % 70003763;
}