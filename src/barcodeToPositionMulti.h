#ifndef BARCODETOPOSITIONMULTI_H
#define BARCODETOPOSITIONMULTI_H

#include <string>
#include <unordered_map>
//#include "robin_hood.h"
#include <atomic>
#include <thread>
#include <functional>
#include <sys/time.h>

#include "options.h"
#include "barcodePositionMap.h"
#include "barcodeProcessor.h"
#include "fixedfilter.h"
#include "writerThread.h"
#include "result.h"
#include "readerwriterqueue.h"
#include "atomicops.h"

using namespace std;

struct ReadPairPack {
    //ReadPair** data;
    vector<ReadPair *> data;
    int count;
};

struct ReadPack {
    //Read** data;
    vector<Read *> data;
    int count;
};

typedef struct ReadPairPack ReadPairPack;

typedef struct ReadPack ReadPack;


typedef struct ReadPairRepository {
    ChunkPair **packBuffer;
    atomic_long readPos;
    atomic_long writePos;
} ReadPairRepository;

class BarcodeToPositionMulti {
public:
    BarcodeToPositionMulti(Options *opt);

    ~BarcodeToPositionMulti();

    bool process();

private:
    void initOutput();

    void closeOutput();

    bool processPairEnd(ReadPairPack *pack, Result *result);

    void initPackRepositoey();

    void destroyPackRepository();

//    void producePack(ReadPairPack *pack);
    void producePack(ChunkPair *pack);


    void consumePack(Result *result);

    void pugzTask1();

    void pugzTask2();

    void producerTask();

    void consumerTask(Result *result);

    void writeTask(WriterThread *config);

public:
    Options *mOptions;
    BarcodePositionMap *mbpmap;
    FixedFilter *fixedFilter;
    moodycamel::ReaderWriterQueue<std::pair<char *, int>> pugzQueue1;
    moodycamel::ReaderWriterQueue<std::pair<char *, int>> pugzQueue2;
    std::atomic_int pugz1Done;
    std::atomic_int pugz2Done;

    //unordered_map<uint64, Position*> misBarcodeMap;

private:
    ReadPairRepository mRepo;
    atomic_bool mProduceFinished;
    atomic_int mFinishedThreads;
    std::mutex mOutputMtx;
    std::mutex mInputMutx;
    gzFile mZipFile;
    ofstream *mOutStream;
    WriterThread *mWriter;
    WriterThread *mUnmappedWriter;
    bool filterFixedSequence = false;


    FastqChunkReaderPair *pairReader;
    dsrc::fq::FastqDataPool *fastqPool;

};

#endif
