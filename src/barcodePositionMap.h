#ifndef  BARCODEPOSITIONMAP_H
#define BARCODEPOSITIONMAP_H

#include <string>
#include <cstring>
#include "fastqreader.h"
#include "read.h"
#include "util.h"
#include "options.h"
#include "chipMaskHDF5.h"

#include "bloomFilter.h"
#include <unordered_map>
//#include "robin_hood.h"
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
    BarcodePositionMap(Options *opt);

    ~BarcodePositionMap();

private:
    void rangeRefresh(Position1 &position);

public:
    long getBarcodeTypes();

    void dumpbpmap(string &mapOutFile);

    void loadbpmap();

    unordered_map<uint64, Position1> *getBpmap() { return &bpmap; };

    int *GetHashHead() const;

    node *GetHashMap() const;

    int GetHashNum() const;

    int GetDims1() const;

    BloomFilter *GetBloomFilter() const;

    Position1*                                   getPosition() {return position_index;}
    int*                                         getHead(){return bpmap_head;}
    int*                                         getNext(){return bpmap_nxt;}
    uint64*                                      getKey() {return bpmap_key;}
    int*                                         getValue(){return bpmap_value;}
    int*                                         getLen(){return bpmap_len;}
    BloomFilter*                                 getBloomFilter(){return bloomFilter;}
    bpmap_key_value*                             getPositionAll(){return position_all;}

public:
    unordered_map<uint64, Position1> bpmap;

    //***********list-hash add by ylf************//
    int *hashHead;
    node *hashMap;
    int hashNum;
    int dims1;
    //******************************************//


    //***********bloom filter add by ylf************//

    BloomFilter *bloomFilter;
//    uint64 *bloomFilter;

    //******************************************//

    int *bpmap_head;
    int *bpmap_nxt;
    int *bpmap_value;
    uint64 *bpmap_key;
    bpmap_key_value* position_all;
    int *bpmap_len;

    Position1* position_index;


    Options *mOptions;
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