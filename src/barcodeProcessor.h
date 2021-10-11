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
#include "BloomFilter.h"

using namespace std;

class BarcodeProcessor {
public:
	BarcodeProcessor(Options* opt, robin_hood::unordered_map<uint64, Position1>* mbpmap);
    BarcodeProcessor(Options* opt, robin_hood::unordered_map<uint32, bpmap_segment_value>* mbpmap_segment);
    BarcodeProcessor(Options* opt, robin_hood::unordered_map<uint64, Position1>* mbpmap, robin_hood::unordered_map<uint32, bpmap_segment_value>* mbpmap_segment);
    BarcodeProcessor(Options* opt, robin_hood::unordered_map<uint64, Position1>** mbpmap_hash);
    BarcodeProcessor(Options* opt, robin_hood::unordered_map<uint32, uint32>** mbpmap_hash_index, Position1* mposition1_index);
    BarcodeProcessor(Options* opt, int* mbpmap_head, int* mbpmap_nxt,uint64* mbpmap_key,int* mbpmap_value,Position1* mposition_index);
    BarcodeProcessor(Options* opt, int* mbpmap_head, int* mbpmap_len,uint64* mbpmap_key,int* mbpmap_value,Position1* mposition_index,bool ordered);
    BarcodeProcessor(Options* opt, int* mbpmap_head, int* mbpmap_nxt,uint64* mbpmap_key,Position1* mposition_index);
    BarcodeProcessor(Options* opt, int* mbpmap_head, int* mbpmap_nxt,uint64* mbpmap_key,Position1* mposition_index,BloomFilter *mbloomFilter);
    BarcodeProcessor(Options* opt, int* mbpmap_head, int* mbpmap_nxt,bpmap_key_value* mposition_all,BloomFilter *mbloomFilter);
    BarcodeProcessor();
	~BarcodeProcessor();
	bool process(Read* read1, Read* read2);
	void dumpDNBmap(string& dnbMapFile);
private:
	void addPositionToName(Read* r, Position1* position, pair<string, string>* umi = NULL);
	void addPositionToNames(Read* r1, Read* r2, Position1* position, pair<string, string>* umi = NULL);
	void  getUMI(Read* r, pair<string, string>& umi, bool isRead2=false);
	void decodePosition(const uint32 codePos, pair<uint16, uint16>& decodePos);
	void decodePosition(const uint64 codePos, pair<uint32, uint32>& decodePos);
	uint32 encodePosition(int fovCol, int fovRow);
	uint64 encodePosition(uint32 x, uint32 y);
	long getBarcodeTypes();

	Position1* getPosition(uint64 barcodeInt);
	Position1* getPositionSegment(uint64 barcodeInt);
    Position1* getPositionSegmentClassification(uint64 barcodeInt);
    Position1* getPositionHash(uint64 barcodeInt);
    Position1* getPositionHashIndex(uint64 barcodeInt);
    Position1* getPositionHashTable(uint64 barcodeInt);
    Position1* getPositionHashTableOrder(uint64 barcodeInt);
    Position1* getPositionHashTableNoIndex(uint64 barcodeInt);
    Position1* getPositionHashTableNoIndexWithBloomFiler(uint64 barcodeInt);
    Position1* getPositionHashTableOneArrayWithBloomFiler(uint64 barcodeInt);

    Position1* getPosition(string& barcodeString);
    Position1* getPositionSegment(string& barcodeString);
    Position1* getPositionSegmentClassification(string& barcodeString);
    Position1* getPositionHash(string& barcodeString);
    Position1* getPositionHashIndex(string& barcodeString);
    Position1* getPositionHashTable(string& barcodeString);
    Position1* getPositionHashTableOrder(string& barcodeString);
    Position1* getPositionHashTableNoIndex(string& barcodeString);
    Position1* getPositionHashTableNoIndexWithBloomFiler(string& barcodeString);
    Position1* getPositionHashTableOneArrayWithBloomFiler(string& barcodeString);


	void misMaskGenerate();
	void misMaskGenerateSegment();
	void misMaskGenerateHash();

	string positionToString(Position1* position);
	string positionToString(Position* position);
	robin_hood::unordered_map<uint64, Position1>::iterator getMisOverlap(uint64 barcodeInt);
//    robin_hood::unordered_map<uint32, Position1>::iterator getMisOverlapSegment(uint64 barcodeInt);
    /*
     * 返回状态码
     * 0: 符合规则 -1：不符合规则
     */
    int getMisOverlapSegment(uint64 barcodeInt,robin_hood::unordered_map<uint32, Position1>::iterator &result_iter_value);
    int getMisOverlapSegmentClassification(uint64 barcodeInt,robin_hood::unordered_map<uint32, Position1>::iterator &result_iter_value);
    int getMisOverlapHash(uint64 barcodeInt,robin_hood::unordered_map<uint64, Position1>::iterator &result_iter_value);
    int getMisOverlapHashIndex(uint64 barcodeInt,robin_hood::unordered_map<uint32, uint32>::iterator &result_iter_value);
    int getMisOverlapHashTable(uint64 barcodeInt,uint32 &result_value);
    int getMisOverlapHashTableOrder(uint64 barcodeInt,uint32 &result_value);
    int getMisOverlapHashTableNoIndex(uint64 barcodeInt,Position1 *&result_value);
    int getMisOverlapHashTableNoIndexWithBloomFiler(uint64 barcodeInt,Position1 *&result_value);
    int getMisOverlapHashTableOneArrayWithBloomFiler(uint64 barcodeInt,Position1 *&result_value);



    Position1* getNOverlap(string& barcodeString, uint8  Nindex);
	int getNindex(string& barcodeString);
	void addDNB(uint64 barcodeInt);
	bool barcodeStatAndFilter(pair<string, string>& barcode);
	bool barcodeStatAndFilter(string& barcodeQ);
	bool umiStatAndFilter (pair<string, string>& umi);
private:
	uint64* misMask;
	int misMaskLen;
	int* misMaskLens;
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
	Options* mOptions;
    robin_hood::unordered_map<uint64, Position1>* bpmap;
    /*
	 *  当前先处理25长度的问题，其他有情况在进行处理
	 */
    robin_hood::unordered_map<uint32,  bpmap_segment_value>* bpmap_segment;
    robin_hood::unordered_map<uint64,Position1> **bpmap_hash;

    robin_hood::unordered_map<uint32,uint32> **bpmap_hash_index;
    Position1* position_index;

    /*
     * Bloom Filter
     */
    BloomFilter* bloomFilter;


    int* bpmap_head;
    int* bpmap_nxt;
    uint64* bpmap_key;
    int* bpmap_value;
    int* bpmap_len;
    bpmap_key_value *position_all;


	long totalReads = 0;
	long mMapToSlideRead = 0;
	long overlapReads = 0;
	long overlapReadsWithMis = 0;
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
	robin_hood::unordered_map<uint64, int> mDNB;
	int mismatch;
	int barcodeLen;

	int64 MAPNUM;
};


#endif // ! BARCODE_PROCESSOR_H
