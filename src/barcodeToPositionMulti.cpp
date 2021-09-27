#include "barcodeToPositionMulti.h"
#include "mytime.h"



BarcodeToPositionMulti::BarcodeToPositionMulti(Options *opt) {
    mOptions = opt;
    mProduceFinished = false;
    mFinishedThreads = 0;
    mOutStream = NULL;
    mZipFile = NULL;
    mWriter = NULL;
    mUnmappedWriter = NULL;
    bool isSeq500 = opt->isSeq500;
    mbpmap = new BarcodePositionMap(opt);
    //barcodeProcessor = new BarcodeProcessor(opt, &mbpmap->bpmap);
    if (!mOptions->transBarcodeToPos.fixedSequence.empty() || !mOptions->transBarcodeToPos.fixedSequenceFile.empty()) {
        fixedFilter = new FixedFilter(opt);
        filterFixedSequence = true;
    }
}

BarcodeToPositionMulti::~BarcodeToPositionMulti() {
    //if (fixedFilter) {
    //	delete fixedFilter;
    //}
    //unordered_map<uint64, Position*>().swap(misBarcodeMap);
}

bool BarcodeToPositionMulti::process() {
    initOutput();
    initPackRepositoey();
    // cout<<"producer start"<<endl;
    // clock_t t0=clock();
    // extern atomic<long> getPos_t;
    // getPos_t=0;
    timeb start,endp,endc,endw;
    ftime(&start);
    

    std::thread producer(std::bind(&BarcodeToPositionMulti::producerTask, this));

    Result **results = new Result *[mOptions->thread];
    BarcodeProcessor **barcodeProcessors = new BarcodeProcessor *[mOptions->thread];
    for (int t = 0; t < mOptions->thread; t++) {
        results[t] = new Result(mOptions, true);
        results[t]->setBarcodeProcessor(mbpmap->getBpmap());
    }

    std::thread **threads = new thread *[mOptions->thread];
    for (int t = 0; t < mOptions->thread; t++) {
        threads[t] = new std::thread(std::bind(&BarcodeToPositionMulti::consumerTask, this, results[t]));
    }

    std::thread *writerThread = NULL;
    std::thread *unMappedWriterThread = NULL;
    if (mWriter) {
        writerThread = new std::thread(std::bind(&BarcodeToPositionMulti::writeTask, this, mWriter));
    }
    if (mUnmappedWriter) {
        unMappedWriterThread = new std::thread(std::bind(&BarcodeToPositionMulti::writeTask, this, mUnmappedWriter));
    }

    producer.join();
    ftime(&endp);

    for (int t = 0; t < mOptions->thread; t++) {
        threads[t]->join();
    }
    ftime(&endc);
    double getPos_t=0;
    for(int i=0;i<mOptions->thread;i++)
        getPos_t+=results[i]->mBarcodeProcessor->tt;
    // double tt=0;
    // getPos_t.compare_exchange_strong(tt,tt);
    cout<<"getPosition:"<<getPos_t<<endl;

    if (writerThread)
        writerThread->join();
    if (unMappedWriterThread)
        unMappedWriterThread->join();
    ftime(&endw);
    printTime(start,endp,"producer finish");
    printTime(start,endc,"consumer finish");
    printTime(start,endw,"writer finish");

    if (mOptions->verbose)
        loginfo("start to generate reports\n");

    //merge result
    vector<Result *> resultList;
    for (int t = 0; t < mOptions->thread; t++) {
        resultList.push_back(results[t]);
    }
    Result *finalResult = Result::merge(resultList);
    finalResult->print();


    cout << resetiosflags(ios::fixed) << setprecision(2);
    if (!mOptions->transBarcodeToPos.mappedDNBOutFile.empty()) {
        cout << "mapped_dnbs: " << finalResult->mBarcodeProcessor->mDNB.size() << endl;
        finalResult->dumpDNBs(mOptions->transBarcodeToPos.mappedDNBOutFile);
    }

    //clean up
    for (int t = 0; t < mOptions->thread; t++) {
        delete threads[t];
        threads[t] = NULL;
        delete results[t];
        results[t] = NULL;
    }

    delete[] threads;
    delete[] results;

    if (writerThread)
        delete writerThread;
    if (unMappedWriterThread)
        delete unMappedWriterThread;

    closeOutput();

    return true;
}

void BarcodeToPositionMulti::initOutput() {
    mWriter = new WriterThread(mOptions->out, mOptions->compression);
    if (!mOptions->transBarcodeToPos.unmappedOutFile.empty()) {
        mUnmappedWriter = new WriterThread(mOptions->transBarcodeToPos.unmappedOutFile, mOptions->compression);
    }
}

void BarcodeToPositionMulti::closeOutput() {
    if (mWriter) {
        delete mWriter;
        mWriter = NULL;
    }
    if (mUnmappedWriter) {
        delete mUnmappedWriter;
        mUnmappedWriter = NULL;
    }
}

bool BarcodeToPositionMulti::processPairEnd(ReadPairPack *pack, Result *result) {
    string outstr;
    string unmappedOut;
    bool hasPosition;
    bool fixedFiltered;
    for (int p = 0; p < pack->count; p++) {
        result->mTotalRead++;
        ReadPair *pair = pack->data[p];
        Read *or1 = pair->mLeft;
        Read *or2 = pair->mRight;
        if (filterFixedSequence) {
            fixedFiltered = fixedFilter->filter(or1, or2, result);
            if (fixedFiltered) {
                delete pair;
                continue;
            }
        }
        hasPosition = result->mBarcodeProcessor->process(or1, or2);
        if (hasPosition) {
            outstr += or2->toString();
        } else if (mUnmappedWriter) {
            unmappedOut += or2->toString();
        }
        delete pair;
    }
    mOutputMtx.lock();
    if (mUnmappedWriter && !unmappedOut.empty()) {
        //write reads that can't be mapped to the slide
        char *udata = new char[unmappedOut.size()];
        memcpy(udata, unmappedOut.c_str(), unmappedOut.size());
        mUnmappedWriter->input(udata, unmappedOut.size());
    }
    if (mWriter && !outstr.empty()) {
        char *data = new char[outstr.size()];
        memcpy(data, outstr.c_str(), outstr.size());
        mWriter->input(data, outstr.size());
    }
    mOutputMtx.unlock();
//    delete pack->data;
    delete pack;
    return true;
}

void BarcodeToPositionMulti::initPackRepositoey() {
    mRepo.packBuffer = new ChunkPair *[PACK_NUM_LIMIT];
    memset(mRepo.packBuffer, 0, sizeof(ReadPairPack *) * PACK_NUM_LIMIT);
    mRepo.writePos = 0;
    mRepo.readPos = 0;
}

void BarcodeToPositionMulti::destroyPackRepository() {
    delete mRepo.packBuffer;
    mRepo.packBuffer = NULL;
}

void BarcodeToPositionMulti::producePack(ChunkPair *pack) {
    mRepo.packBuffer[mRepo.writePos] = pack;
    mRepo.writePos++;
}

void BarcodeToPositionMulti::consumePack(Result *result) {
    dsrc::fq::FastqDataChunk *chunk;
    ChunkPair *chunkpair;
    ReadPairPack *data = new ReadPairPack;
    ReadPack *leftPack = new ReadPack;
    ReadPack *rightPack = new ReadPack;
    mInputMutx.lock();
    while (mRepo.writePos <= mRepo.readPos) {
        usleep(1000);
        if (mProduceFinished) {
            mInputMutx.unlock();
            return;
        }
    }
    chunkpair = mRepo.packBuffer[mRepo.readPos];
    mRepo.readPos++;
    mInputMutx.unlock();
    leftPack->count = dsrc::fq::chunkFormat(chunkpair->leftpart, leftPack->data, true);
    rightPack->count = dsrc::fq::chunkFormat(chunkpair->rightpart, rightPack->data, true);


    data->count = leftPack->count < rightPack->count ? leftPack->count : rightPack->count;
    for (int i = 0; i < data->count; ++i) {
        data->data.push_back(new ReadPair(leftPack->data[i], rightPack->data[i]));
    }
    pairReader->fastqPool_left->Release(chunkpair->leftpart);
    pairReader->fastqPool_right->Release(chunkpair->rightpart);
    processPairEnd(data, result);
    delete leftPack;
    delete rightPack;
}



void BarcodeToPositionMulti::producerTask() {

    if (mOptions->verbose)
        loginfo("start to load data");
    long lastReported = 0;
    int slept = 0;
    long readNum = 0;
    bool splitSizeReEvaluated = false;
    pairReader = new FastqChunkReaderPair(mOptions->transBarcodeToPos.in1, mOptions->transBarcodeToPos.in2, true, 0, 0);

    ChunkPair *chunk_pair;
    while ((chunk_pair = pairReader->readNextChunkPair()) != NULL) {
        //cerr << (char*)chunk_pair->leftpart->data.Pointer();
        producePack(chunk_pair);
        while (mRepo.writePos - mRepo.readPos > PACK_IN_MEM_LIMIT) {
            slept++;
            usleep(100);
        }

    }


    mProduceFinished = true;
    if (mOptions->verbose)
        loginfo("all reads loaded, start to monitor thread status");
    //lock.unlock();

}


void BarcodeToPositionMulti::consumerTask(Result *result) {
    //std::cout << "in consumerTask " << std::endl;
    while (true) {
        while (mRepo.writePos <= mRepo.readPos) {
            if (mProduceFinished)
                break;
            usleep(1000);
        }
        //std::unique_lock<std::mutex> lock(mRepo.readCounterMtx);
        if (mProduceFinished && mRepo.writePos == mRepo.readPos) {
            mFinishedThreads++;
            if (mOptions->verbose) {
                string msg = "finished " + to_string(mFinishedThreads) + " threads. Data processing completed.";
                loginfo(msg);
            }
            //lock.unlock();
            break;
        }
        if (mProduceFinished) {
            if (mOptions->verbose) {
                string msg = "thread is processing the " + to_string(mRepo.readPos) + "/" + to_string(mRepo.writePos) +
                             " pack";
                loginfo(msg);
            }
            consumePack(result);
            //lock.unlock();
        } else {
            //lock.unlock();
            consumePack(result);
        }
    }

    if (mFinishedThreads == mOptions->thread) {
        if (mWriter)
            mWriter->setInputCompleted();
        if (mUnmappedWriter)
            mUnmappedWriter->setInputCompleted();
    }

    if (mOptions->verbose) {
        string msg = "finished one thread";
        loginfo(msg);
    }
}

//void BarcodeToPositionMulti::consumerTask(Result *result) {
//    while (true) {
//        while (mRepo.writePos <= mRepo.readPos) {
//            if (mProduceFinished)
//                break;
//            usleep(1000);
//        }
//        if (mProduceFinished && mRepo.writePos == mRepo.readPos) {
//            mFinishedThreads++;
//            if (mOptions->verbose) {
//                string msg = "finished " + to_string(mFinishedThreads) + " threads. Data processing completed.";
//                loginfo(msg);
//            }
//            break;
//        }
//        if (mProduceFinished) {
//            if (mOptions->verbose) {
//                string msg = "thread is processing the " + to_string(mRepo.readPos) + "/" + to_string(mRepo.writePos) +
//                             " pack";
//                loginfo(msg);
//            }
//            consumePack(result);
//        } else {
//            consumePack(result);
//        }
//    }
//
//    if (mFinishedThreads == mOptions->thread) {
//        if (mWriter)
//            mWriter->setInputCompleted();
//        if (mUnmappedWriter)
//            mUnmappedWriter->setInputCompleted();
//    }
//    if (mOptions->verbose) {
//        string msg = "finished one thread";
//        loginfo(msg);
//    }
//}

void BarcodeToPositionMulti::writeTask(WriterThread *config) {
    while (true) {
        //loginfo("writeTask running: " + config->getFilename());
        if (config->isCompleted()) {
            config->output();
            break;
        }
        config->output();
    }

    if (mOptions->verbose) {
        string msg = config->getFilename() + " writer finished";
        loginfo(msg);
    }
}
