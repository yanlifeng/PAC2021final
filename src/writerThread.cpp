#include "writerThread.h"

WriterThread::WriterThread(string filename, int compressionLevel) {
    compression = compressionLevel;

    mWriter1 = NULL;

    mInputCounter = 0;
    mOutputCounter = 0;
    mInputCompleted = false;
    mFilename = filename;
    wSum = 0;
    cSum = 0;

    mRingBuffer = new char *[PACK_NUM_LIMIT];
    memset(mRingBuffer, 0, sizeof(char *) * PACK_NUM_LIMIT);
    mRingBufferSizes = new size_t[PACK_NUM_LIMIT];
    mRingBufferTags = new int[PACK_NUM_LIMIT];
    memset(mRingBufferSizes, 0, sizeof(size_t) * PACK_NUM_LIMIT);
    memset(mRingBufferTags, 0, sizeof(int) * PACK_NUM_LIMIT);
    initWriter(filename);
}

WriterThread::WriterThread(string filename, Options *options, int compressionLevel) {
    compression = compressionLevel;
    mOptions = options;

    mWriter1 = NULL;

    mInputCounter = 0;
    mOutputCounter = 0;
    mInputCompleted = false;
    mFilename = filename;
    wSum = 0;
    cSum = 0;

    mRingBuffer = new char *[PACK_NUM_LIMIT];
    memset(mRingBuffer, 0, sizeof(char *) * PACK_NUM_LIMIT);
    mRingBufferSizes = new size_t[PACK_NUM_LIMIT];
    mRingBufferTags = new int[PACK_NUM_LIMIT];
    memset(mRingBufferSizes, 0, sizeof(size_t) * PACK_NUM_LIMIT);
    memset(mRingBufferTags, 0, sizeof(int) * PACK_NUM_LIMIT);
    initWriter(filename);
}

WriterThread::~WriterThread() {
    cleanup();
    delete mRingBuffer;
}

bool WriterThread::isCompleted() {
    return mInputCompleted && (mOutputCounter == mInputCounter);
}

bool WriterThread::setInputCompleted() {
    mInputCompleted = true;
    return true;
}

void WriterThread::output() {
    if (mOutputCounter >= mInputCounter) {
        usleep(100);
    }
    while (mOutputCounter < mInputCounter) {
        mWriter1->write(mRingBuffer[mOutputCounter], mRingBufferSizes[mOutputCounter]);
        delete mRingBuffer[mOutputCounter];
        wSum += mRingBufferSizes[mOutputCounter];
        cSum++;
        mRingBuffer[mOutputCounter] = NULL;
        mOutputCounter++;
        //cout << "Writer thread: " <<  mFilename <<  " mOutputCounter: " << mOutputCounter << " mInputCounter: " << mInputCounter << endl;
    }
}

void WriterThread::output(MPI_Comm communicator) {
    if (mOutputCounter >= mInputCounter) {
        usleep(100);
    }
    while (mOutputCounter < mInputCounter) {

        int tmpSize = mRingBufferSizes[mOutputCounter];
        MPI_Send(&(tmpSize), 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(mRingBuffer[mOutputCounter], tmpSize, MPI_CHAR, 0, 1, MPI_COMM_WORLD);

        delete mRingBuffer[mOutputCounter];
        mRingBuffer[mOutputCounter] = NULL;
        mOutputCounter++;
    }
}

void WriterThread::output(moodycamel::ReaderWriterQueue<std::pair<int, std::pair<char *, int>>> *Q) {

    if (mOutputCounter >= mInputCounter) {
        usleep(100);
    }
    while (mOutputCounter < mInputCounter) {
//        mWriter1->write(mRingBuffer[mOutputCounter], mRingBufferSizes[mOutputCounter]);
//        delete mRingBuffer[mOutputCounter];

        auto pos = mRingBuffer[mOutputCounter];
        auto size = mRingBufferSizes[mOutputCounter];
        auto tag = mRingBufferTags[mOutputCounter];

        while (Q->try_enqueue({tag, {mRingBuffer[mOutputCounter], mRingBufferSizes[mOutputCounter]}}) == 0) {
            printf("waiting to push a chunk to pigz queue\n");
            usleep(100);
        }
        wSum += mRingBufferSizes[mOutputCounter];
        cSum++;
//        printf("push a chunk to pigz queue, queue size %d\n", Q->size_approx());
        mRingBuffer[mOutputCounter] = NULL;
        mOutputCounter++;
        //cout << "Writer thread: " <<  mFilename <<  " mOutputCounter: " << mOutputCounter << " mInputCounter: " << mInputCounter << endl;
    }
}


void WriterThread::inputFromMerge(char *data, size_t size) {
//    mtx.lock();
    mRingBuffer[mInputCounter] = data;
    mRingBufferSizes[mInputCounter] = size;
    mRingBufferTags[mInputCounter] = 2;
    mInputCounter++;
//    mtx.unlock();
}

void WriterThread::input(char *data, size_t size) {
//    mtx.lock();
    mRingBuffer[mInputCounter] = data;
    mRingBufferSizes[mInputCounter] = size;
    mRingBufferTags[mInputCounter] = 1;
    mInputCounter++;
//    mtx.unlock();
}

void WriterThread::cleanup() {
    deleteWriter();
}

void WriterThread::deleteWriter() {
    if (mWriter1 != NULL) {
        delete mWriter1;
        mWriter1 = NULL;
    }
}

void WriterThread::initWriter(string filename1) {
    deleteWriter();
    mWriter1 = new Writer(filename1, compression);
}

void WriterThread::initWriter(ofstream *stream) {
    deleteWriter();
    mWriter1 = new Writer(stream);
}

void WriterThread::initWriter(gzFile gzfile) {
    deleteWriter();
    mWriter1 = new Writer(gzfile);
}

long WriterThread::bufferLength() {
    return mInputCounter - mOutputCounter;
}

long long WriterThread::GetWSum() const {
    return wSum;
}

int WriterThread::GetCSum() const {
    return cSum;
}

const atomic_long &WriterThread::GetMInputCounter() const {
    return mInputCounter;
}

const atomic_long &WriterThread::GetMOutputCounter() const {
    return mOutputCounter;
}
