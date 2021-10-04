#ifndef  BARCODEPOSITIONMAP_H
#define BARCODEPOSITIONMAP_H

#include <string>
#include <cstring>
#include "fastqreader.h"
#include "read.h"
#include "util.h"
#include "options.h"
#include "chipMaskHDF5.h"
#include "BloomFilter.h"
#include <unordered_map>
#include <iomanip>
#include <set>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>


using namespace std;


class BarcodePositionMap {
public:
	//BarcodePositionMap(vector<string>& InFile, string MaskFile, int BarcodeStart, int BarcodeLen, int Segment, int TurnFovDegree, bool IsSeq500, int RC, string readidSep = "/");
	//BarcodePositionMap(string InFile, string MaskFile, int BarcodeStart, int BarcodeLen, int Segment, int TurnFovDegree, bool IsSeq500, int RC);
	//BarcodePositionMap(string InFile, int BarcodeStart, int BarcodeLen);
	BarcodePositionMap(Options* opt);
	~BarcodePositionMap();
private:
	void rangeRefresh(Position1& position);
public:
	long getBarcodeTypes();
	void dumpbpmap(string& mapOutFile);
	void loadbpmap();
	robin_hood::unordered_map<uint64, Position1>* getBpmap() { return &bpmap; }
    robin_hood::unordered_map<uint32, bpmap_segment_value>* getBpmapsegment() {return &bpmap_segment;}
    robin_hood::unordered_map<uint64, Position1>** getBpmaphash() {return bpmap_hash;}
    robin_hood::unordered_map<uint32, uint32>**  getBpmapHashIndex() {return bpmap_hash_index;}
    Position1*                                   getPosition() {return position_index;}
    int*                                         getHead(){return bpmap_head;}
    int*                                         getNext(){return bpmap_nxt;}
    uint64*                                      getKey() {return bpmap_key;}
    int*                                         getValue(){return bpmap_value;}
    int*                                         getLen(){return bpmap_len;}
    BloomFilter*                                 getBloomFilter(){return bloomFilter;}


public:
	robin_hood::unordered_map<uint64,  Position1> bpmap;
	/*
	 *  当前先处理25长度的问题，其他有情况在进行处理
	 *  MAP 嵌套 MAP
	 */
    robin_hood::unordered_map<uint32,  bpmap_segment_value> bpmap_segment;

    /*
	 *  当前先处理25长度的问题，其他有情况在进行处理
	 *  HashTable 嵌套 MAP
	 */
    robin_hood::unordered_map<uint64,  Position1> **bpmap_hash;

    /*
	 *  当前先处理25长度的问题，其他有情况在进行处理
	 *  HashTable 嵌套 MAP ，只存储index，Position1集中存储
	 */
    robin_hood::unordered_map<uint32,  uint32> **bpmap_hash_index;


    /*
     * HashTable 嵌套 HashTable 第一层前12位， 第二层mod79
     * 效果极差，极不推荐使用，应当立即销毁
     */
    vector<bpmap_vector_value> **bpmap_hash_vector;

    /*
     * HashTable Node
     */
    int *bpmap_head;
    int *bpmap_nxt;
    int *bpmap_value;
    uint64 *bpmap_key;

    int *bpmap_len;

    Position1* position_index;


    /*
     * 筛选专区
     */
    BloomFilter* bloomFilter;


    Options* mOptions;
	set<uint64> dupBarcode;
	long overlapBarcodes;
	long dupBarcodes;
	string maskFile;
	int barcodeStart;
	int barcodeLen;
	int segment;
	uint32 minX = OUTSIDE_DNB_POS_COL;
    uint32 minY = OUTSIDE_DNB_POS_ROW;
    uint32 maxX = 0;
    uint32 maxY = 0;
	vector<std::string> inMasks;
	int slidePitch = 500;
};





#endif // ! BARCODEPOSITIONMAP_H