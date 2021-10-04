#include "writerThread.h"

WriterThread::WriterThread(string filename, int compressionLevel) {
    compression = compressionLevel;

    mWriter1 = NULL;

    mInputCounter = 0;
    mOutputCounter = 0;
    mInputCompleted = false;
    mFilename = filename;

    mRingBuffer = new char *[PACK_NUM_LIMIT];
    memset(mRingBuffer, 0, sizeof(char *) * PACK_NUM_LIMIT);
    mRingBufferSizes = new size_t[PACK_NUM_LIMIT];
    memset(mRingBufferSizes, 0, sizeof(size_t) * PACK_NUM_LIMIT);
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
//        printf("processor 1 send data %ld\n", mRingBufferSizes[mOutputCounter]);

        long tmpSize = mRingBufferSizes[mOutputCounter];
        MPI_Send(&(tmpSize), 1, MPI_LONG_LONG, 0, 0, communicator);
        MPI_Send(mRingBuffer[mOutputCounter], tmpSize, MPI_CHAR, 0, 0, communicator);

        delete mRingBuffer[mOutputCounter];
        mRingBuffer[mOutputCounter] = NULL;
        mOutputCounter++;
        //cout << "Writer thread: " <<  mFilename <<  " mOutputCounter: " << mOutputCounter << " mInputCounter: " << mInputCounter << endl;
    }
}

void WriterThread::output(moodycamel::ReaderWriterQueue<pair<char *, int>> *Q) {
    if (mOutputCounter >= mInputCounter) {
        usleep(100);
    }
    while (mOutputCounter < mInputCounter) {
//        mWriter1->write(mRingBuffer[mOutputCounter], mRingBufferSizes[mOutputCounter]);
//        delete mRingBuffer[mOutputCounter];
        while (Q->try_enqueue({mRingBuffer[mOutputCounter], mRingBufferSizes[mOutputCounter]}) == 0) {
//            printf("waiting to push a chunk to pigz queue\n");
            usleep(100);
        }
//        printf("push a chunk to pigz queue, queue size %d\n", Q->size_approx());
        mRingBuffer[mOutputCounter] = NULL;
        mOutputCounter++;
        //cout << "Writer thread: " <<  mFilename <<  " mOutputCounter: " << mOutputCounter << " mInputCounter: " << mInputCounter << endl;
    }
}

void WriterThread::input(char *data, size_t size) {
    mRingBuffer[mInputCounter] = data;
    mRingBufferSizes[mInputCounter] = size;
    mInputCounter++;
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
