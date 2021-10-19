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
        int nows = mSizes.size();
        if (mRingBufferTags[mOutputCounter] == 1)
            mSizes.push_back({mRingBufferSizes[mOutputCounter], nows});
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

//        MPI_Send(&(tmpSize), 1, MPI_INT, 0, 0, communicator);
//        printf("processor 1 send size %d\n", tmpSize);
//        usleep(100);

//        loginfo("processor " + to_string(mOptions->myRank) + " send size " + to_string(tmpSize));
//        cout << "processor 1 send size " << tmpSize << endl;

        MPI_Send(&(tmpSize), 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
//        usleep(100);

//        printf("processor 1 send data %d\n", tmpSize);
//        loginfo("processor " + to_string(mOptions->myRank) + " send data " + to_string(tmpSize));

//        cout << "processor 1 send data " << tmpSize << endl;

//                MPI_Send(mRingBuffer[mOutputCounter], tmpSize, MPI_CHAR, 0, 0, communicator);
        MPI_Send(mRingBuffer[mOutputCounter], tmpSize, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
//        usleep(100);

        delete mRingBuffer[mOutputCounter];
        mRingBuffer[mOutputCounter] = NULL;
        mOutputCounter++;
        //cout << "Writer thread: " <<  mFilename <<  " mOutputCounter: " << mOutputCounter << " mInputCounter: " << mInputCounter << endl;
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
        int nowSS = min(10, int(size));
//        printf("print before pigz %d from tag %d\n", nowSS, tag);
//
//        for (int i = 0; i < nowSS; i++) {
//            printf("%c", pos[i]);
//        }
//        printf("\n");
//        fflush(stdout);
//        for (int i = nowSS - 1; i >= 0; i--) {
//            printf("%c", pos[size - i - 1]);
//        }
//        printf("\n");
//        fflush(stdout);

        while (Q->try_enqueue({tag, {mRingBuffer[mOutputCounter], mRingBufferSizes[mOutputCounter]}}) == 0) {
            printf("waiting to push a chunk to pigz queue\n");
            usleep(100);
        }
        wSum += mRingBufferSizes[mOutputCounter];
        int nows = mSizes.size();
        if (mRingBufferTags[mOutputCounter] == 1)
            mSizes.push_back({mRingBufferSizes[mOutputCounter], nows});
        cSum++;
//        printf("push a chunk to pigz queue, queue size %d\n", Q->size_approx());
        mRingBuffer[mOutputCounter] = NULL;
        mOutputCounter++;
        //cout << "Writer thread: " <<  mFilename <<  " mOutputCounter: " << mOutputCounter << " mInputCounter: " << mInputCounter << endl;
    }
}


void WriterThread::inputFromMerge(char *data, size_t size) {
    mtx.lock();
    mRingBuffer[mInputCounter] = data;
    mRingBufferSizes[mInputCounter] = size;
    mRingBufferTags[mInputCounter] = 2;
//    int pos = mInputCounter;
//    printf("input2 a data, size is %zu, pos is %d\n", size, pos);
//    fflush(stdout);
//    loginfo("processor " + to_string(mOptions->myRank) + " mInputCounter " + to_string(mInputCounter) + " " +
//            to_string(size));
    mInputCounter++;
    mtx.unlock();
}

void WriterThread::input(char *data, size_t size) {
    mtx.lock();
    mRingBuffer[mInputCounter] = data;
    mRingBufferSizes[mInputCounter] = size;
    mRingBufferTags[mInputCounter] = 1;
//    int pos = mInputCounter;
//    printf("input1 a data, size is %zu, pos is %d\n", size, pos);
//    fflush(stdout);
//    printf("mInputCounter %ld\n", mInputCounter.load());
//    loginfo("processor " + to_string(mOptions->myRank) + " mInputCounter " + to_string(mInputCounter) + " " +
//            to_string(size));
    mInputCounter++;
//    cout << "mInputCounter " << mInputCounter << endl;

//    if (mInputCounter >= PACK_NUM_LIMIT) {
//        printf("gg0\n");
//        exit(0);
//    }
    mtx.unlock();
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
