#ifndef CHIPMASKHDF5_H
#define CHIPMASKHDF5_H

#include <iostream>
#include <hdf5.h>
#include <unordered_map>
#include "robin_hood.h"
#include "common.h"
#include "util.h"
#include <atomic>
#include <thread>

#define RANK 3
#define CDIM0 1537
#define CDIM1 1537
#define DATASETNAME "bpMatrix_"
#define ATTRIBUTEDIM 6   //rowShift, colShift, barcodeLength, slide pitch
#define ATTRIBUTEDIM1 2
#define ATTRIBUTENAME "dnbInfo"
#define ATTRIBUTENAME1 "chipInfo"

using namespace std;

class ChipMaskHDF5{
public:
    ChipMaskHDF5(std::string FileName);
    ~ChipMaskHDF5();

    void creatFile();
    herr_t writeDataSet(std::string chipID, slideRange& sliderange, robin_hood::unordered_map<uint64, Position1>& bpMap, uint32_t BarcodeLen, uint8_t segment, uint32_t slidePitch, uint compressionLevel = 6, int index = 1);
    void openFile();
    void readDataSet(robin_hood::unordered_map<uint64, Position1>& bpMap, int index = 1);
    void readDataSetSegment(robin_hood::unordered_map<uint32, bpmap_segment_value>& bpMapSegment, int index = 1);
    void readDataSetHash(robin_hood::unordered_map<uint64,  Position1> **&bpmap_hash, int index = 1);
    void readDataSetHashIndex(robin_hood::unordered_map<uint32,  uint32> **&bpmap_hash_index, Position1 *& position_index, int index = 1);
    void readDataSetHashVector(vector<bpmap_vector_value> **&bpmap_hash_vector, Position1 *& position_index, int index = 1);
    void readDataSetHashList(int *&bpmap_head, int *&bpmap_nxt, uint64 *&bpmap_key, int *&bpmap_value,Position1 *& position_index, int index = 1);
    void readDataSetHashListOrder(int *&bpmap_head, int *&bpmap_len, uint64 *&bpmap_key, int *&bpmap_value,Position1 *& position_index, int index = 1);
public:
    std::string fileName;
    hid_t fileID;
    uint64*** bpMatrix;
};

#endif 