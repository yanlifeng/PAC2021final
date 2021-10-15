#ifndef  BARCODE_PROCESSOR_H
#define BARCODE_PROCESSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include "read.h"
#include "util.h"
#include "barcodePositionMap.h"
#include "options.h"
#include "util.h"
#include "bloomFilter.h"
//#include "robin_hood.h"

using namespace std;

class BarcodeProcessor {
public:
    BarcodeProcessor(Options *opt, unordered_map<uint64, Position1> *mbpmap);

    BarcodeProcessor(Options *opt, int mhashNum, int *mhashHead, node *mhashMap);

//    BarcodeProcessor(Options *opt, int mhashNum, int *mhashHead, node *mhashMap, uint64 *mBloomFilter);

    BarcodeProcessor(Options* opt, int* mbpmap_head, int* mbpmap_nxt,bpmap_key_value* mposition_all,BloomFilter *mbloomFilter);

    BarcodeProcessor();

    ~BarcodeProcessor();

    bool process(Read *read1, Read *read2);

    void dumpDNBmap(string &dnbMapFile);

private:
    void addPositionToName(Read *r, Position1* position, pair<string, string> *umi = NULL);

    void addPositionToNames(Read *r1, Read *r2, Position1* position, pair<string, string> *umi = NULL);

    void getUMI(Read *r, pair<string, string> &umi, bool isRead2 = false);

    void decodePosition(const uint32 codePos, pair<uint16, uint16> &decodePos);

    void decodePosition(const uint64 codePos, pair<uint32, uint32> &decodePos);

    uint32 encodePosition(int fovCol, int fovRow);

    uint64 encodePosition(uint32 x, uint32 y);

    long getBarcodeTypes();

    int getPosition(uint64 barcodeInt);

    int getPosition(string &barcodeString);

    void misMaskGenerate();
    void misMaskGenerateSegment();

    string positionToString(int position);

    string positionToString(Position1 *position);

//    unordered_map<uint64, Position1>::iterator getMisOverlap(uint64 barcodeInt);

    pair<int, int> getMisOverlap(uint64 barcodeInt);

    int getNOverlap(string &barcodeString, uint8 Nindex);

    int getNindex(string &barcodeString);

    void addDNB(uint64 barcodeInt);

    bool barcodeStatAndFilter(pair<string, string> &barcode);

    bool barcodeStatAndFilter(string &barcodeQ);

    bool umiStatAndFilter(pair<string, string> &umi);

    pair<int, int> queryMap(uint64 barcodeInt);

    int getMisOverlapHashTableOneArrayWithBloomFiler(uint64 barcodeInt,Position1 *&result_value);
    Position1* getPositionHashTableOneArrayWithBloomFiler(string& barcodeString);
    Position1* getPositionHashTableOneArrayWithBloomFiler(uint64 barcodeInt);


private:
    uint64 *misMask;
    int misMaskLen;
    int *misMaskLens;
    int misMaskClassificationNumber;
    int* misMaskClassification;
    uint64* misMaskLensSegmentL;
    uint64* misMaskLensSegmentR;
    uint64* misMaskHash;
    const char q10 = '+';
    const char q20 = '5';
    const char q30 = '?';
    string polyT = "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTT";
    uint64 polyTInt;
public:
    Options *mOptions;
    unordered_map<uint64, Position1> *bpmap;

    //***********list-hash add by ylf************//
    int *hashHead;
    node *hashMap;
    int hashNum;
    //******************************************//


    BloomFilter* bloomFilter;


    int* bpmap_head;
    int* bpmap_nxt;
    uint64* bpmap_key;
    int* bpmap_value;
    int* bpmap_len;
    bpmap_key_value *position_all;



    long totQuery = 0;
    long filterQuery = 0;
    long queryYes = 0;


    long totalReads = 0;//tot reads
    long mMapToSlideRead = 0;//tot mapped
    long overlapReads = 0;//mismatch=0 mapped
    long overlapReadsWithMis = 0;//mismatch>0 mapped
    long overlapReadsWithN = 0;
    long barcodeQ10 = 0;
    long barcodeQ20 = 0;
    long barcodeQ30 = 0;
    long umiQ10 = 0;
    long umiQ20 = 0;
    long umiQ30 = 0;
    long umiQ10FilterReads = 0;
    long umiNFilterReads = 0;
    long umiPloyAFilterReads = 0;
    unordered_map<uint64, int> mDNB;

    int mismatch;
    int barcodeLen;
};


#endif // ! BARCODE_PROCESSOR_H
