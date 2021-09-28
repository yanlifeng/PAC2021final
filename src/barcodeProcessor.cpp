#include "barcodeProcessor.h"


BarcodeProcessor::BarcodeProcessor(Options *opt, unordered_map<uint64, Position1> *mbpmap) {
    mOptions = opt;
    bpmap = mbpmap;
    mismatch = opt->transBarcodeToPos.mismatch;
    barcodeLen = opt->barcodeLen;
    polyTInt = seqEncode(polyT.c_str(), 0, barcodeLen, mOptions->rc);
    misMaskGenerate();
}

BarcodeProcessor::BarcodeProcessor(Options *opt, int mhashNum, int *mhashHead, node *mhashMap) {
    mOptions = opt;

    hashNum = mhashNum;
    hashHead = mhashHead;
    hashMap = mhashMap;
//    printf("now new BarcodeProcessor , hashNum is %d\n", hashNum);
//    printf("test5 val is %d\n", hashHead[109547259]);


    mismatch = opt->transBarcodeToPos.mismatch;
    barcodeLen = opt->barcodeLen;
    polyTInt = seqEncode(polyT.c_str(), 0, barcodeLen, mOptions->rc);
    misMaskGenerate();
}

BarcodeProcessor::BarcodeProcessor(Options *opt, int mhashNum, int *mhashHead, node *mhashMap,
                                   BloomFilter *mBloomFilter) {
    mOptions = opt;

    hashNum = mhashNum;
    hashHead = mhashHead;
    hashMap = mhashMap;
    bloomFilter = mBloomFilter;
//    printf("now new BarcodeProcessor , hashNum is %d\n", hashNum);
//    printf("test5 val is %d\n", hashHead[109547259]);


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
        //
        barcode = read1->mSeq.mStr.substr(mOptions->barcodeStart, mOptions->barcodeLen);
        barcodeQ = read1->mQuality.substr(mOptions->barcodeStart, mOptions->barcodeLen);
    } else if (mOptions->transBarcodeToPos.barcodeRead == 2) {
        barcode = read2->mSeq.mStr.substr(mOptions->barcodeStart, mOptions->barcodeLen);
        barcodeQ = read2->mQuality.substr(mOptions->barcodeStart, mOptions->barcodeLen);
    } else {
        error_exit("barcodeRead must be 1 or 2 . please check the --barcodeRead option you give");
    }
    barcodeStatAndFilter(barcodeQ);


    int position = getPosition(barcode);


    if (position != -1) {
        mMapToSlideRead++;
        bool umiPassFilter = true;
        //def umi start 25, umi len 10, umi read 1
        if (mOptions->transBarcodeToPos.umiStart >= 0 && mOptions->transBarcodeToPos.umiLen > 0) {
            pair<string, string> umi;
            if (mOptions->transBarcodeToPos.umiRead == 1) {
                //
                getUMI(read1, umi);
            } else {
                getUMI(read2, umi, true);
            }
            umiPassFilter = umiStatAndFilter(umi);
            if (!mOptions->transBarcodeToPos.PEout) {
                //
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
            addDNB(encodePosition(position / mOptions->dims1Size, position % mOptions->dims1Size));

        return umiPassFilter;
    }
    return false;

}

void BarcodeProcessor::addPositionToName(Read *r, int position, pair<string, string> *umi) {
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

void BarcodeProcessor::addPositionToNames(Read *r1, Read *r2, int position, pair<string, string> *umi) {
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

void BarcodeProcessor::getUMI(Read *r, pair<string, string> &umi, bool isRead2) {
    string umiSeq = r->mSeq.mStr.substr(mOptions->transBarcodeToPos.umiStart, mOptions->transBarcodeToPos.umiLen);
    string umiQ = r->mQuality.substr(mOptions->transBarcodeToPos.umiStart, mOptions->transBarcodeToPos.umiLen);
    umi.first = umiSeq;
    umi.second = umiQ;
    if (isRead2) {
        r->mSeq.mStr = r->mSeq.mStr.substr(0, mOptions->transBarcodeToPos.umiStart);
        r->mQuality = r->mQuality.substr(0, mOptions->transBarcodeToPos.umiStart);
    }
}


void BarcodeProcessor::decodePosition(const uint32 codePos, pair<uint16, uint16> &decodePos) {
    decodePos.first = codePos >> 16;
    decodePos.second = codePos & 0x0000FFFF;
}

void BarcodeProcessor::decodePosition(const uint64 codePos, pair<uint32, uint32> &decodePos) {
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
//    return bpmap->size();
    return hashNum;
}


int BarcodeProcessor::getPosition(uint64 barcodeInt) {


    auto iter = queryMap(barcodeInt);
    if (iter.first == 1) {
        overlapReads++;
        return (iter.second);
    } else if (mismatch > 0) {
        iter = getMisOverlap(barcodeInt);
        if (iter.first) {
            overlapReadsWithMis++;
            return iter.second;
        } else {
            return -1;
        }
    }
    return -1;
}

pair<int, int> BarcodeProcessor::queryMap(uint64 barcodeInt) {
    int ok = 0;
    int p = -1;
//    printf("bloom query %lld\n", barcodeInt);

    if (bloomFilter->Get(barcodeInt) == 0)return {ok, p};

    int key = barcodeInt % mod;
//    int key = mol(barcodeInt);
//    if (key >= mod)key -= mod;

//    if (key != barcodeInt % mod) {
//        printf("%lld %d %lld\n", barcodeInt, key, barcodeInt % mod);
//        exit(0);
//    }

    for (int i = hashHead[key]; i != -1; i = hashMap[i].pre) {
        if (hashMap[i].v == barcodeInt) {
            p = hashMap[i].p;
            ok = 1;
            break;
        }
    }
    return {ok, p};
}

int BarcodeProcessor::getPosition(string &barcodeString) {
    int Nindex = getNindex(barcodeString);
    if (Nindex == -1) {
        uint64 barcodeInt = seqEncode(barcodeString.c_str(), 0, barcodeLen);
        if (barcodeInt == polyTInt) {
            return -1;
        }
        return getPosition(barcodeInt);
    } else if (Nindex == -2) {
        return -1;
    } else if (mismatch > 0) {
        return getNOverlap(barcodeString, Nindex);
    }
    return -1;
}

void BarcodeProcessor::misMaskGenerate() {
    misMaskLen = possibleMis(barcodeLen, mismatch);
    misMaskLens = new int[mismatch];
    for (int i = 0; i < mismatch; i++) {
        misMaskLens[i] = possibleMis(barcodeLen, i + 1);
    }

    misMask = (uint64 *) malloc(misMaskLen * sizeof(uint64));
    set<uint64> misMaskSet;
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

string BarcodeProcessor::positionToString(int position) {
    stringstream positionString;
    positionString << position / mOptions->dims1Size << "_" << position % mOptions->dims1Size;
    return positionString.str();
}

string BarcodeProcessor::positionToString(Position *position) {
    stringstream positionString;
    positionString << position->x << "_" << position->y;
    return positionString.str();
}

//unordered_map<uint64, Position1>::iterator BarcodeProcessor::getMisOverlap(uint64 barcodeInt) {
//    uint64 misBarcodeInt;
//    int misCount = 0;
//    int misMaskIndex = 0;
//    unordered_map<uint64, Position1>::iterator iter;
//    unordered_map<uint64, Position1>::iterator overlapIter;
//
//    for (int mis = 0; mis < mismatch; mis++) {
//        misCount = 0;
//        while (misMaskIndex < misMaskLens[mis]) {
//            misBarcodeInt = barcodeInt ^ misMask[misMaskIndex];
//            misMaskIndex++;
//            iter = bpmap->find(misBarcodeInt);
//            if (iter != bpmap->end()) {
//                overlapIter = iter;
//                misCount++;
//                if (misCount > 1) {
//                    return bpmap->end();
//                }
//            }
//        }
//        if (misCount == 1) {
//            return overlapIter;
//        }
//    }
//    return bpmap->end();
//}



pair<int, int> BarcodeProcessor::getMisOverlap(uint64 barcodeInt) {
    uint64 misBarcodeInt;
    int misCount = 0;
    int misMaskIndex = 0;
    pair<int, int> iter;
    pair<int, int> overlapIter;

    for (int mis = 0; mis < mismatch; mis++) {
        misCount = 0;
        while (misMaskIndex < misMaskLens[mis]) {
            misBarcodeInt = barcodeInt ^ misMask[misMaskIndex];
            misMaskIndex++;

            iter = queryMap(misBarcodeInt);
            if (iter.first) {
                overlapIter = iter;
                misCount++;
                if (misCount > 1) {
                    return {0, -1};
                }
            }
        }
        if (misCount == 1) {
            return overlapIter;
        }
    }
    return {0, -1};
}


int BarcodeProcessor::getNOverlap(string &barcodeString, uint8 Nindex) {
    //N has the same encode (11) with G
    int misCount = 0;
    uint64 barcodeInt = seqEncode(barcodeString.c_str(), 0, barcodeString.length());
    pair<int, int> iter;
    pair<int, int> overlapIter;
    iter = queryMap(barcodeInt);
    if (iter.first) {
        misCount++;
        overlapIter = iter;
    }
    for (uint64 j = 1; j < 4; j++) {
        uint64 misBarcodeInt = barcodeInt ^ (j << Nindex * 2);
        iter = queryMap(misBarcodeInt);
        if (iter.first) {
            misCount++;
            if (misCount > 1) {
                return -1;
            }
            overlapIter = iter;
        }
    }
    if (misCount == 1) {
        overlapReadsWithN++;
        return overlapIter.second;
    }
    return -1;
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

bool BarcodeProcessor::barcodeStatAndFilter(pair<string, string> &barcode) {
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

bool BarcodeProcessor::umiStatAndFilter(pair<string, string> &umi) {
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
    unordered_map<uint64, int> mDNB_tmp;
    for (auto it :mDNB) {
        mDNB_tmp[it.first] = it.second;
    }
    if (ends_with(dnbMapFile, ".bin")) {
        mDNB_tmp.reserve(mDNB_tmp.size());
        writer.open(dnbMapFile, ios::out | ios::binary);
        boost::archive::binary_oarchive oa(writer);
        oa << mDNB_tmp;
    } else {
        writer.open(dnbMapFile);
        unordered_map<uint64, int>::iterator mapIter = mDNB.begin();
        while (mapIter != mDNB.end()) {
            uint32 x = mapIter->first >> 32;
            uint32 y = mapIter->first & 0x00000000FFFFFFFF;
            writer << x << "\t" << y << "\t" << mapIter->second << endl;
            mapIter++;
        }
    }
    writer.close();
}
