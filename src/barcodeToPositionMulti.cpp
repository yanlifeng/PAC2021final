#include "barcodeToPositionMulti.h"

#include "../lib/gzip_decompress.hpp" //FIXME

#include "prog_util.h"
#include <fstream>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <utime.h>

double GetTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double) tv.tv_sec + (double) tv.tv_usec / 1000000;
}


double
PUGZGetTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double) tv.tv_sec + (double) tv.tv_usec / 1000000;
}

struct options {
    bool count_lines;
    unsigned nthreads;
};

static const tchar *const optstring = T(":hnlt:V");

static void
show_usage(FILE *fp) {
    fprintf(fp,
            "Usage: %" TS " [-l] [-t n] FILE...\n"
            "Decompress the specified FILEs.\n"
            "\n"
            "Options:\n"
            "  -l        count line instead of content to standard output\n"
            "  -t n      use n threads\n"
            "  -h        print this help\n"
            "  -V        show version and legal information\n",
            program_invocation_name);
}

static void
show_version(void) {
    printf("Pugz parallel gzip decompression program\n"
           "Copyright 2019 Maël Kerbiriou, Rayan Chikhi\n"
           "\n"
           "Based on:\n"
           "gzip compression program v"
    LIBDEFLATE_VERSION_STRING
    "\n"
    "Copyright 2016 Eric Biggers\n"
    "\n"
    "This program is free software which may be modified and/or redistributed\n"
    "under the terms of the MIT license.  There is NO WARRANTY, to the extent\n"
    "permitted by law.  See the COPYING file for details.\n");
}

static int
stat_file(struct file_stream *in, stat_t *stbuf, bool allow_hard_links) {
    if (tfstat(in->fd, stbuf) != 0) {
        msg("%" TS ": unable to stat file", in->name);
        return -1;
    }

    if (!S_ISREG(stbuf->st_mode) && !in->is_standard_stream) {
        msg("%" TS " is %s -- skipping", in->name, S_ISDIR(stbuf->st_mode) ? "a directory" : "not a regular file");
        return -2;
    }

    if (stbuf->st_nlink > 1 && !allow_hard_links) {
        msg("%" TS " has multiple hard links -- skipping "
            "(use -f to process anyway)",
            in->name);
        return -2;
    }

    return 0;
}

//static int
//decompress_file(const tchar *path, const struct options *options) {
//    double t0 = PUGZGetTime();
//    struct file_stream in;
//    stat_t stbuf;
//    int ret;
//    const byte *in_p;
//
//    ret = xopen_for_read(path, true, &in);
//    if (ret != 0) goto out_free_paths;
//
//    ret = stat_file(&in, &stbuf, true);
//    if (ret != 0) goto out_close_in;
//
//    /* TODO: need a streaming-friendly solution */
//    ret = map_file_contents(&in, size_t(stbuf.st_size));
//    if (ret != 0) goto out_close_in;
//
//    in_p = static_cast<const byte *>(in.mmap_mem);
//
//    cout << "pre cost " << PUGZGetTime() - t0 << endl;
//    t0 = PUGZGetTime();
//
//    if (options->count_lines) {
//        LineCounter line_counter{};
//        libdeflate_gzip_decompress(in_p, in.mmap_size, options->nthreads, line_counter, nullptr);
//    } else {
//
//        vector<pair<char *, int>> G;
//        int fd = open("pp1.fq", O_WRONLY | O_CREAT);
//        PRINT_DEBUG("fd is %d\n", fd);
//        if (fd == -1) {
//            PRINT_DEBUG("gg on create file\n");
//            return -1;
//        }
//        OutputConsumer output{};
//        output.fd = fd;
//        output.P = &G;
//        ConsumerSync sync{};
//        libdeflate_gzip_decompress(in_p, in.mmap_size, options->nthreads, output, &sync);
//
//        cout << "gunzip and push data to memory cost " << PUGZGetTime() - t0 << endl;
//        t0 = PUGZGetTime();
//        for (auto item : G) {
//            write(fd, item.first, item.second);
//        }
//        cout << "write data to disk cost " << PUGZGetTime() - t0 << endl;
//        t0 = PUGZGetTime();
//        for (auto item : G) {
//            delete[] item.first;
//        }
//
//        cout << "delete data cost " << PUGZGetTime() - t0 << endl;
//    }
//
//    ret = 0;
//
//    out_close_in:
//    xclose(&in);
//    out_free_paths:
//    return ret;
//}
//
//int
//tmain(int argc, tchar *argv[]) {
//    tchar *default_file_list[] = {nullptr};
//    struct options options;
//    int opt_char;
//    int i;
//    int ret;
//
//    _program_invocation_name = get_filename(argv[0]);
//
//    options.count_lines = false;
//    options.nthreads = 1;
//
//    while ((opt_char = tgetopt(argc, argv, optstring)) != -1) {
//        switch (opt_char) {
//            case 'l':
//                options.count_lines = true;
//                break;
//
//            case 'h':
//                show_usage(stdout);
//                return 0;
//            case 'n':
//                /*
//                 * -n means don't save or restore the original filename
//                 *  in the gzip header.  Currently this implementation
//                 *  already behaves this way by default, so accept the
//                 *  option as a no-op.
//                 */
//                break;
//
//            case 't':
//                options.nthreads = unsigned(atoi(toptarg));
//                fprintf(stderr, "using %d threads for decompression (experimental)\n", options.nthreads);
//                break;
//            case 'V':
//                show_version();
//                return 0;
//            default:
//                show_usage(stderr);
//                return 1;
//        }
//    }
//
//    argv += toptind;
//    argc -= toptind;
//
//    if (argc == 0) {
//        argv = default_file_list;
//        argc = sizeof(default_file_list) / sizeof(default_file_list);
//    } else {
//        for (i = 0; i < argc; i++)
//            if (argv[i][0] == '-' && argv[i][1] == '\0') argv[i] = nullptr;
//    }
//
//    fprintf(stderr, "=======================\n");
//    ret = 0;
//    for (i = 0; i < argc; i++) {
//        fprintf(stderr, "%s\n", argv[i]);
//        ret |= -decompress_file(argv[i], &options);
//    }
//    fprintf(stderr, "=======================\n");
//
//    /*
//     * If ret=0, there were no warnings or errors.  Exit with status 0.
//     * If ret=2, there was at least one warning.  Exit with status 2.
//     * Else, there was at least one error.  Exit with status 1.
//     */
//    if (ret != 0 && ret != 2) ret = 1;
//
//    return ret;
//}


void BarcodeToPositionMulti::getMbpmap() {
    mbpmap = new BarcodePositionMap(mOptions);

}


BarcodeToPositionMulti::BarcodeToPositionMulti(Options *opt) {
    mOptions = opt;
    mProduceFinished = false;
    mFinishedThreads = 0;
    mOutStream = NULL;
    mZipFile = NULL;
    mWriter = NULL;
    mUnmappedWriter = NULL;
    bool isSeq500 = opt->isSeq500;
//    mbpmap = new BarcodePositionMap(opt);
//    printf("test4 val is %d\n", mbpmap->GetHashHead()[109547259]);

    //barcodeProcessor = new BarcodeProcessor(opt, &mbpmap->bpmap);
    if (!mOptions->transBarcodeToPos.fixedSequence.empty() || !mOptions->transBarcodeToPos.fixedSequenceFile.empty()) {
        fixedFilter = new FixedFilter(opt);
        filterFixedSequence = true;
    }
    if (mOptions->usePugz) {
        pugzQueue1 = new moodycamel::ReaderWriterQueue<pair<char *, int>>(1 << 20);
        pugzQueue2 = new moodycamel::ReaderWriterQueue<pair<char *, int>>(1 << 20);
    }
    pugz1Done = 0;
    pugz2Done = 0;
    producerDone = 0;
}

BarcodeToPositionMulti::~BarcodeToPositionMulti() {
    //if (fixedFilter) {
    //	delete fixedFilter;
    //}
    //unordered_map<uint64, Position*>().swap(misBarcodeMap);
}

bool BarcodeToPositionMulti::process() {

//    thread pugzer1(bind(&BarcodeToPositionMulti::pugzTask1, this));
//    thread pugzer2(bind(&BarcodeToPositionMulti::pugzTask2, this));
//    pugzer1.join();
//    pugzer2.join();


    auto t00 = GetTime();

    initOutput();
    initPackRepositoey();
    thread *pugzer1;
    thread *pugzer2;
    auto getMbp = new thread(bind(&BarcodeToPositionMulti::getMbpmap, this));
//    getMbp->join();



    if (mOptions->usePugz) {
        pugzer1 = new thread(bind(&BarcodeToPositionMulti::pugzTask1, this));
        pugzer2 = new thread(bind(&BarcodeToPositionMulti::pugzTask2, this));
    }


    thread producer(bind(&BarcodeToPositionMulti::producerTask, this));

    printf("wait get map thread done...\n");
    getMbp->join();
    printf("get map thread done\n");

    printf("get map cost %.4f\n", GetTime() - t00);

    Result **results = new Result *[mOptions->thread];
    BarcodeProcessor **barcodeProcessors = new BarcodeProcessor *[mOptions->thread];
    for (int t = 0; t < mOptions->thread; t++) {
        results[t] = new Result(mOptions, true);
//        results[t]->setBarcodeProcessor(mbpmap->getBpmap());
        results[t]->setBarcodeProcessor(mbpmap->GetHashNum(), mbpmap->GetHashHead(), mbpmap->GetHashMap());
    }


    printf("consumer thread begin...\n");


    printf("new result done\n");

    thread **threads = new thread *[mOptions->thread];
    for (int t = 0; t < mOptions->thread; t++) {
        threads[t] = new thread(bind(&BarcodeToPositionMulti::consumerTask, this, results[t]));
    }

    thread *writerThread = NULL;
    thread *unMappedWriterThread = NULL;
    if (mWriter) {
        writerThread = new thread(bind(&BarcodeToPositionMulti::writeTask, this, mWriter));
    }
    if (mUnmappedWriter) {
        unMappedWriterThread = new thread(bind(&BarcodeToPositionMulti::writeTask, this, mUnmappedWriter));
    }

    producer.join();

    cout << "producer done" << endl;
    if (mOptions->usePugz) {
        pugzer1->join();
        cout << "pugzer1 done" << endl;
        pugzer2->join();
        cout << "pugzer2 done" << endl;
    }

    for (int t = 0; t < mOptions->thread; t++) {
        threads[t]->join();
    }
    cout << "consumer done" << endl;
    printf("consumer cost %.4f\n", GetTime() - t00);

    if (writerThread)
        writerThread->join();
    if (unMappedWriterThread)
        unMappedWriterThread->join();

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
    printf("final and delete cost %.4f\n", GetTime() - t00);

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
    double t = GetTime();
    leftPack->count = dsrc::fq::chunkFormat(chunkpair->leftpart, leftPack->data, true);
    rightPack->count = dsrc::fq::chunkFormat(chunkpair->rightpart, rightPack->data, true);


    data->count = leftPack->count < rightPack->count ? leftPack->count : rightPack->count;
    for (int i = 0; i < data->count; ++i) {
        data->data.push_back(new ReadPair(leftPack->data[i], rightPack->data[i]));
    }
    result->costFormat += GetTime() - t;
    pairReader->fastqPool_left->Release(chunkpair->leftpart);
    pairReader->fastqPool_right->Release(chunkpair->rightpart);
    processPairEnd(data, result);
    delete leftPack;
    delete rightPack;
}


//void BarcodeToPositionMulti::consumePack(Result *result) {
//    ReadPairPack *data;
//    mInputMutx.lock();
//    while (mRepo.writePos <= mRepo.readPos) {
//        usleep(1000);
//        if (mProduceFinished) {
//            mInputMutx.unlock();
//            return;
//        }
//    }
//    data = mRepo.packBuffer[mRepo.readPos];
//    mRepo.readPos++;
//    //if (mOptions->verbose)
//    //	loginfo("readPos: " + to_string(mRepo.readPos) + "\n");
//    mInputMutx.unlock();
//    processPairEnd(data, result);
//}

//void BarcodeToPositionMulti::producerTask() {
//    if (mOptions->verbose)
//        loginfo("start to load data");
//    long lastReported = 0;
//    int slept = 0;
//    long readNum = 0;
//    ReadPair **data = new ReadPair *[PACK_SIZE];
//    memset(data, 0, sizeof(ReadPair *) * PACK_SIZE);
//    FastqReaderPair reader(mOptions->transBarcodeToPos.in1, mOptions->transBarcodeToPos.in2, true);
//    int count = 0;
//    bool needToBreak = false;
//    while (true) {
//        ReadPair *read = reader.read();
//        if (!read || needToBreak) {
//            ReadPairPack *pack = new ReadPairPack;
//            pack->data = data;
//            pack->count = count;
//            producePack(pack);
//            data = NULL;
//            if (read) {
//                delete read;
//                read = NULL;
//            }
//            break;
//        }
//        data[count] = read;
//        count++;
//        if (mOptions->verbose && count + readNum >= lastReported + 1000000) {
//            lastReported = count + readNum;
//            string msg = "loaded " + to_string((lastReported / 1000000)) + "M read pairs";
//            loginfo(msg);
//        }
//        if (count == PACK_SIZE || needToBreak) {
//            ReadPairPack *pack = new ReadPairPack;
//            pack->data = data;
//            pack->count = count;
//            producePack(pack);
//            //re-initialize data for next pack
//            data = new ReadPair *[PACK_SIZE];
//            memset(data, 0, sizeof(ReadPair *) * PACK_SIZE);
//            // if the consumer is far behind this producer, sleep and wait to limit memory usage
//            while (mRepo.writePos - mRepo.readPos > PACK_IN_MEM_LIMIT) {
//                slept++;
//                usleep(100);
//            }
//            readNum += count;
//            // if the writer threads are far behind this producer, sleep and wait
//            // check this only when necessary
//            if (readNum % (PACK_SIZE * PACK_IN_MEM_LIMIT) == 0 && mWriter) {
//                while ((mWriter && mWriter->bufferLength() > PACK_IN_MEM_LIMIT)) {
//                    slept++;
//                    usleep(1000);
//                }
//            }
//            // reset count to 0
//            count = 0;
//        }
//    }
//    mProduceFinished = true;
//    if (mOptions->verbose) {
//        loginfo("all reads loaded, start to monitor thread status");
//    }
//    if (data != NULL)
//        delete[] data;
//}
void BarcodeToPositionMulti::pugzTask1() {
    printf("now use pugz to decompress, (%d threads)\n", mOptions->pugzThread);
    double t0 = GetTime();
    struct file_stream in;
    stat_t stbuf;
    int ret;
    const byte *in_p;

    ret = xopen_for_read(mOptions->transBarcodeToPos.in1.c_str(), true, &in);
    if (ret != 0) {
        printf("gg on xopen_for_read\n");
        exit(0);
    }

    ret = stat_file(&in, &stbuf, true);
    if (ret != 0) {
        printf("gg on stat_file\n");
        exit(0);
    }
    /* TODO: need a streaming-friendly solution */
    ret = map_file_contents(&in, size_t(stbuf.st_size));

    if (ret != 0) {
        printf("gg on map_file_contents\n");
        exit(0);
    }

    in_p = static_cast<const byte *>(in.mmap_mem);
//    std::vector<std::pair<char *, int>> G;
//    int fd = open("pp1.fq", O_WRONLY | O_CREAT);
//    printf("fd is %d\n", fd);
//    if (fd == -1) {
//        printf("gg on create file\n");
//        exit(0);
//    }
    OutputConsumer output{};
//    output.G = &G;
    output.P = pugzQueue1;
    output.num = 1;
    output.pDone = &producerDone;
    ConsumerSync sync{};
    libdeflate_gzip_decompress(in_p, in.mmap_size, mOptions->pugzThread, output, &sync);

    pugz1Done = 1;
    std::cout << "gunzip and push data to memory cost " << GetTime() - t0 << std::endl;
//    t0 = GetTime();
//    pair<char *, int> now;
//    while (pugzQueue1.try_dequeue(now)) {
//        write(fd, now.first, now.second);
//        delete[] now.first;
//    }
//    close(fd);
//    std::cout << "write data to disk cost " << GetTime() - t0 << std::endl;

}

void BarcodeToPositionMulti::pugzTask2() {
    double t0 = GetTime();
    struct file_stream in;
    stat_t stbuf;
    int ret;
    const byte *in_p;

    ret = xopen_for_read(mOptions->transBarcodeToPos.in2.c_str(), true, &in);
    if (ret != 0) {
        printf("gg on xopen_for_read\n");
        exit(0);
    }

    ret = stat_file(&in, &stbuf, true);
    if (ret != 0) {
        printf("gg on stat_file\n");
        exit(0);
    }
    /* TODO: need a streaming-friendly solution */
    ret = map_file_contents(&in, size_t(stbuf.st_size));

    if (ret != 0) {
        printf("gg on map_file_contents\n");
        exit(0);
    }

    in_p = static_cast<const byte *>(in.mmap_mem);
    //    std::vector<std::pair<char *, int>> G;
//    int fd = open("pp2.fq", O_WRONLY | O_CREAT);
//    printf("fd is %d\n", fd);
//    if (fd == -1) {
//        printf("gg on create file\n");
//        exit(0);
//    }
    OutputConsumer output{};
//    output.G = &G;
    output.P = pugzQueue2;
    output.num = 2;
    output.pDone = &producerDone;
    ConsumerSync sync{};
    libdeflate_gzip_decompress(in_p, in.mmap_size, mOptions->pugzThread, output, &sync);

    pugz2Done = 1;
    std::cout << "gunzip and push data to memory cost " << GetTime() - t0 << std::endl;
//    t0 = GetTime();
//    pair<char *, int> now;
//    while (pugzQueue2.try_dequeue(now)) {
//        write(fd, now.first, now.second);
//        delete[] now.first;
//    }
//    close(fd);
//    std::cout << "write data to disk cost " << GetTime() - t0 << std::endl;
}


void BarcodeToPositionMulti::producerTask() {
    double t0 = GetTime();
    if (mOptions->verbose)
        loginfo("start to load data");
    long lastReported = 0;
    int slept = 0;
    long readNum = 0;
    bool splitSizeReEvaluated = false;
    pairReader = new FastqChunkReaderPair(mOptions->transBarcodeToPos.in1, mOptions->transBarcodeToPos.in2, true, 0, 0);

    ChunkPair *chunk_pair;
    pair<char *, int> last1;
    pair<char *, int> last2;
    int cnt = 0;

    if (mOptions->usePugz) {
        last1.first = new char[1 << 20];
        last1.second = 0;
        last2.first = new char[1 << 20];
        last2.second = 0;

        while ((chunk_pair = pairReader->readNextChunkPair(pugzQueue1, pugzQueue2, pugz1Done, pugz2Done,
                                                           last1, last2)) != NULL) {
            //cerr << (char*)chunk_pair->leftpart->data.Pointer();
            if (mOptions->verbose)
                loginfo("producer read one chunk");
//            printf("read %d chunk done, size is %lld %lld\n", cnt++,
//                   chunk_pair->leftpart->size, chunk_pair->rightpart->size);
//            cout << "read " << (cnt++) << " chunk done, size is " << chunk_pair->leftpart->size << " "
//                 << chunk_pair->rightpart->size << endl;
            producePack(chunk_pair);
            while (mRepo.writePos - mRepo.readPos > PACK_IN_MEM_LIMIT) {
//                printf("producer wait consumer\n");
//                cout << "producer wait consumer" << endl;
                slept++;
                usleep(100);
            }

        }
    } else {
        while ((chunk_pair = pairReader->readNextChunkPair()) != NULL) {
            //cerr << (char*)chunk_pair->leftpart->data.Pointer();
            if (mOptions->verbose)
                loginfo("producer read one chunk");
//            printf("read %d chunk done, size is %lld %lld\n", cnt++,
//                   chunk_pair->leftpart->size, chunk_pair->rightpart->size);
            producePack(chunk_pair);
            while (mRepo.writePos - mRepo.readPos > PACK_IN_MEM_LIMIT) {
//                printf("producer waiting...\n");
                slept++;
                usleep(100);
            }

        }
    }


    mProduceFinished = true;
    if (mOptions->verbose)
        loginfo("all reads loaded, start to monitor thread status");
    //lock.unlock();

    printf("producer cost %.5f\n", GetTime() - t0);
    producerDone = 1;


}


void BarcodeToPositionMulti::consumerTask(Result *result) {
    //cout << "in consumerTask " << endl;
    while (true) {
        while (mRepo.writePos <= mRepo.readPos) {
            if (mProduceFinished)
                break;
//            printf("consumer waiting...\n");
            usleep(1000);
        }
        //unique_lock<mutex> lock(mRepo.readCounterMtx);
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
