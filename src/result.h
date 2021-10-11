#ifndef  RESULT_H
#define RESULT_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <map>
#include <unordered_map>
#include <iomanip>
#include "common.h"
#include "options.h"
#include "writer.h"
#include "barcodeProcessor.h"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/map.hpp>

using namespace std;

class Result {
public:
    Result(Options *opt, int threadId, bool paired = true);

    ~Result();

    Writer *getWriter() { return mWriter; }

    static Result *merge(vector<Result *> &list);

    void print();

    void dumpDNBs(string &mappedDNBOutFile);

    void setBarcodeProcessor(robin_hood::unordered_map<uint64, Position1> *bpmap);

    void setBarcodeProcessor(robin_hood::unordered_map<uint64, Position1> *bpmap, robin_hood::unordered_map<uint32,  bpmap_segment_value> *bpmap_segment);

    void setBarcodeProcessorSegment(robin_hood::unordered_map<uint32,  bpmap_segment_value> *bpmap_segment);

    void setBarcodeProcessorHash(robin_hood::unordered_map<uint64,Position1> **bpmap_hash);

    void setBarcodeProcessorHashIndex(robin_hood::unordered_map<uint32,uint32> **bpmap_hash_index, Position1* position1_index);

    void setBarcodeProcessorHashTable(int* bpmap_head, int* bpmap_nxt,uint64* bpmap_key,int* bpmap_value,Position1* position_index);

    void setBarcodeProcessorHashTableOrder(int* bpmap_head, int* bpmap_len,uint64* bpmap_key,int* bpmap_value,Position1* position_index,bool ordered);

    void setBarcodeProcessorHashTableNoIndex(int* bpmap_head, int* bpmap_nxt,uint64* bpmap_key,Position1* position_index);

    void setBarcodeProcessorHashTableNoIndexWithBloomFilter(int* bpmap_head, int* bpmap_nxt,uint64* bpmap_key,Position1* position_index,BloomFilter* bloomFilter);

    void setBarcodeProcessorHashTableOneArrayWithBloomFilter(int* bpmap_head, int* bpmap_nxt,bpmap_key_value* position_all,BloomFilter* bloomFilter);

private:
    void setBarcodeProcessor();

public:
    Options *mOptions;
    bool mPaired;
    long mTotalRead;
    long mFxiedFilterRead;
    long mDupRead;
    long mLowQuaRead;
    long mWithoutPositionReads;
    long overlapReadsWithMis = 0;
    long overlapReadsWithN = 0;
    Writer *mWriter;
    BarcodeProcessor *mBarcodeProcessor;
    int mThreadId;
    double costFormat;

};

#endif // ! RESULT_H
