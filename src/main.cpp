#include <stdio.h>
#include "fastqreader.h"
#include <time.h>
#include "cmdline.h"
#include <sstream>
#include "util.h"
#include "options.h"
#include "barcodeToPositionMulti.h"
#include "barcodeToPositionMultiPE.h"
#include "barcodeListMerge.h"
#include "chipMaskFormatChange.h"
#include "chipMaskMerge.h"
#include <mutex>

#include <sys/time.h>

double MainGetTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double) tv.tv_sec + (double) tv.tv_usec / 1000000;
}


string command;
mutex logmtx;

int main(int argc, char *argv[]) {
    if (argc == 1) {
        cerr << "spatial_transcriptome: an spatial transcriptome data processor" << endl;
    }
    int my_rank, num_procs;
    int proc_len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Get_processor_name(processor_name, &proc_len);
    printf("Process %d of %d ,processor name is %s\n", my_rank, num_procs, processor_name);


    cmdline::parser cmd;
    // input/output
    cmd.add<string>("in", 'i', "mask file of stereomics chip or input barcode_list file", "");
    cmd.add<string>("in1", 'I', "the second sequencing fastq file path of read1", false, "");
    cmd.add<string>("in2", 0, "the second sequencing fastq file path of read2", false, "");
    cmd.add<string>("barcodeReadsCount", 0, "the mapped barcode list file with reads count per barcode.", false, "");
    cmd.add<string>("out", 'O', "output file prefix or fastq output file of read1", true, "");
    cmd.add<string>("out2", 0, "fastq output file of read2", false, "");
    cmd.add<string>("report", 0, "logging file path.", false, "");
    cmd.add("PEout", 0, "if this option was given, PE reads with barcode tag will be writen");
    cmd.add<int>("compression", 'z',
                 "compression level for gzip output (1 ~ 9). 1 is fastest, 9 is smallest, default is 4.", false, 4);
    cmd.add<string>("unmappedOut", 0,
                    "output file path for barcode unmapped reads of read1, if this path isn't given, discard the reads.",
                    false, "");
    cmd.add<string>("unmappedOut2", 0,
                    "output file path for barcode unmapped reads of read2, if this path isn't given, discard the reads.",
                    false, "");
    cmd.add<uint32_t>("barcodeLen", 'l', "barcode length, default is 25", false, 25);
    cmd.add<int>("barcodeStart", 0, "barcode start position", false, 0);
    cmd.add<int>("umiRead", 0, "read1 or read2 contains the umi sequence.", false, 1);
    cmd.add<int>("barcodeRead", 0, "read1 or read2 contains the barcode sequence.", false, 1);
    cmd.add<int>("umiStart", 0,
                 "umi start position. if the start postion is negative number, no umi sequence will be found", false,
                 25);
    cmd.add<int>("umiLen", 0, "umi length.", false, 10);
    cmd.add<string>("fixedSequence", 0, "fixed sequence in read1 that will be filtered.", false, "");
    cmd.add<int>("fixedStart", 0, "fixed sequence start position can by specied.", false, -1);
    cmd.add<int>("barcodeSegment", 0, "barcode segment for every position on the stereo-chip.", false, 1);
    cmd.add<string>("fixedSequenceFile", 0,
                    "file contianing the fixed sequences and the start position, one sequence per line in the format: TGCCTCTCAG\t-1. when position less than 0, means wouldn't specified",
                    false, "");
    cmd.add<long>("mapSize", 0, "bucket size of the new unordered_map.", false, 0);
    cmd.add<int>("mismatch", 0, "max mismatch is allowed for barcode overlap find.", false, 0);
    cmd.add<int>("action", 0,
                 "chose one action you want to run [map_barcode_to_slide = 1, merge_barcode_list = 2, mask_format_change = 3, mask_merge = 4].",
                 false, 1);
    cmd.add<int>("thread", 'w', "number of thread that will be used to run.", false, 2);
    cmd.add<int>("pugzThread", 0, "number of thread that will be used to pugz.", false, 1);
    cmd.add<int>("pigzThread", 0, "number of thread that will be used to pigz.", false, 1);
    cmd.add("verbose", 'V', "output verbose log information (i.e. when every 1M reads are processed).");
    cmd.add("usePugz", 0, "use pugz to decompress\n");
    cmd.add("usePigz", 0, "use pigz to decompress\n");
//    cmd.add<int>("numaId", 0, "for test", false, 3);
    cmd.add("outGzSpilt", 0, "");

    cmd.parse_check(argc, argv);

    if (argc == 1) {
        cerr << cmd.usage() << endl;
        return 0;
    }


    Options opt;
    opt.in = cmd.get<string>("in");
    opt.out = cmd.get<string>("out");
    opt.compression = cmd.get<int>("compression");
    opt.barcodeLen = cmd.get<uint32_t>("barcodeLen");
    opt.barcodeStart = cmd.get<int>("barcodeStart");
    opt.mapSize = cmd.get<long>("mapSize");
    opt.actionInt = cmd.get<int>("action");
    opt.verbose = cmd.exist("verbose");
    opt.usePugz = cmd.exist("usePugz");
    opt.usePigz = cmd.exist("usePigz");
    opt.thread = cmd.get<int>("thread");
    opt.pugzThread = cmd.get<int>("pugzThread");
    opt.pigzThread = cmd.get<int>("pigzThread");
    opt.report = cmd.get<string>("report");
    opt.barcodeSegment = cmd.get<int>("barcodeSegment");
    opt.transBarcodeToPos.in = cmd.get<string>("in");
    opt.transBarcodeToPos.in1 = cmd.get<string>("in1");
    opt.transBarcodeToPos.in2 = cmd.get<string>("in2");
    opt.transBarcodeToPos.out1 = cmd.get<string>("out");
    opt.transBarcodeToPos.out2 = cmd.get<string>("out2");
    opt.transBarcodeToPos.mismatch = cmd.get<int>("mismatch");
    opt.transBarcodeToPos.unmappedOutFile = cmd.get<string>("unmappedOut");
    opt.transBarcodeToPos.unmappedOutFile2 = cmd.get<string>("unmappedOut2");
    opt.transBarcodeToPos.umiRead = cmd.get<int>("umiRead");
    opt.transBarcodeToPos.umiStart = cmd.get<int>("umiStart");
    opt.transBarcodeToPos.umiLen = cmd.get<int>("umiLen");
    opt.transBarcodeToPos.barcodeRead = cmd.get<int>("barcodeRead");
    opt.transBarcodeToPos.mappedDNBOutFile = cmd.get<string>("barcodeReadsCount");
    opt.transBarcodeToPos.fixedSequence = cmd.get<string>("fixedSequence");
    opt.transBarcodeToPos.fixedStart = cmd.get<int>("fixedStart");
    opt.transBarcodeToPos.fixedSequenceFile = cmd.get<string>("fixedSequenceFile");
    opt.transBarcodeToPos.PEout = cmd.exist("PEout");
//    opt.numaId = cmd.get<int>("numaId");
    opt.outGzSpilt = cmd.exist("outGzSpilt");

    printf("outGzSpilt is %d\n", opt.outGzSpilt);

    opt.myRank = my_rank;
    opt.numPro = num_procs;
    opt.communicator = MPI_COMM_WORLD;

//    if (num_procs > 2) {
//        printf("mpirun -n can't > 2\n");
//        exit(0);
//    }
//    opt.numaId = my_rank;

    if (num_procs >= 2) {
        string out_name = opt.out;
        int pos = out_name.find(".fq");
        if (pos < 0 || pos > out_name.size()) {
            printf("gg out has no .fq\n");
            exit(0);
        }
        string sifx = out_name.substr(pos, out_name.size());
        opt.out = out_name.substr(0, pos) + to_string(my_rank) + sifx;
        opt.transBarcodeToPos.out1 = out_name.substr(0, pos) + to_string(my_rank) + sifx;
    } else if (num_procs == 1) {
//        opt.numaId = -1;
    }

//    opt.numaId = 3;
//    opt.myRank = 0;

//    printf("numa id is %d\n", opt.numaId);
    printf("pro num is %d\n", opt.numPro);

    if (opt.usePugz) {
        printf("now use pugz, %d threads\n", opt.pugzThread);
    }
    if (opt.usePigz) {

        if (opt.pigzThread == 1) {
            printf("pigz thread number must >1\n");
            exit(0);
        }
        printf("now use pigz, %d threads\n", opt.pigzThread);
        string out_name = opt.transBarcodeToPos.out1;
        printf("ori out name is %s\n", out_name.c_str());
        printf(".gz pos is %lu\n", out_name.find(".gz"));
        opt.transBarcodeToPos.out1 = out_name.substr(0, out_name.find(".gz"));
        opt.out = opt.transBarcodeToPos.out1;
    }
    printf("now out name is %s\n", opt.transBarcodeToPos.out1.c_str());

    if (ends_with(opt.transBarcodeToPos.in1, ".gz") == 0) {
        opt.usePugz = 0;
    }

    stringstream ss;
    for (int i = 0; i < argc; i++) {
        ss << argv[i] << " ";
    }
    command = ss.str();
    time_t t1 = time(NULL);
    auto t_t0 = MainGetTime();
    opt.init();
    opt.validate();

//    cout << "action : " << opt.actionInt << endl;

    if (opt.actionInt == 1) {
        if (opt.transBarcodeToPos.PEout) {
            cout << "has pe" << endl;
            auto t_t0 = MainGetTime();
            BarcodeToPositionMultiPE barcodeToPosMultiPE(&opt);
            cout << "new cost " << MainGetTime() - t_t0 << endl;
            t_t0 = MainGetTime();
            barcodeToPosMultiPE.process();
            cout << "process cost " << MainGetTime() - t_t0 << endl;
        } else {


//            cout << "no pe" << endl;
            auto t_t0 = MainGetTime();
            BarcodeToPositionMulti barcodeToPosMulti(&opt);
//            cout << "new cost " << MainGetTime() - t_t0 << endl;
            t_t0 = MainGetTime();
            barcodeToPosMulti.process();
            cout << opt.myRank << " work done" << endl;
            if (opt.myRank == 0)
                cout << "process cost " << MainGetTime() - t_t0 << endl;


        }
    } else if (opt.actionInt == 2) {
        BarcodeListMerge barcodeListMerge(&opt);
        barcodeListMerge.mergeBarcodeLists();
    } else if (opt.actionInt == 3) {
        ChipMaskFormatChange chipMaskFormatChange(&opt);
        chipMaskFormatChange.change();
    } else if (opt.actionInt == 4) {
        ChipMaskMerge chipMaskMerge(&opt);
        chipMaskMerge.maskMerge();
    } else {
        cerr << endl << "wrong action has been choosed." << endl;
    }
    if (opt.myRank == 0)
        cout << "my time : " << MainGetTime() - t_t0 << endl;
    time_t t2 = time(NULL);
    if (opt.myRank == 0) {
        cerr << endl << command << endl;
        cerr << "spatialRNADrawMap" << ", time used: " << (t2 - t1) << " seconds" << endl;
    }
    MPI_Finalize();

    return 0;
}
