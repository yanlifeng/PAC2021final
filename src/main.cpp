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
#include <omp.h>
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
    cmd.add("verbose", 'V', "output verbose log information (i.e. when every 1M reads are processed).");

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
    opt.thread = cmd.get<int>("thread");
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
    /*
     *  action : 1
     */

    if (opt.actionInt == 1) {
        if (opt.transBarcodeToPos.PEout) {
            BarcodeToPositionMultiPE barcodeToPosMultiPE(&opt);
            barcodeToPosMultiPE.process();
        } else {
            /*
             * In this
             */
            auto t_t0 = MainGetTime();
            BarcodeToPositionMulti barcodeToPosMulti(&opt);
            /*
             *  stl:unordered_map              17756 sec
             *  robin_hood:unordered_map        8981 sec
             *  嵌套 MAP                         9000 sec
             */

            /*
             *    118394902 直接完全匹配       550
             *     21275225 mismatch==1      850
             *      4976251+51148304 mismatch==2 && dont map      7600
             *
             */
            cerr << "new cost " << MainGetTime() - t_t0 << endl;
            t_t0 = MainGetTime();
            barcodeToPosMulti.process();
            cerr << "process cost " << MainGetTime() - t_t0 << endl;
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

    cout << "my time : " << MainGetTime() - t_t0 << endl;
    time_t t2 = time(NULL);

    cerr << endl << command << endl;
    cerr << "spatialRNADrawMap" << ", time used: " << (t2 - t1) << " seconds" << endl;

    return 0;
}

//            /*
//             *  没计划好，根本跑不完，尬住了啊
//             */
//            const int MisNum = 30;
//            int **MisMatchNum = new int*[70];
//            for (int i=0;i<70;i++){
//                MisMatchNum[i] = new int[MisNum];
//            }
//            for (int num=0;num<70;num++){
//                for (int i=0;i<MisNum;i++){
//                    MisMatchNum[num][i] = 0;
//                }
//            }
//
//            unordered_map<uint64, Position1> *umap = barcodeToPosMulti.mbpmap->getBpmap();
//            uint64 *map_key = new uint64[umap->size()+1];
//            int index = 0;
//            unordered_map<uint64, Position1>::iterator x;
//            for (x = umap->begin(); x != umap->end(); ++x) {
//                map_key[index++] = x->first;
//            }
//#pragma omp parallel for num_threads(64)
//            for (int i=0;i<index;i++) {
//                int now_threads = omp_get_thread_num();
//                for (int j=i+1;j<index;j++) {
//                    uint64 a = map_key[i];
//                    uint64 b = map_key[j];
//                    int now_mis = 0;
//                    for (int k=0;k<32;k++){
//                        if ((a&3)!=(b&3)) now_mis++;
//                        a = a >> 2;
//                        b = b >> 2;
//                    }
//                    MisMatchNum[now_threads][now_mis]++;
//                }
//            }
//            for (int i=1;i<64;i++){
//                for (int j=0;j<MisNum;j++){
//                    MisMatchNum[0][j] += MisMatchNum[i][j];
//                }
//            }
//            for (int i=0;i<MisNum;i++){
//                printf("MisMatch %d Num is %d\n",i,MisMatchNum[0][i]/2);
//            }