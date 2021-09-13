#ifndef COMMON_H
#define COMMON_H

#define FASTP_VER "0.20.0"

#define _DEBUG false

#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <unordered_map>
#include "robin_hood.h"


#include <chrono>
typedef std::chrono::high_resolution_clock Clock;
#define TDEF(x_) chrono::high_resolution_clock::time_point x_##_t0, x_##_t1;
#define TSTART(x_) x_##_t0 = Clock::now();
#define TEND(x_) x_##_t1 = Clock::now();
#define TPRINT(x_, str) fprintf(stderr,"%-20s \t%.6f\t sec\n", str, chrono::duration_cast<chrono::microseconds>(x_##_t1 - x_##_t0).count()/1e6);
#define TINT(x_) chrono::duration_cast<chrono::microseconds>(x_##_t1 - x_##_t0).count()



typedef long long int int64;
typedef unsigned long long int uint64;

typedef int int32;
typedef unsigned int uint32;

typedef short int16;
typedef unsigned short uint16;

typedef char int8;
typedef unsigned char uint8;

const char ATCG_BASES[4] = {'A', 'C', 'T', 'G'};
const uint8 RC_BASE[4] = {2, 3, 0, 1};

static const int SEQ_BINARY_SIZE = 8;
static const int COUNT_BINARY_SUZE = 2;

static const long long int MAX_BARCODE = 0xffffffffffffffff;

#pragma pack(2)


#pragma pack()

// the limit of the queue to store the packs
// error may happen if it generates more packs than this number
static const int PACK_NUM_LIMIT = 10000000;

//buckets number for barcode unordered set
static const int BARCODE_SET_LIMIT = 100000000;

// how many reads one pack has
static const int PACK_SIZE = 1000;

// if one pack is produced, but not consumed, it will be kept in the memory
// this number limit the number of in memory packs
// if the number of in memory packs is full, the producer thread should sleep
static const int PACK_IN_MEM_LIMIT = 500;

// if read number is more than this, warn it
static const int WARN_STANDALONE_READ_LIMIT = 10000;

//block number per fov
static const int BLOCK_COL_NUM = 10;
static const int BLOCK_ROW_NUM = 10;
static const int TRACK_WIDTH = 3;
static const int FOV_GAP = 0;
static const int MAX_DNB_EXP = 250;

static const int EST_DNB_DISTANCE = 1;

//outside dnb idx reture value
static const int OUTSIDE_DNB_POS_ROW = 1410065408;
static const int OUTSIDE_DNB_POS_COL = 1410065408;

typedef struct slideRange {
    uint32 colStart;
    uint32 colEnd;
    uint32 rowStart;
    uint32 rowEnd;
} slideRange;






typedef struct Position {
    friend class boost::serialization::access;

    uint8 fov_c;
    uint8 fov_r;
    uint32 x;
    uint32 y;

    template<typename Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & fov_c;
        ar & fov_r;
        ar & x;
        ar & y;
    }
} Position;

typedef struct Position1 {
    friend class boost::serialization::access;

    uint32 x;
    uint32 y;

    template<typename Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & x;
        ar & y;
    }
} Position1;


typedef struct bpmap_segment_value{
    uint32 minvalue = -1;
    uint32 maxvalue = -1;
    robin_hood::unordered_map<uint32,Position1> segment;
}bpmap_segment_value;




/*
  This file is a part of DSRC software distributed under GNU GPL 2 licence.
  The homepage of the DSRC project is http://sun.aei.polsl.pl/dsrc

  Authors: Lucas Roguski and Sebastian Deorowicz

  Version: 2.00
*/


//#include "../include/dsrc/Globals.h"
#include "Globals.h"

#ifndef NDEBUG
#	define DEBUG 1
#endif

#define BIT(x)                            (1 << (x))
#define BIT_ISSET(x, pos)                ((x & BIT(pos)) != 0)
#define BIT_SET(x, pos)                    (x |= BIT(pos))
#define BIT_UNSET(x, pos)                (x &= ~(BIT(pos)))
#define MIN(x, y)                        ((x) <= (y) ? (x) : (y))
#define MAX(x, y)                        ((x) >= (y) ? (x) : (y))
#define ABS(x)                            ((x) >=  0  ? (x) : -(x))
#define SIGN(x)                            ((x) >=  0  ?  1  : -1)
#define REC_EXTENSION_FACTOR(size)        ( ((size) / 4 > 1024) ? ((size) / 4) : 1024 )
#define MEM_EXTENSION_FACTOR(size)        REC_EXTENSION_FACTOR(size)

#if defined (_WIN32)
#	define _CRT_SECURE_NO_WARNINGS
#	pragma warning(disable : 4996) // D_SCL_SECURE
#	pragma warning(disable : 4244) // conversion uint64 to uint32
#	pragma warning(disable : 4267)
#	pragma warning(disable : 4800) // conversion byte to bool
#endif

// TODO: refactor raw data structs to avoid using <string> as a member
#include <string>


#define COMPILE_TIME_ASSERT(COND, MSG) typedef char static_assertion_##MSG[(!!(COND))*2-1]
#define COMPILE_TIME_ASSERT1(X, L) COMPILE_TIME_ASSERT(X,static_assertion_at_line_##L)
#define COMPILE_TIME_ASSERT2(X, L) COMPILE_TIME_ASSERT1(X,L)
#define STATIC_ASSERT(X)    COMPILE_TIME_ASSERT2(X,__LINE__)


namespace dsrc {

    namespace fq {

// ********************************************************************************************
        struct FastqDatasetType {
            static const uint32 AutoQualityOffset = 0;
            static const uint32 DefaultQualityOffset = 33;

            uint32 qualityOffset;
            bool plusRepetition;
            bool colorSpace;


            FastqDatasetType()
                    : qualityOffset(AutoQualityOffset), plusRepetition(false), colorSpace(false) {}

            static FastqDatasetType Default() {
                FastqDatasetType ds;
                ds.qualityOffset = AutoQualityOffset;
                ds.plusRepetition = false;
                ds.colorSpace = false;
                return ds;
            }
        };

        struct StreamsInfo {
            enum StreamName {
                MetaStream = 0,
                TagStream,
                DnaStream,
                QualityStream,

                StreamCount = 4
            };

            uint64 sizes[4];

            StreamsInfo() {
                Clear();
            }

            void Clear() {
                std::fill(sizes, sizes + StreamCount, 0);
            }
        };

        struct FastqRecord;

    } // namespace fq


    namespace comp {

        struct CompressionSettings {
            static const uint32 MaxDnaOrder = 9;
            static const uint32 MaxQualityOrder = 6;
            static const uint32 DefaultDnaOrder = 0;
            static const uint32 DefaultQualityOrder = 0;
            static const uint32 DefaultTagPreserveFlags = 0;        // 0 -- keep all

            uint32 dnaOrder;
            uint32 qualityOrder;
            uint64 tagPreserveFlags;
            bool lossy;
            bool calculateCrc32;

            CompressionSettings()
                    : dnaOrder(0), qualityOrder(0), tagPreserveFlags(DefaultTagPreserveFlags), lossy(false),
                      calculateCrc32(false) {}

            static CompressionSettings Default() {
                CompressionSettings s;
                s.dnaOrder = DefaultDnaOrder;
                s.qualityOrder = DefaultQualityOrder;
                s.tagPreserveFlags = DefaultTagPreserveFlags;
                s.lossy = false;
                s.calculateCrc32 = false;
                return s;
            }
        };

        struct InputParameters {
            static const uint32 DefaultQualityOffset = fq::FastqDatasetType::AutoQualityOffset;
            static const uint32 DefaultDnaCompressionLevel = 0;
            static const uint32 DefaultQualityCompressionLevel = 0;
            static const uint32 DefaultProcessingThreadNum = 2;
            static const uint64 DefaultTagPreserveFlags = 0;
            static const uint32 DefaultFastqBufferSizeMB = 8;

            static const bool DefaultLossyCompressionMode = false;
            static const bool DefaultCalculateCrc32 = false;


            uint32 qualityOffset;
            uint32 dnaCompressionLevel;
            uint32 qualityCompressionLevel;
            uint32 threadNum;
            uint64 tagPreserveFlags;

            uint32 fastqBufferSizeMB;
            bool lossyCompression;
            bool calculateCrc32;
            bool useFastqStdIo;

            std::string inputFilename;
            std::string outputFilename;

            InputParameters()
                    : qualityOffset(DefaultQualityOffset), dnaCompressionLevel(DefaultDnaCompressionLevel),
                      qualityCompressionLevel(DefaultQualityCompressionLevel), threadNum(DefaultProcessingThreadNum),
                      tagPreserveFlags(DefaultTagPreserveFlags), fastqBufferSizeMB(DefaultFastqBufferSizeMB),
                      lossyCompression(DefaultLossyCompressionMode), calculateCrc32(DefaultCalculateCrc32),
                      useFastqStdIo(false) {}

            static InputParameters Default() {
                InputParameters args;
                return args;
            }
        };

        struct Field;

        struct DnaStats;
        struct QualityStats;

        class BlockCompressor;

        class HuffmanEncoder;

        struct DsrcDataChunk;

    } // namespace comp


    namespace core {

        class Buffer;

        class BitMemoryReader;

        class BitMemoryWriter;

        class ErrorHandler;

    } // namespace core

} // namespace dsrc


#endif /* COMMON_H */
