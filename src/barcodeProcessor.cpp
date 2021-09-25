#include "barcodeProcessor.h"

BarcodeProcessor::BarcodeProcessor(Options *opt, robin_hood::unordered_map <uint64, Position1> *mbpmap) {
    mOptions = opt;
    bpmap = mbpmap;
    mismatch = opt->transBarcodeToPos.mismatch;
    barcodeLen = opt->barcodeLen;
    polyTInt = seqEncode(polyT.c_str(), 0, barcodeLen, mOptions->rc);
    misMaskGenerate();
}

BarcodeProcessor::BarcodeProcessor(Options *opt, robin_hood::unordered_map <uint32, bpmap_segment_value> *mbpmap_segment) {
    mOptions = opt;
    bpmap_segment = mbpmap_segment;
    mismatch = opt->transBarcodeToPos.mismatch;
    barcodeLen = opt->barcodeLen;
    polyTInt = seqEncode(polyT.c_str(), 0, barcodeLen, mOptions->rc);
    misMaskGenerateSegment();
}

BarcodeProcessor::BarcodeProcessor(Options *opt, robin_hood::unordered_map <uint64, Position1> *mbpmap, robin_hood::unordered_map <uint32, bpmap_segment_value> *mbpmap_segment) {
    mOptions = opt;
    bpmap = mbpmap;
    bpmap_segment = mbpmap_segment;
    mismatch = opt->transBarcodeToPos.mismatch;
    barcodeLen = opt->barcodeLen;
    polyTInt = seqEncode(polyT.c_str(), 0, barcodeLen, mOptions->rc);
    misMaskGenerateSegment();
}

BarcodeProcessor::BarcodeProcessor(Options *opt, robin_hood::unordered_map <uint64, Position1> **mbpmap_hash) {
    mOptions = opt;
    bpmap_hash = mbpmap_hash;
    mismatch = opt->transBarcodeToPos.mismatch;
    barcodeLen = opt->barcodeLen;
    polyTInt = seqEncode(polyT.c_str(), 0, barcodeLen, mOptions->rc);
    misMaskGenerateHash();
}

BarcodeProcessor::BarcodeProcessor(Options *opt, robin_hood::unordered_map<uint32,uint32> **mbpmap_hash_index, Position1* mposition1_index){
    mOptions = opt;
    bpmap_hash_index = mbpmap_hash_index;
    position_index = mposition1_index;
    mismatch = opt->transBarcodeToPos.mismatch;
    barcodeLen = opt->barcodeLen;
    polyTInt = seqEncode(polyT.c_str(), 0, barcodeLen, mOptions->rc);
    misMaskGenerateSegment();
}

BarcodeProcessor::BarcodeProcessor(Options* opt, int* mbpmap_head, int* mbpmap_nxt,uint64* mbpmap_key,int* mbpmap_value,Position1* mposition_index){
    mOptions = opt;
    bpmap_head = mbpmap_head;
    bpmap_nxt  = mbpmap_nxt;
    bpmap_key  = mbpmap_key;
    bpmap_value= mbpmap_value;
    position_index = mposition_index;
    mismatch = opt->transBarcodeToPos.mismatch;
    barcodeLen = opt->barcodeLen;
    polyTInt = seqEncode(polyT.c_str(), 0, barcodeLen, mOptions->rc);
    misMaskGenerate();
}



BarcodeProcessor::BarcodeProcessor() {
}

BarcodeProcessor::~BarcodeProcessor() {

}

bool BarcodeProcessor::process(Read *read1, Read *read2) {
    totalReads++;
    string barcode;
    string barcodeQ;
    if (mOptions->transBarcodeToPos.barcodeRead == 1) {
        barcode = read1->mSeq.mStr.substr(mOptions->barcodeStart, mOptions->barcodeLen);
        barcodeQ = read1->mQuality.substr(mOptions->barcodeStart, mOptions->barcodeLen);
    } else if (mOptions->transBarcodeToPos.barcodeRead == 2) {
        barcode = read2->mSeq.mStr.substr(mOptions->barcodeStart, mOptions->barcodeLen);
        barcodeQ = read2->mQuality.substr(mOptions->barcodeStart, mOptions->barcodeLen);
    } else {
        error_exit("barcodeRead must be 1 or 2 . please check the --barcodeRead option you give");
    }
    barcodeStatAndFilter(barcodeQ);
    Position1 *position = getPositionHashTable(barcode);
//    Position1 *position = getPositionHashIndex(barcode);
    //    Position1 *position = getPositionHash(barcode);
    //    Position1 *position = getPosition(barcode);


    if (position != nullptr) {
        mMapToSlideRead++;
        bool umiPassFilter = true;
        if (mOptions->transBarcodeToPos.umiStart >= 0 && mOptions->transBarcodeToPos.umiLen > 0) {
            pair <string, string> umi;
            if (mOptions->transBarcodeToPos.umiRead == 1) {
                getUMI(read1, umi);
            } else {
                getUMI(read2, umi, true);
            }
            umiPassFilter = umiStatAndFilter(umi);
            if (!mOptions->transBarcodeToPos.PEout) {
                addPositionToName(read2, position, &umi);
            } else {
                addPositionToNames(read1, read2, position, &umi);
            }
        } else {
            if (!mOptions->transBarcodeToPos.PEout) {
                addPositionToName(read2, position);
            } else {
                addPositionToNames(read1, read2, position);
            }
        }
        if (!mOptions->transBarcodeToPos.mappedDNBOutFile.empty())
            addDNB(encodePosition(position->x, position->y));

        return umiPassFilter;
    }
    return false;

}

void BarcodeProcessor::addPositionToName(Read *r, Position1 *position, pair <string, string> *umi) {
    string position_tag = positionToString(position);
    int readTagPos = r->mName.find("/");
    string readName;
    if (readTagPos != string::npos) {
        readName = r->mName.substr(0, readTagPos);
    } else {
        readName = r->mName;
    }
    if (umi == NULL)
        r->mName = readName + "|||CB:Z:" + position_tag;
    else {
        r->mName = readName + "|||CB:Z:" + position_tag + "|||UR:Z:" + umi->first + "|||UY:Z:" + umi->second;
    }
}

void BarcodeProcessor::addPositionToNames(Read *r1, Read *r2, Position1 *position, pair <string, string> *umi) {
    string position_tag = positionToString(position);
    int readTagPos = r1->mName.find("/");
    string readName;
    if (readTagPos != string::npos) {
        readName = r1->mName.substr(0, readTagPos);
    } else {
        readName = r1->mName;
    }
    if (umi == NULL) {
        r1->mName = readName + "|||CB:Z:" + position_tag;
        if (mOptions->transBarcodeToPos.barcodeRead == 1) {
            r1->trimBack(mOptions->barcodeStart);
        } else {
            r2->trimBack(mOptions->barcodeStart);
        }
        r2->mName = r1->mName;
    } else {
        r1->mName = readName + "|||CB:Z:" + position_tag + "|||UR:Z:" + umi->first + "|||UY:Z:" + umi->second;
        if (mOptions->transBarcodeToPos.umiRead == 1 && mOptions->transBarcodeToPos.barcodeRead == 1) {
            int trimStart =
                    mOptions->barcodeStart > mOptions->transBarcodeToPos.umiStart ? mOptions->transBarcodeToPos.umiStart
                                                                                  : mOptions->barcodeStart;
            r1->trimBack(trimStart);
        } else {
            r2->trimBack(mOptions->barcodeStart);
        }
        r2->mName = r1->mName;
    }
}

void BarcodeProcessor::getUMI(Read *r, pair <string, string> &umi, bool isRead2) {
    string umiSeq = r->mSeq.mStr.substr(mOptions->transBarcodeToPos.umiStart, mOptions->transBarcodeToPos.umiLen);
    string umiQ = r->mQuality.substr(mOptions->transBarcodeToPos.umiStart, mOptions->transBarcodeToPos.umiLen);
    umi.first = umiSeq;
    umi.second = umiQ;
    if (isRead2) {
        r->mSeq.mStr = r->mSeq.mStr.substr(0, mOptions->transBarcodeToPos.umiStart);
        r->mQuality = r->mQuality.substr(0, mOptions->transBarcodeToPos.umiStart);
    }
}


void BarcodeProcessor::decodePosition(const uint32 codePos, pair <uint16, uint16> &decodePos) {
    decodePos.first = codePos >> 16;
    decodePos.second = codePos & 0x0000FFFF;
}

void BarcodeProcessor::decodePosition(const uint64 codePos, pair <uint32, uint32> &decodePos) {
    decodePos.first = codePos >> 32;
    decodePos.second = codePos & 0x00000000FFFFFFFF;
}

uint32 BarcodeProcessor::encodePosition(int fovCol, int fovRow) {
    uint32 encodePos = (fovCol << 16) | fovRow;
    return encodePos;
}

uint64 BarcodeProcessor::encodePosition(uint32 x, uint32 y) {
    uint64 encodePos = ((uint64) x << 32) | (uint64) y;
    return encodePos;
}

long BarcodeProcessor::getBarcodeTypes() {
    return bpmap->size();
}

Position1 *BarcodeProcessor::getPosition(uint64 barcodeInt) {
    robin_hood::unordered_map<uint64, Position1>::iterator iter = bpmap->find(barcodeInt);
    if (iter != bpmap->end()) {
        overlapReads++;
        return &iter->second;
    } else if (mismatch > 0) {
        iter = getMisOverlap(barcodeInt);
        if (iter != bpmap->end()) {
            overlapReadsWithMis++;
            return &iter->second;
        } else {
            return nullptr;
        }
    }
    return nullptr;
}


Position1 *BarcodeProcessor::getPositionSegment(uint64 barcodeInt){
    robin_hood::unordered_map<uint32, bpmap_segment_value>::iterator iter_key = bpmap_segment->find(getMapKey(barcodeInt));
    robin_hood::unordered_map<uint32, Position1>::iterator iter_value;

    if (iter_key != bpmap_segment->end()) {
        iter_value = iter_key->second.segment.find(getMapValue(barcodeInt));
        if (iter_value != iter_key->second.segment.end()) {
            overlapReads++;
            return &iter_value->second;
        }
    }


    if (mismatch > 0) {
//        printf("In this Segment!\n");
        int mis_status = getMisOverlapSegment(barcodeInt,iter_value);
        if (mis_status == 0) {
            overlapReadsWithMis++;
            return &iter_value->second;
        } else {
            return nullptr;
        }
    }

    return nullptr;
}

Position1 *BarcodeProcessor::getPositionSegmentClassification(uint64 barcodeInt){
    robin_hood::unordered_map<uint32, bpmap_segment_value>::iterator iter_key = bpmap_segment->find(getMapKey(barcodeInt));
    robin_hood::unordered_map<uint32, Position1>::iterator iter_value;

    if (iter_key != bpmap_segment->end()) {
        iter_value = iter_key->second.segment.find(getMapValue(barcodeInt));
        if (iter_value != iter_key->second.segment.end()) {
            overlapReads++;
            return &iter_value->second;
        }
    }


    if (mismatch > 0) {
//        printf("In this Segment!\n");
        int mis_status = getMisOverlapSegmentClassification(barcodeInt,iter_value);
        if (mis_status == 0) {
            overlapReadsWithMis++;
            return &iter_value->second;
        } else {
            return nullptr;
        }
    }

    return nullptr;
}

Position1 *BarcodeProcessor::getPositionHash(uint64 barcodeInt){
    robin_hood::unordered_map<uint64, Position1>::iterator iter;
//    uint32 mapkey = getMapKey(barcodeInt)%HashTable;
    uint32 mapkey = getMapKey(barcodeInt);
    if (bpmap_hash[mapkey] != nullptr){
         iter = bpmap_hash[mapkey]->find(barcodeInt);

        if (iter != bpmap_hash[mapkey]->end()) {
            overlapReads++;
            return &iter->second;
        }
    }
//    cerr << "OK" << endl;

    if (mismatch > 0) {
//        printf("In this Segment!\n");
        int mis_status = getMisOverlapHash(barcodeInt,iter);
        if (mis_status == 0) {
            overlapReadsWithMis++;
            return &iter->second;
        } else {
            return nullptr;
        }
    }

    return nullptr;
}

Position1 *BarcodeProcessor::getPositionHashIndex(uint64 barcodeInt){
    robin_hood::unordered_map<uint32, uint32>::iterator iter;
//    uint32 mapkey = getMapKey(barcodeInt)%HashTable;
    uint32 mapkey = getMapKey(barcodeInt);
    if (bpmap_hash_index[mapkey] != nullptr){
        iter = bpmap_hash_index[mapkey]->find(getMapValue(barcodeInt));

        if (iter != bpmap_hash_index[mapkey]->end()) {
            overlapReads++;
            return &position_index[iter->second];
        }
    }
//    cerr << "OK" << endl;

    if (mismatch > 0) {
//        printf("In this Segment!\n");
        int mis_status = getMisOverlapHashIndex(barcodeInt,iter);
        if (mis_status == 0) {
            overlapReadsWithMis++;
            return &position_index[iter->second];
        } else {
            return nullptr;
        }
    }

    return nullptr;
}

Position1 *BarcodeProcessor::getPositionHashTable(uint64 barcodeInt){
    uint32 mapKey = barcodeInt%MOD;
    for (int i=bpmap_head[mapKey];i!=-1;i=bpmap_nxt[i]){
        if (bpmap_key[i] == barcodeInt){
            overlapReads++;
            return &position_index[bpmap_value[i]];
        }
    }
//    cerr << " in this Ok \n" << endl;
    if (mismatch > 0) {
        int mis_status;
        uint32 result_value;
        mis_status = getMisOverlapHashTable(barcodeInt,result_value);
        if (mis_status == 0) {
            overlapReadsWithMis++;
            return &position_index[result_value];
        } else {
            return nullptr;
        }
    }
    return nullptr;
}

Position1 *BarcodeProcessor::getPosition(string &barcodeString) {
    int Nindex = getNindex(barcodeString);
    if (Nindex == -1) {
        uint64 barcodeInt = seqEncode(barcodeString.c_str(), 0, barcodeLen);
        if (barcodeInt == polyTInt) {
            return nullptr;
        }
        return getPosition(barcodeInt);
    } else if (Nindex == -2) {
        return nullptr;
    } else if (mismatch > 0) {

        return getNOverlap(barcodeString, Nindex);
    }
    return nullptr;
}

Position1 *BarcodeProcessor::getPositionSegment(string &barcodeString) {
    int Nindex = getNindex(barcodeString);
    if (Nindex == -1) {
        uint64 barcodeInt = seqEncode(barcodeString.c_str(), 0, barcodeLen);
        if (barcodeInt == polyTInt) {
            return nullptr;
        }
        return getPositionSegment(barcodeInt);
    } else if (Nindex == -2) {
        return nullptr;
    } else if (mismatch > 0) {
        printf("In this !!!!\n");
        return getNOverlap(barcodeString, Nindex);
    }
    return nullptr;
}

Position1 *BarcodeProcessor::getPositionSegmentClassification(string &barcodeString) {
    int Nindex = getNindex(barcodeString);
    if (Nindex == -1) {
        uint64 barcodeInt = seqEncode(barcodeString.c_str(), 0, barcodeLen);
        if (barcodeInt == polyTInt) {
            return nullptr;
        }
        return getPositionSegmentClassification(barcodeInt);
    } else if (Nindex == -2) {
        return nullptr;
    } else if (mismatch > 0) {
        printf("In this !!!!\n");
        return getNOverlap(barcodeString, Nindex);
    }
    return nullptr;
}

Position1 *BarcodeProcessor::getPositionHash(string &barcodeString) {
    int Nindex = getNindex(barcodeString);
    if (Nindex == -1) {
        uint64 barcodeInt = seqEncode(barcodeString.c_str(), 0, barcodeLen);
        if (barcodeInt == polyTInt) {
            return nullptr;
        }
        return getPositionHash(barcodeInt);
    } else if (Nindex == -2) {
        return nullptr;
    } else if (mismatch > 0) {
        printf("In this !!!!\n");
        return getNOverlap(barcodeString, Nindex);
    }
    return nullptr;
}

Position1 *BarcodeProcessor::getPositionHashIndex(string &barcodeString) {
    int Nindex = getNindex(barcodeString);
    if (Nindex == -1) {
        uint64 barcodeInt = seqEncode(barcodeString.c_str(), 0, barcodeLen);
        if (barcodeInt == polyTInt) {
            return nullptr;
        }
        return getPositionHashIndex(barcodeInt);
    } else if (Nindex == -2) {
        return nullptr;
    } else if (mismatch > 0) {
        printf("In this !!!!\n");
        return getNOverlap(barcodeString, Nindex);
    }
    return nullptr;
}

Position1 *BarcodeProcessor::getPositionHashTable(string &barcodeString) {
    int Nindex = getNindex(barcodeString);
    if (Nindex == -1) {
        uint64 barcodeInt = seqEncode(barcodeString.c_str(), 0, barcodeLen);
        if (barcodeInt == polyTInt) {
            return nullptr;
        }
        return getPositionHashTable(barcodeInt);
    } else if (Nindex == -2) {
        return nullptr;
    } else if (mismatch > 0) {
        printf("In this !!!!\n");
        return getNOverlap(barcodeString, Nindex);
    }
    return nullptr;
}



void BarcodeProcessor::misMaskGenerate() {
    misMaskLen = possibleMis(barcodeLen, mismatch);
    misMaskLens = new int[mismatch];
    for (int i = 0; i < mismatch; i++) {
        misMaskLens[i] = possibleMis(barcodeLen, i + 1);
    }

    misMask = (uint64 *) malloc(misMaskLen * sizeof(uint64));
    set <uint64> misMaskSet;
    int index = 0;
    if (mismatch > 0) {
        for (int i = 0; i < barcodeLen; i++) {
            for (uint64 j = 1; j < 4; j++) {
                uint64 misMaskInt = j << i * 2;
                misMask[index] = misMaskInt;
                index++;
            }
        }
        if (mOptions->verbose) {
            string msg = "1 mismatch mask barcode number: " + to_string(index);
            loginfo(msg);
        }
    }
    if (mismatch == 2) {
        misMaskSet.clear();
        for (int i = 0; i < barcodeLen; i++) {
            for (uint64 j = 1; j < 4; j++) {
                uint64 misMaskInt1 = j << i * 2;
                for (int k = 0; k < barcodeLen; k++) {
                    if (k == i) {
                        continue;
                    }
                    for (uint64 j2 = 1; j2 < 4; j2++) {
                        uint64 misMaskInt2 = j2 << k * 2;
                        uint64 misMaskInt = misMaskInt1 | misMaskInt2;
                        misMaskSet.insert(misMaskInt);
                    }
                }
            }
        }
        for (auto iter = misMaskSet.begin(); iter != misMaskSet.end(); iter++) {
            misMask[index] = *iter;
            index++;
        }
        if (mOptions->verbose) {
            string msg = "2 mismatch mask barcode number: " + to_string(misMaskSet.size());
            loginfo(msg);
        }
    }
    if (mismatch == 3) {
        misMaskSet.clear();
        for (int i = 0; i < barcodeLen; i++) {
            for (uint64 j = 1; j < 4; j++) {
                uint64 misMaskInt1 = j << i * 2;
                for (int k = 0; k < barcodeLen; k++) {
                    if (k == i) {
                        continue;
                    }
                    for (uint64 j2 = 1; j2 < 4; j2++) {
                        uint64 misMaskInt2 = j2 << k * 2;
                        for (int h = 0; h < barcodeLen; h++) {
                            if (h == k || h == i) {
                                continue;
                            }
                            for (uint64 j3 = 1; j3 < 4; j3++) {
                                uint64 misMaskInt3 = j3 << h * 2;
                                uint64 misMaskInt = misMaskInt1 | misMaskInt2 | misMaskInt3;
                                misMaskSet.insert(misMaskInt);
                            }
                        }
                    }
                }
            }
        }
        for (auto iter = misMaskSet.begin(); iter != misMaskSet.end(); iter++) {
            misMask[index] = *iter;
            index++;
        }
        if (mOptions->verbose) {
            string msg = "3 mismatch mask barcode number: " + to_string(misMaskSet.size());
            loginfo(msg);
        }
        misMaskSet.clear();
    }
    if (mOptions->verbose) {
        string msg = "total mismatch mask length: " + to_string(misMaskLen);
        loginfo(msg);
    }
}

void BarcodeProcessor::misMaskGenerateSegment() {
    misMaskLen = barcodeLen*3;
    misMask = new uint64[misMaskLen];
    int index=0;
    for (int i = 0; i < barcodeLen; i++) {
        for (uint64 j = 1; j < 4; j++) {
            uint64 misMaskInt = j << (i * 2);
            misMask[index] = misMaskInt;
            index++;
        }
    }
    /*
     *  int misMaskClassificationNumber;
	 *  int* misMaskClassification;
	 *  int* misMaskLensSegmentL;
     *  int* misMaskLensSegmentR;
     */


    misMaskClassification = new int[700];
    misMaskLensSegmentL = new uint32[9*(barcodeLen*(barcodeLen-1))/2];
    misMaskLensSegmentR = new uint32[9*(barcodeLen*(barcodeLen-1))/2];
    index = 0;
    int Classificationindex = 0;
    for (int i=0;i<barcodeLen-1;i++){
        for (uint64 k1=1;k1<4;k1++){
            for (int j=i+1;j<barcodeLen;j++){
                for (uint64 k2=1;k2<4;k2++){
                    uint64 misMaskInt = (k1<<(i*2)) | (k2<<(j*2));
                    misMaskLensSegmentL[index] = getMapKey(misMaskInt);
                    misMaskLensSegmentR[index] = getMapValue(misMaskInt);
                    index++;
                    if (j<=11) {
                        misMaskClassification[Classificationindex] = index;
                        Classificationindex++;
                    }
                }
            }
            if (i<=11){
                misMaskClassification[Classificationindex] = index;
                Classificationindex++;
            }
        }
    }
    misMaskClassification[Classificationindex] = index;
    Classificationindex++;
    misMaskClassificationNumber = Classificationindex;
//    cerr << "misMaskClassificationNumber is" << misMaskClassificationNumber << endl;
//    for (int i=0;i<misMaskClassificationNumber;i++){
//        printf("Class is %d , index is %d\n",i,misMaskClassification[i]);
//    }
//    cerr << "Index is " << index << endl;
//    cerr << "Classificationindex is " << Classificationindex << endl;
}
void BarcodeProcessor::misMaskGenerateHash(){
    misMaskLen = barcodeLen*3;
    misMask = new uint64[misMaskLen];
    int index=0;
    for (int i = 0; i < barcodeLen; i++) {
        for (uint64 j = 1; j < 4; j++) {
            uint64 misMaskInt = j << (i * 2);
            misMask[index] = misMaskInt;
            index++;
        }
    }
    /*
     *  int misMaskClassificationNumber;
	 *  int* misMaskClassification;
	 *  int* misMaskLensSegmentL;
     *  int* misMaskLensSegmentR;
     */


    misMaskClassification = new int[700];
    misMaskHash = new uint64[9*(barcodeLen*(barcodeLen-1))/2];
    index = 0;
    int Classificationindex = 0;
    for (int i=0;i<barcodeLen-1;i++){
        for (uint64 k1=1;k1<4;k1++){
            for (int j=i+1;j<barcodeLen;j++){
                for (uint64 k2=1;k2<4;k2++){
                    uint64 misMaskInt = (k1<<(i*2)) | (k2<<(j*2));
                    misMaskHash[index] = misMaskInt;
                    index++;
                    if (j<=11) {
                        misMaskClassification[Classificationindex] = index;
                        Classificationindex++;
                    }
                }
            }
            if (i<=11){
                misMaskClassification[Classificationindex] = index;
                Classificationindex++;
            }
        }
    }
    misMaskClassification[Classificationindex] = index;
    Classificationindex++;
    misMaskClassificationNumber = Classificationindex;
//    cerr << "Mis Mask is Complete" << endl;
//    cerr << "misMaskClassificationNumber is" << misMaskClassificationNumber << endl;
//    for (int i=0;i<misMaskClassificationNumber;i++){
//        printf("Class is %d , index is %d\n",i,misMaskClassification[i]);
//    }
//    cerr << "Index is " << index << endl;
//    cerr << "Classificationindex is " << Classificationindex << endl;
}

string BarcodeProcessor::positionToString(Position *position) {
    stringstream positionString;
    positionString << position->x << "_" << position->y;
    return positionString.str();
}

string BarcodeProcessor::positionToString(Position1 *position) {
    stringstream positionString;
    positionString << position->x << "_" << position->y;
    return positionString.str();
}

robin_hood::unordered_map<uint64, Position1>::iterator BarcodeProcessor::getMisOverlap(uint64 barcodeInt) {
    uint64 misBarcodeInt;
    int misCount = 0;
    int misMaskIndex = 0;
    robin_hood::unordered_map<uint64, Position1>::iterator iter;
    robin_hood::unordered_map<uint64, Position1>::iterator overlapIter;

    for (int mis = 0; mis < mismatch; mis++) {
        misCount = 0;
        while (misMaskIndex < misMaskLens[mis]) {
            misBarcodeInt = barcodeInt ^ misMask[misMaskIndex];
            misMaskIndex++;
            iter = bpmap->find(misBarcodeInt);
            if (iter != bpmap->end()) {
                overlapIter = iter;
                misCount++;
                if (misCount > 1) {
                    return bpmap->end();
                }
            }
        }
        if (misCount == 1) {
            return overlapIter;
        }
    }

    return bpmap->end();
}

int BarcodeProcessor::getMisOverlapSegment(uint64 barcodeInt,
                                           robin_hood::unordered_map<uint32, Position1>::iterator &result_iter_value) {
    uint64 misBarcodeInt;
    int misCount = 0;
    int misMaskIndex = 0;
    robin_hood::unordered_map<uint32, bpmap_segment_value>::iterator iter_key;
    robin_hood::unordered_map<uint32, Position1>::iterator iter_value;
//    robin_hood::unordered_map<uint32, Position1>::iterator overlapIter_value;

    for (int mis = 0; mis < mismatch; mis++) {
        misCount = 0;
        while (misMaskIndex < misMaskLens[mis]) {

            misBarcodeInt = barcodeInt ^ misMask[misMaskIndex];

            misMaskIndex++;

            iter_key = bpmap_segment->find(getMapKey(misBarcodeInt));

            if (iter_key != bpmap_segment->end()) {

                iter_value = iter_key->second.segment.find(getMapValue(misBarcodeInt));

                if (iter_value != iter_key->second.segment.end()){
                    result_iter_value = iter_value;
                    misCount++;
                    if (misCount > 1) {
                        return -1;
                    }
                }

            }
        }
        if (misCount == 1) {
            return 0;
        }
    }



    return -1;
}

int BarcodeProcessor::getMisOverlapSegmentClassification(uint64 barcodeInt,
                                           robin_hood::unordered_map<uint32, Position1>::iterator &result_iter_value) {
    uint64 misBarcodeInt;
    int misCount = 0;

    robin_hood::unordered_map<uint32, bpmap_segment_value>::iterator iter_key;
    robin_hood::unordered_map<uint32, Position1>::iterator iter_value;
//    robin_hood::unordered_map<uint32, Position1>::iterator overlapIter_value;

/*
 *  处理mismatch == 1 的情况
 */

    for (int i=0;i<misMaskLen;i++){
        misBarcodeInt = barcodeInt ^ misMask[i];
        iter_key = bpmap_segment->find(getMapKey(misBarcodeInt));
        if (iter_key != bpmap_segment->end()) {
            iter_value = iter_key->second.segment.find(getMapValue(misBarcodeInt));
            if (iter_value != iter_key->second.segment.end()){
                result_iter_value = iter_value;
                misCount++;
                if (misCount > 1) {
                    return -1;
                }
            }
        }
    }
    if (misCount== 1) return 0;

    if (mismatch < 2) return -1;



    uint32 mapkey   = getMapKey(barcodeInt);
    uint32 mapValue = getMapValue(barcodeInt);

//    cerr << "misMaskClassificationNumber is " << misMaskClassificationNumber << endl;


//  顺序枚举
    int misMaskIndex = 0;
    for (int i=0;i<misMaskClassificationNumber;i++){

        iter_key = bpmap_segment->find(mapkey^misMaskLensSegmentL[misMaskIndex]);
        if (iter_key == bpmap_segment->end()){
            misMaskIndex = misMaskClassification[i];
            continue;
        }
        while (misMaskIndex < misMaskClassification[i]){
            uint32 misBarcodeIntValue = mapValue^misMaskLensSegmentR[misMaskIndex];
            misMaskIndex++;
            if (misBarcodeIntValue < iter_key->second.minvalue || misBarcodeIntValue > iter_key->second.maxvalue) {
                continue;
            }
            iter_value = iter_key->second.segment.find(misBarcodeIntValue);
            if (iter_value != iter_key->second.segment.end()){
                result_iter_value = iter_value;
                misCount++;
                if (misCount > 1) {
                    return -1;
                }
            }
        }
    }



/*
    // 逆序枚举
    int misMaskIndex = misMaskClassification[misMaskClassificationNumber-1]-1;
    for (int i=misMaskClassificationNumber-1;i>=0;i--){

        iter_key = bpmap_segment->find(mapkey^misMaskLensSegmentL[misMaskIndex]);
        if (iter_key == bpmap_segment->end()){
            if (i>0) misMaskIndex = misMaskClassification[i-1]-1;
            continue;
        }
        int MisEnd = i>0?misMaskClassification[i-1]:0;
        while (misMaskIndex >= MisEnd){
            uint32 misBarcodeIntValue = mapValue^misMaskLensSegmentR[misMaskIndex];
            misMaskIndex--;
            if (misBarcodeIntValue < iter_key->second.minvalue || misBarcodeIntValue > iter_key->second.maxvalue) {
                continue;
            }
            iter_value = iter_key->second.segment.find(misBarcodeIntValue);
            if (iter_value != iter_key->second.segment.end()){
                result_iter_value = iter_value;
                misCount++;
                if (misCount > 1) {
                    return -1;
                }
            }
        }
    }

*/
    if (misCount== 1) return 0;

    return -1;
}

int BarcodeProcessor::getMisOverlapHash(uint64 barcodeInt,
                                                         robin_hood::unordered_map<uint64, Position1>::iterator &result_iter_value) {

//    cerr << "getMisOverlapHash Start" << endl;
    uint64 misBarcodeInt;
    int misCount = 0;

    robin_hood::unordered_map<uint64, Position1>::iterator iter;
//    robin_hood::unordered_map<uint32, Position1>::iterator overlapIter_value;

/*
 *  处理mismatch == 1 的情况
 */

    uint32 mapkey;
    for (int i=0;i<misMaskLen;i++){

        misBarcodeInt = barcodeInt ^ misMask[i];
//        cerr << 1 << endl;
//        mapkey = getMapKey(misBarcodeInt)%HashTable;
        mapkey = getMapKey(misBarcodeInt);
//        iter = bpmap_hash[796856]->find(1111);
        if (bpmap_hash[mapkey] == nullptr) {
            continue;
        }
        iter = bpmap_hash[mapkey]->find(misBarcodeInt);
//        cerr << 2 << endl;
        if (iter != bpmap_hash[mapkey]->end()) {
            result_iter_value = iter;
            misCount++;
            if (misCount > 1) {
//                cerr << "getMisOverlapHash End" << endl;
                return -1;
            }
        }
    }
    if (misCount== 1) {
//        cerr << "getMisOverlapHash End" << endl;
        return 0;
    }

    if (mismatch < 2){
//        cerr << "getMisOverlapHash End" << endl;
        return -1;
    }



//    cerr << "getMisOverlapHash Midden" << endl;



//    cerr << "misMaskClassificationNumber is " << misMaskClassificationNumber << endl;


//  顺序枚举
    int misMaskIndex = 0;
    for (int i=0;i<misMaskClassificationNumber;i++){
//        mapkey = getMapKey((misMaskHash[misMaskIndex]^barcodeInt))%HashTable;
        mapkey = getMapKey((misMaskHash[misMaskIndex]^barcodeInt));
        if (bpmap_hash[mapkey] == nullptr){
            misMaskIndex = misMaskClassification[i];
            continue;
        }
        while (misMaskIndex < misMaskClassification[i]){
            uint64 misBarcodeInt = barcodeInt^misMaskHash[misMaskIndex];
            misMaskIndex++;
            iter = bpmap_hash[mapkey]->find(misBarcodeInt);
            if (iter != bpmap_hash[mapkey]->end()){
                result_iter_value = iter;
                misCount++;
                if (misCount > 1) {
//                    cerr << "getMisOverlapHash End" << endl;
                    return -1;
                }
            }
        }
    }



/*
    // 逆序枚举
    int misMaskIndex = misMaskClassification[misMaskClassificationNumber-1]-1;
    for (int i=misMaskClassificationNumber-1;i>=0;i--){

        iter_key = bpmap_segment->find(mapkey^misMaskLensSegmentL[misMaskIndex]);
        if (iter_key == bpmap_segment->end()){
            if (i>0) misMaskIndex = misMaskClassification[i-1]-1;
            continue;
        }
        int MisEnd = i>0?misMaskClassification[i-1]:0;
        while (misMaskIndex >= MisEnd){
            uint32 misBarcodeIntValue = mapValue^misMaskLensSegmentR[misMaskIndex];
            misMaskIndex--;
            if (misBarcodeIntValue < iter_key->second.minvalue || misBarcodeIntValue > iter_key->second.maxvalue) {
                continue;
            }
            iter_value = iter_key->second.segment.find(misBarcodeIntValue);
            if (iter_value != iter_key->second.segment.end()){
                result_iter_value = iter_value;
                misCount++;
                if (misCount > 1) {
                    return -1;
                }
            }
        }
    }

*/
    if (misCount== 1) {
//        cerr << "getMisOverlapHash End" << endl;
        return 0;
    }
//    cerr << "getMisOverlapHash End" << endl;
    return -1;
}

int BarcodeProcessor::getMisOverlapHashIndex(uint64 barcodeInt,
                                             robin_hood::unordered_map<uint32, uint32>::iterator &result_iter_value){
    uint64 misBarcodeInt;
    int misCount = 0;

    robin_hood::unordered_map<uint32, uint32>::iterator iter_value;

/*
 *  处理mismatch == 1 的情况
 */

    for (int i=0;i<misMaskLen;i++){
        misBarcodeInt = barcodeInt ^ misMask[i];
        uint32 mapkey = getMapKey(misBarcodeInt);
        if (bpmap_hash_index[mapkey]!=nullptr) {
            iter_value = bpmap_hash_index[mapkey]->find(getMapValue(misBarcodeInt));
            if (iter_value != bpmap_hash_index[mapkey]->end()){
                result_iter_value = iter_value;
                misCount++;
                if (misCount > 1) {
                    return -1;
                }
            }
        }
    }
    if (misCount== 1) return 0;

    if (mismatch < 2) return -1;



    uint32 mapkey   = getMapKey(barcodeInt);
    uint32 mapValue = getMapValue(barcodeInt);

//    cerr << "misMaskClassificationNumber is " << misMaskClassificationNumber << endl;


//  顺序枚举
    int misMaskIndex = 0;
    for (int i=0;i<misMaskClassificationNumber;i++){

        if (bpmap_hash_index[mapkey^misMaskLensSegmentL[misMaskIndex]] == nullptr){
            misMaskIndex = misMaskClassification[i];
            continue;
        }
        while (misMaskIndex < misMaskClassification[i]){
            uint32 misBarcodeIntValue = mapValue^misMaskLensSegmentR[misMaskIndex];
            iter_value = bpmap_hash_index[mapkey^misMaskLensSegmentL[misMaskIndex]]->find(misBarcodeIntValue);
            if (iter_value != bpmap_hash_index[mapkey^misMaskLensSegmentL[misMaskIndex]]->end()){
                result_iter_value = iter_value;
                misCount++;
                if (misCount > 1) {
                    return -1;
                }
            }
            misMaskIndex++;
        }
    }



/*
    // 逆序枚举
    int misMaskIndex = misMaskClassification[misMaskClassificationNumber-1]-1;
    for (int i=misMaskClassificationNumber-1;i>=0;i--){

        iter_key = bpmap_segment->find(mapkey^misMaskLensSegmentL[misMaskIndex]);
        if (iter_key == bpmap_segment->end()){
            if (i>0) misMaskIndex = misMaskClassification[i-1]-1;
            continue;
        }
        int MisEnd = i>0?misMaskClassification[i-1]:0;
        while (misMaskIndex >= MisEnd){
            uint32 misBarcodeIntValue = mapValue^misMaskLensSegmentR[misMaskIndex];
            misMaskIndex--;
            if (misBarcodeIntValue < iter_key->second.minvalue || misBarcodeIntValue > iter_key->second.maxvalue) {
                continue;
            }
            iter_value = iter_key->second.segment.find(misBarcodeIntValue);
            if (iter_value != iter_key->second.segment.end()){
                result_iter_value = iter_value;
                misCount++;
                if (misCount > 1) {
                    return -1;
                }
            }
        }
    }

*/
    if (misCount== 1) return 0;

    return -1;
}

int BarcodeProcessor::getMisOverlapHashTable(uint64 barcodeInt, uint32 &result_value){
    uint64 misBarcodeInt;
    int misCount = 0;
    int misMaskIndex = 0;
    for (int mis = 0; mis < mismatch; mis++) {
        misCount = 0;
        while (misMaskIndex < misMaskLens[mis]) {
            misBarcodeInt = barcodeInt ^ misMask[misMaskIndex];
            misMaskIndex++;
            uint32 mapKey = misBarcodeInt%MOD;
            for (int i=bpmap_head[mapKey];i!=-1;i=bpmap_nxt[i]){
                if (bpmap_key[i] == misBarcodeInt){
                    result_value = bpmap_value[i];
                    misCount++;
                    if (misCount > 1){
                        return -1;
                    }
                    break;
                }
            }
        }
        if (misCount == 1) {
            return 0;
        }
    }

    return -1;
}



Position1 *BarcodeProcessor::getNOverlap(string &barcodeString, uint8 Nindex) {
    //N has the same encode (11) with G
    int misCount = 0;
    uint64 barcodeInt = seqEncode(barcodeString.c_str(), 0, barcodeString.length());
    robin_hood::unordered_map<uint64, Position1>::iterator iter;
    robin_hood::unordered_map<uint64, Position1>::iterator overlapIter;
    iter = bpmap->find(barcodeInt);
    if (iter != bpmap->end()) {
        misCount++;
        overlapIter = iter;
    }
    for (uint64 j = 1; j < 4; j++) {
        uint64 misBarcodeInt = barcodeInt ^ (j << Nindex * 2);
        iter = bpmap->find(misBarcodeInt);
        if (iter != bpmap->end()) {
            misCount++;
            if (misCount > 1) {
                return nullptr;
            }
            overlapIter = iter;
        }
    }
    if (misCount == 1) {
        overlapReadsWithN++;
        return &overlapIter->second;
    }
    return nullptr;
}

int BarcodeProcessor::getNindex(string &barcodeString) {
    int Nindex = barcodeString.find("N");
    if (Nindex == barcodeString.npos) {
        return -1;
    } else if (Nindex != barcodeString.rfind("N")) {
        return -2;
    }
    return Nindex;
}

void BarcodeProcessor::addDNB(uint64 barcodeInt) {
    if (mDNB.count(barcodeInt) > 0) {
        mDNB[barcodeInt]++;
    } else {
        mDNB[barcodeInt]++;
    }
}

bool BarcodeProcessor::barcodeStatAndFilter(pair <string, string> &barcode) {
    for (int i = 0; i < barcodeLen; i++) {
        if (barcode.second[i] >= q30) {
            barcodeQ30++;
            barcodeQ20++;
            barcodeQ10++;
        } else if (barcode.second[i] >= q20) {
            barcodeQ20++;
            barcodeQ10++;
        } else if (barcode.second[i] >= q10) {
            barcodeQ10++;
        }
    }
    return true;
}

bool BarcodeProcessor::barcodeStatAndFilter(string &barcodeQ) {
    for (int i = 0; i < barcodeLen; i++) {
        if (barcodeQ[i] >= q30) {
            barcodeQ30++;
            barcodeQ20++;
            barcodeQ10++;
        } else if (barcodeQ[i] >= q20) {
            barcodeQ20++;
            barcodeQ10++;
        } else if (barcodeQ[i] >= q10) {
            barcodeQ10++;
        }
    }
    return true;
}

bool BarcodeProcessor::umiStatAndFilter(pair <string, string> &umi) {
    int q10BaseCount = 0;
    for (int i = 0; i < mOptions->transBarcodeToPos.umiLen; i++) {
        if (umi.second[i] >= q30) {
            umiQ30++;
            umiQ20++;
            umiQ10++;
        } else if (umi.second[i] >= q20) {
            umiQ20++;
            umiQ10++;
        } else if (umi.second[i] >= q10) {
            umiQ10++;
        } else {
            q10BaseCount++;
        }
    }
    if (umi.first.find("N") != string::npos) {
        umiNFilterReads++;
        return false;
    } else if (seqEncode(umi.first.c_str(), 0, mOptions->transBarcodeToPos.umiLen) == 0) {
        umiPloyAFilterReads++;
        return false;
    } else if (q10BaseCount > 1) {
        umiQ10FilterReads++;
        return false;
    } else {
        return true;
    }
}

void BarcodeProcessor::dumpDNBmap(string &dnbMapFile) {
    ofstream writer;
    if (ends_with(dnbMapFile, ".bin")) {
        mDNB.reserve(mDNB.size());
        writer.open(dnbMapFile, ios::out | ios::binary);
        boost::archive::binary_oarchive oa(writer);
        std::unordered_map<uint64, int> mDNB_stl;
        robin_hood::unordered_map<uint64, int>::iterator mapIter;
        for (mapIter = mDNB.begin();mapIter!=mDNB.end();mapIter++){
            mDNB_stl[mapIter->first] = mapIter->second;
        }
        oa << mDNB_stl;
    } else {
        writer.open(dnbMapFile);
        robin_hood::unordered_map<uint64, int>::iterator mapIter = mDNB.begin();
        while (mapIter != mDNB.end()) {
            uint32 x = mapIter->first >> 32;
            uint32 y = mapIter->first & 0x00000000FFFFFFFF;
            writer << x << "\t" << y << "\t" << mapIter->second << endl;
            mapIter++;
        }
    }
    writer.close();
}
