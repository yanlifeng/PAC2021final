#include "chipMaskHDF5.h"
#include <sys/time.h>
#include <omp.h>

double HD5GetTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double) tv.tv_sec + (double) tv.tv_usec / 1000000;
}

ChipMaskHDF5::ChipMaskHDF5(std::string FileName) {
    fileName = FileName;
}

ChipMaskHDF5::~ChipMaskHDF5() {

}

void ChipMaskHDF5::creatFile() {
    fileID = H5Fcreate(fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    //cout << "create hdf5 file successfully." << endl;
}

void ChipMaskHDF5::openFile() {
    fileID = H5Fopen(fileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    //int round = 3;
    //while (fileID< 0 && round>0){
    //    usleep(1000);
    //    fileID = H5Fopen(fileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    //    round-- ;
    //}  
}

herr_t ChipMaskHDF5::writeDataSet(std::string chipID, slideRange &sliderange, unordered_map<uint64, Position1> &bpMap,
                                  uint32_t barcodeLen, uint8_t segment, uint32_t slidePitch, uint compressionLevel,
                                  int index) {
    //generate dataSet space
    hid_t dataspaceID;
    hsize_t dims[RANK];
    dims[0] = sliderange.rowEnd - sliderange.rowStart + 1;
    dims[1] = sliderange.colEnd - sliderange.colStart + 1;
    dims[2] = segment;
    dataspaceID = H5Screate_simple(RANK, dims, NULL);
    hsize_t memDims[1] = {dims[0] * dims[1] * dims[2]};
    hid_t memdataspaceID = H5Screate_simple(1, memDims, NULL);
    //transfer bpMap to bpMatrix
    bpMatrix = new uint64 **[dims[0]];
    uint64 *bpMatrix_buffer = new uint64[dims[0] * dims[1] * dims[2]]();
    for (int i = 0; i < dims[0]; i++) {
        bpMatrix[i] = new uint64 *[dims[1]];
        for (int j = 0; j < dims[1]; j++) {
            bpMatrix[i][j] = bpMatrix_buffer + i * dims[1] * dims[2] + j * dims[2];
        }
    }
    uint64 *barcodes;
    for (auto mapIter = bpMap.begin(); mapIter != bpMap.end(); mapIter++) {
        int row = mapIter->second.y - sliderange.rowStart;
        int col = mapIter->second.x - sliderange.colStart;
        barcodes = bpMatrix[row][col];
        for (int i = 0; i < segment; i++) {
            if (barcodes[i] == 0) {
                barcodes[i] = mapIter->first;
                //cout << mapIter->second.y-sliderange.rowStart << " : " << mapIter->second.x-sliderange.colStart << " : " << mapIter->first << endl;
                break;
            }
        }
        //cout << mapIter->second.y-sliderange.rowStart << " : " << mapIter->second.x-sliderange.colStart << " : " << mapIter->first << endl;
    }

    //define dataSet chunk, compression need chunking data
    hid_t plistID;
    //create property list of dataSet generation
    plistID = H5Pcreate(H5P_DATASET_CREATE);

    hsize_t cdims[RANK] = {CDIM0, CDIM1, segment};
    herr_t status;
    status = H5Pset_chunk(plistID, RANK, cdims);
    status = H5Pset_deflate(plistID, compressionLevel);

    //create dataset
    hid_t datasetID;
    std::string datasetName = DATASETNAME + std::to_string(index);
    datasetID = H5Dcreate2(fileID, datasetName.c_str(), H5T_NATIVE_UINT64, dataspaceID, H5P_DEFAULT, plistID,
                           H5P_DEFAULT);

    //create dataset attribute [rowShift, colShift, barcodeLength, slidePitch]
    uint32 attributeValues[ATTRIBUTEDIM] = {sliderange.rowStart, sliderange.colStart, barcodeLen, slidePitch};
    //create attribute to store dnb infomation
    hsize_t dimsA[1] = {ATTRIBUTEDIM};
    hid_t attributeSpace = H5Screate_simple(1, dimsA, NULL);
    hid_t attributeID = H5Acreate(datasetID, ATTRIBUTENAME, H5T_NATIVE_UINT32, attributeSpace, H5P_DEFAULT,
                                  H5P_DEFAULT);

    //create attribute to sotre chip id
    hid_t attributeSpace1 = H5Screate(H5S_SCALAR);
    hid_t attributeID1 = H5Acreate(datasetID, ATTRIBUTENAME1, H5T_NATIVE_CHAR, attributeSpace1, H5P_DEFAULT,
                                   H5P_DEFAULT);

    //write attribute
    status = H5Awrite(attributeID, H5T_NATIVE_INT, &attributeValues[0]);
    status = H5Awrite(attributeID1, H5T_NATIVE_CHAR, chipID.c_str());

    //close attribute
    status = H5Sclose(attributeSpace);
    status = H5Sclose(attributeSpace1);
    status = H5Aclose(attributeID);
    status = H5Aclose(attributeID1);

    //write matrix data to hdf5 file dataset
    status = H5Dwrite(datasetID, H5T_NATIVE_UINT64, memdataspaceID, dataspaceID, H5P_DEFAULT, &bpMatrix_buffer[0]);
    status = H5Sclose(dataspaceID);
    status = H5Dclose(datasetID);
    status = H5Pclose(plistID);
    status = H5Fclose(fileID);

    delete[] bpMatrix;
    delete[] bpMatrix_buffer;
    return status;
}

void ChipMaskHDF5::readDataSet(unordered_map<uint64, Position1> &bpMap, int index) {
    herr_t status;
    //open dataset with datasetName
    std::string datasetName = DATASETNAME + std::to_string(index);
    auto t0 = HD5GetTime();
    hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);
    printf("open cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();

    //read attribute of the dataset

    //uint32 attributeValues[ATTRIBUTEDIM];
    //hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
    //status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
    //uint32 rowOffset = attributeValues[0];
    //uint32 colOffset = attributeValues[1];
    //cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

    hid_t dspaceID = H5Dget_space(datasetID);
    printf("H5Dget_space cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();
    hid_t dtype_id = H5Dget_type(datasetID);
    printf("H5Dget_type cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();
    hid_t plistID = H5Dget_create_plist(datasetID);
    printf("H5Dget_create_plist cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();
    int rank = H5Sget_simple_extent_ndims(dspaceID);
    printf("H5Sget_simple_extent_ndims cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();
    hsize_t dims[rank];
    status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);
    printf("H5Sget_simple_extent_dims cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();
    uint64 matrixLen = 1;
    for (int i = 0; i < rank; i++) {
        matrixLen *= dims[i];
    }

    int segment = 1;
    if (rank >= 3) {
        segment = dims[2];
    }

    uint64 *bpMatrix_buffer = new uint64[matrixLen]();

    printf("new buffer cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();
    //cerr << "read bpMatrix finished..." <<endl;

    /*
    bpMatrix = new uint64**[dims[0]];
    for (int i=0; i<dims[0]; i++){
        bpMatrix[i] = new uint64*[dims[1]];
        for (int j = 0; j < dims[1]; j++){
            bpMatrix[i][j] = bpMatrix_buffer + i*dims[1]*segment + j*segment;
        }
    }
    */

    status = H5Dread(datasetID, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, bpMatrix_buffer);
    //status = H5Aclose(attributeID);

    printf("H5Dread cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();
    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);
    printf("close cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();

    printf("dim size %d * %d * %d\n", dims[0], dims[1], dims[2]);
    printf("rank %d\n", rank);
    printf("matrixLen %lld\n", matrixLen);

//#pragma omp parallel for num_threads(64)
    for (uint32 r = 0; r < dims[0]; r++) {
        //bpMatrix[r] = new uint64*[dims[1]];
        for (uint32 c = 0; c < dims[1]; c++) {
            //bpMatrix[r][c] = bpMatrix_buffer + r*dims[1]*dims[2] + c*dims[2];
            Position1 position = {c, r};
            if (rank >= 3) {
                segment = dims[2];
                for (int s = 0; s < segment; s++) {
                    uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
                    if (barcodeInt == 0) {
                        continue;
                    }
                    bpMap[barcodeInt] = position;
                }
            } else {
                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
                if (barcodeInt == 0) {
                    continue;
                }
                bpMap[barcodeInt] = position;
            }
        }
    }

    printf("for  cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();

    /*
    for (int r = 0; r<dims[0]; r++){
        for (int c = 0; c<dims[1]; c++){
            for (int s = 0; s<dims[2]; s++){
                uint64 barcodeInt = bpMatrix[r][c][s];
                if (barcodeInt == 0){
                    continue;
                }
                Position1 position = {c, r};
                bpMap[barcodeInt] = position;
            }
        }
    }
    */

    delete[] bpMatrix_buffer;
    //delete[] bpMatrix;
}