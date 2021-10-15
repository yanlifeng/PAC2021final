#ifndef  RESULT_H
#define RESULT_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <map>
#include <unordered_map>
//#include "robin_hood.h"
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

    void setBarcodeProcessor(unordered_map<uint64, Position1> *bpmap);

//    void setBarcodeProcessor(int headNum, int *hashHead, node *hashMap, uint64 *bloomFilter);

    void
    setBarcodeProcessorHashTableOneArrayWithBloomFilter(int *bpmap_head, int *bpmap_nxt, bpmap_key_value *position_all,
                                                        BloomFilter *bloomFilter);


private:
    void setBarcodeProcessor();

public:
    Options *mOptions;

    double GetCostPe() const;

    bool mPaired;
    long mTotalRead;
    long mFxiedFilterRead;
    long mDupRead;
    long mLowQuaRead;
    long mWithoutPositionReads;
    long overlapReadsWithMis = 0;

    double GetCostWait() const;

    double GetCostNew() const;

    double GetCostAll() const;

    long overlapReadsWithN = 0;
    Writer *mWriter;
    BarcodeProcessor *mBarcodeProcessor;
    int mThreadId;

    double GetCostFormat() const;

    double costWait;
    double costFormat;
    double costNew;
    double costPE;
    double costAll;

};

#endif // ! RESULT_H
