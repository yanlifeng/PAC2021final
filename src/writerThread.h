#ifndef WRITER_THREAD_H
#define WRITER_THREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>
#include "writer.h"
#include <atomic>
#include <mutex>
#include "atomicops.h"
#include "readerwriterqueue.h"
#include "util.h"
#include "options.h"

using namespace std;

class WriterThread {
public:
    WriterThread(string filename, int compressionLevel = 4);

    WriterThread(string filename, Options *options, int compressionLevel = 4);

    ~WriterThread();

    void initWriter(string filename1);

    void initWriter(ofstream *stream);

    void initWriter(gzFile gzfile);

    void cleanup();

    bool isCompleted();

    void output();

    void output(MPI_Comm communicator);

    void output(moodycamel::ReaderWriterQueue<pair<int, pair<char *, int>>> *Q);

    void input(char *data, size_t size);

    void inputFromMerge(char *data, size_t size);


    bool setInputCompleted();

    long bufferLength();

    string getFilename() { return mFilename; }

private:
    void deleteWriter();

private:
    Writer *mWriter1;
    int compression;
    string mFilename;
    Options *mOptions;


    //for split output
    bool mInputCompleted;
    atomic_long mInputCounter;
    atomic_long mOutputCounter;
    char **mRingBuffer;
    size_t *mRingBufferSizes;
    int *mRingBufferTags;
public:
    const atomic_long &GetMInputCounter() const;

    const atomic_long &GetMOutputCounter() const;

private:

    int cSum;
public:
    int GetCSum() const;


private:

    long long wSum;
public:
    long long GetWSum() const;

private:

    mutex mtx;
};

#endif