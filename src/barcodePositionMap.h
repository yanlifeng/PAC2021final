#ifndef  BARCODEPOSITIONMAP_H
#define BARCODEPOSITIONMAP_H

#include <string>
#include <cstring>
#include "fastqreader.h"
#include "read.h"
#include "util.h"
#include "options.h"
#include "chipMaskHDF5.h"
#include <unordered_map>
#include <iomanip>
#include <set>
#include "myhash.h"


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
	uint32* getBpmap() { return hashmap; };
public:
	// unordered_map<uint64,  Position1> bpmap;
	// hash_map bpmap;
	uint32* hashmap;
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