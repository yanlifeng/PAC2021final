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

herr_t ChipMaskHDF5::writeDataSet(std::string chipID, slideRange &sliderange, robin_hood::unordered_map<uint64, Position1> &bpMap,
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

void ChipMaskHDF5::readDataSet(robin_hood::unordered_map<uint64, Position1> &bpMap, int index) {
    herr_t status;
    //open dataset with datasetName
    std::string datasetName = DATASETNAME + std::to_string(index);
    auto t0 = HD5GetTime();
    hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);
//    printf("open cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();

    //read attribute of the dataset

    //uint32 attributeValues[ATTRIBUTEDIM];
    //hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
    //status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
    //uint32 rowOffset = attributeValues[0];
    //uint32 colOffset = attributeValues[1];
    //cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

    hid_t dspaceID = H5Dget_space(datasetID);
//    printf("H5Dget_space cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();
    hid_t dtype_id = H5Dget_type(datasetID);
//    printf("H5Dget_type cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();
    hid_t plistID = H5Dget_create_plist(datasetID);
//    printf("H5Dget_create_plist cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();
    int rank = H5Sget_simple_extent_ndims(dspaceID);
//    printf("H5Sget_simple_extent_ndims cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();
    hsize_t dims[rank];
    status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);
//    printf("H5Sget_simple_extent_dims cost %.3f\n", HD5GetTime() - t0);
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

//    printf("new buffer cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();
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

//    printf("H5Dread cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();
    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);
//    printf("close cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();

//    printf("dim size %d * %d * %d\n", dims[0], dims[1], dims[2]);
//    printf("rank %d\n", rank);
//    printf("matrixLen %lld\n", matrixLen);

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

//    printf("for  cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();

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



void ChipMaskHDF5::readDataSetSegment(robin_hood::unordered_map<uint32, bpmap_segment_value>& bpMapSegment, int index){
    herr_t status;
    //open dataset with datasetName
    std::string datasetName = DATASETNAME + std::to_string(index);
    hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);

    //read attribute of the dataset

    //uint32 attributeValues[ATTRIBUTEDIM];
    //hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
    //status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
    //uint32 rowOffset = attributeValues[0];
    //uint32 colOffset = attributeValues[1];
    //cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

    hid_t dspaceID = H5Dget_space(datasetID);

    hid_t dtype_id = H5Dget_type(datasetID);

    hid_t plistID = H5Dget_create_plist(datasetID);

    int rank = H5Sget_simple_extent_ndims(dspaceID);

    hsize_t dims[rank];
    status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);

    uint64 matrixLen = 1;
    for (int i = 0; i < rank; i++) {
        matrixLen *= dims[i];
    }

    int segment = 1;
    if (rank >= 3) {
        segment = dims[2];
    }

    uint64 *bpMatrix_buffer = new uint64[matrixLen]();

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

    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);


    for (uint32 r = 0; r < dims[0]; r++) {
        //bpMatrix[r] = new uint64*[dims[1]];
        for (uint32 c = 0; c < dims[1]; c++) {
            //bpMatrix[r][c] = bpMatrix_buffer + r*dims[1]*dims[2] + c*dims[2];
            Position1 position = {c, r};
            if (rank >= 3) {
                segment = dims[2];
                for (int s = 0; s < segment; s++) {
                    uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
                    if ((barcodeInt>>50)>0) {
                        printf("Map Error!\n");
                    }
                    if (barcodeInt == 0 || (barcodeInt>>50)>0 ) {
                        continue;
                    }
                    // 不处理barcode 长度大于25的情况
                    robin_hood::unordered_map<uint32, bpmap_segment_value>::iterator x1;
                    x1=bpMapSegment.find(getMapKey(barcodeInt));
                    if (x1 != bpMapSegment.end()){
                        x1->second.segment[getMapValue(barcodeInt)] = position;
                        x1->second.maxvalue = getMapValue(barcodeInt) > x1->second.maxvalue ? getMapValue(barcodeInt):x1->second.maxvalue;
                        x1->second.minvalue = getMapValue(barcodeInt) < x1->second.minvalue ? getMapValue(barcodeInt):x1->second.minvalue;
                    }else{
//                        <uint32,bpmap_segment_value>
                        bpmap_segment_value *map_temp = new bpmap_segment_value;
                        map_temp->segment[getMapValue(barcodeInt)] = position;
                        map_temp->maxvalue = getMapValue(barcodeInt);
                        map_temp->minvalue = getMapValue(barcodeInt);
                        bpMapSegment[getMapKey(barcodeInt)] = *map_temp;
                    }
//                    bpMap[barcodeInt] = position;
                }
            } else {
                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
                if ((barcodeInt>>50) > 0) {
                    printf("Map Error!\n");
                }
                if (barcodeInt == 0 || (barcodeInt>>50)>0) {
                    continue;
                }
                robin_hood::unordered_map<uint32, bpmap_segment_value>::iterator x1;
                x1=bpMapSegment.find(getMapKey(barcodeInt));
                if (x1 != bpMapSegment.end()){
                    x1->second.segment[getMapValue(barcodeInt)] = position;
                }else{
                    bpmap_segment_value *map_temp = new bpmap_segment_value;
                    map_temp->segment[getMapValue(barcodeInt)] = position;
                    bpMapSegment[getMapKey(barcodeInt)] = *map_temp;
                }
//                bpMap[barcodeInt] = position;
            }
        }
    }

    int MaxSize = 0;

    robin_hood::unordered_map<uint32, bpmap_segment_value>::iterator iter;
    cerr << "Map One size is "<< bpMapSegment.size() <<endl;
    for (iter=bpMapSegment.begin();iter!=bpMapSegment.end();iter++){
        cerr << "Map Second One Size is " << iter->second.segment.size() << endl;
        MaxSize = max(MaxSize,(int)iter->second.segment.size());
    }
    cerr << "Map Second Max Size is " << MaxSize << endl;


//    printf("for  cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();

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
void ChipMaskHDF5::readDataSetHash(robin_hood::unordered_map<uint64, Position1> **&bpmap_hash, int index){
    herr_t status;
    //open dataset with datasetName
    std::string datasetName = DATASETNAME + std::to_string(index);
    hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);

    //read attribute of the dataset

    //uint32 attributeValues[ATTRIBUTEDIM];
    //hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
    //status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
    //uint32 rowOffset = attributeValues[0];
    //uint32 colOffset = attributeValues[1];
    //cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

    hid_t dspaceID = H5Dget_space(datasetID);

    hid_t dtype_id = H5Dget_type(datasetID);

    hid_t plistID = H5Dget_create_plist(datasetID);

    int rank = H5Sget_simple_extent_ndims(dspaceID);

    hsize_t dims[rank];
    status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);

    uint64 matrixLen = 1;
    for (int i = 0; i < rank; i++) {
        matrixLen *= dims[i];
    }

    int segment = 1;
    if (rank >= 3) {
        segment = dims[2];
    }

    uint64 *bpMatrix_buffer = new uint64[matrixLen]();

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

    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);


    bpmap_hash = new robin_hood::unordered_map<uint64, Position1>*[HashTable];
    for (int i=0;i<HashTable;i++){
        bpmap_hash[i] = nullptr;
    }
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
                    // 不处理barcode 长度大于25的情况
//                    uint32 mapKey = getMapKey(barcodeInt)%HashTable;
                    uint32 mapKey = getMapKey(barcodeInt);
                    if (bpmap_hash[mapKey]!=nullptr){
                        bpmap_hash[mapKey]->insert({barcodeInt,position});
                    }else{
                        bpmap_hash[mapKey] = new robin_hood::unordered_map<uint64, Position1>();
                        bpmap_hash[mapKey]->insert({barcodeInt,position});
                    }
                }
            } else {
                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
                if (barcodeInt == 0) {
                    continue;
                }
//                uint32 mapKey = getMapKey(barcodeInt)%HashTable;
                uint32 mapKey = getMapKey(barcodeInt);
                if (bpmap_hash[mapKey]!=nullptr){
                    bpmap_hash[mapKey]->insert({barcodeInt,position});
                }else{
                    bpmap_hash[mapKey] = new robin_hood::unordered_map<uint64, Position1>();
                    bpmap_hash[mapKey]->insert({barcodeInt,position});
                }
//                bpMap[barcodeInt] = position;
            }
        }
    }

//    printf("for  cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();

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

void ChipMaskHDF5::readDataSetHashIndex(robin_hood::unordered_map<uint32, uint32> **&bpmap_hash_index,
                                        Position1 *&position_index, int index){
    herr_t status;
    //open dataset with datasetName
    std::string datasetName = DATASETNAME + std::to_string(index);
    hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);

    //read attribute of the dataset

    //uint32 attributeValues[ATTRIBUTEDIM];
    //hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
    //status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
    //uint32 rowOffset = attributeValues[0];
    //uint32 colOffset = attributeValues[1];
    //cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

    hid_t dspaceID = H5Dget_space(datasetID);

    hid_t dtype_id = H5Dget_type(datasetID);

    hid_t plistID = H5Dget_create_plist(datasetID);

    int rank = H5Sget_simple_extent_ndims(dspaceID);

    hsize_t dims[rank];
    status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);

    uint64 matrixLen = 1;
    for (int i = 0; i < rank; i++) {
        matrixLen *= dims[i];
    }

    int segment = 1;
    if (rank >= 3) {
        segment = dims[2];
    }

    uint64 *bpMatrix_buffer = new uint64[matrixLen]();

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

    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);



    bpmap_hash_index = new robin_hood::unordered_map<uint32, uint32>*[HashTable];
    position_index   = new Position1[dims[0]*dims[1]];
#pragma omp parallel for num_threads(64)
    for (int i=0;i<HashTable;i++){
        bpmap_hash_index[i] = nullptr;
//        bpmap_hash_index[i] = new robin_hood::unordered_map<uint32, uint32>();
    }

//    int indexPos = 0;
//    for (uint32 r = 0; r < dims[0]; r++) {
//        //bpMatrix[r] = new uint64*[dims[1]];
//        for (uint32 c = 0; c < dims[1]; c++) {
//            //bpMatrix[r][c] = bpMatrix_buffer + r*dims[1]*dims[2] + c*dims[2];
//            Position1 position = {c, r};
//
//            position_index[indexPos] = position;
//            if (rank >= 3) {
//                segment = dims[2];
//                for (int s = 0; s < segment; s++) {
//                    uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
//                    if (barcodeInt == 0) {
//                        continue;
//                    }
//                    // 不处理barcode 长度大于25的情况
////                    uint32 mapKey = getMapKey(barcodeInt)%HashTable;
//                    uint32 mapKey = getMapKey(barcodeInt);
//                    if (bpmap_hash_index[mapKey]!=nullptr){
//                        bpmap_hash_index[mapKey]->insert({getMapValue(barcodeInt),indexPos});
//                    }else{
//                        bpmap_hash_index[mapKey] = new robin_hood::unordered_map<uint32, uint32>();
//                        bpmap_hash_index[mapKey]->insert({getMapValue(barcodeInt),indexPos});
//                    }
//                }
//            }
//            else {
//                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
//                if (barcodeInt == 0) {
//                    continue;
//                }
////                uint32 mapKey = getMapKey(barcodeInt)%HashTable;
//                uint32 mapKey = getMapKey(barcodeInt);
//                if (bpmap_hash_index[mapKey]!=nullptr){
//                    bpmap_hash_index[mapKey]->insert({getMapValue(barcodeInt),indexPos});
//                }else{
//                    bpmap_hash_index[mapKey] = new robin_hood::unordered_map<uint32,uint32>();
//                    bpmap_hash_index[mapKey]->insert({getMapValue(barcodeInt),indexPos});
//                }
////                bpMap[barcodeInt] = position;
//            }
//            indexPos++;
//        }
//    }

/*
 *  多线程插入
 */
cerr << "dims[0] " << dims[0] << " dims[1] " << dims[1] << endl;
    atomic<int> indexPos(0);
    atomic<long long> waitNum(0);
    atomic<bool> *mapUse = new atomic<bool>[HashTable];
#pragma omp parallel for num_threads(64)
    for (int i=0;i<HashTable;i++){
        mapUse[i]=false;
    }
#pragma omp parallel for num_threads(24)
    for (uint32 r = 0; r < dims[0]; r++) {
        //bpMatrix[r] = new uint64*[dims[1]];
        for (uint32 c = 0; c < dims[1]; c++) {
            //bpMatrix[r][c] = bpMatrix_buffer + r*dims[1]*dims[2] + c*dims[2];
            Position1 position = {c, r};
            int thread_pos = indexPos++;
            position_index[thread_pos] = position;
            if (rank >= 3) {
                segment = dims[2];
                for (int s = 0; s < segment; s++) {
                    uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
                    if (barcodeInt == 0) {
                        continue;
                    }
                    // 不处理barcode 长度大于25的情况
//                    uint32 mapKey = getMapKey(barcodeInt)%HashTable;
                    uint32 mapKey = getMapKey(barcodeInt);
                    bool UseFalse = false;
                    bool UseTrue  = true;
                    while(!mapUse[mapKey].compare_exchange_weak(UseFalse,UseTrue)){
                        UseFalse = false;
                        waitNum++;
//                        usleep(1);
                        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
                    }
                    if (bpmap_hash_index[mapKey]!=nullptr){
                        bpmap_hash_index[mapKey]->insert({getMapValue(barcodeInt),thread_pos});
                    }else{
                        bpmap_hash_index[mapKey] = new robin_hood::unordered_map<uint32, uint32>();
                        bpmap_hash_index[mapKey]->insert({getMapValue(barcodeInt),thread_pos});
                    }
                    mapUse[mapKey]=false;
                }
            }
            else {
                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
                if (barcodeInt == 0) {
                    continue;
                }
//                uint32 mapKey = getMapKey(barcodeInt)%HashTable;
                uint32 mapKey = getMapKey(barcodeInt);
                bool UseFalse = false;
                bool UseTrue  = true;
                while(!mapUse[mapKey].compare_exchange_weak(UseFalse,UseTrue)){
                    UseFalse = false;
                    waitNum++;
//                    usleep(1);
                    std::this_thread::sleep_for(std::chrono::nanoseconds(1));
                }
                if (bpmap_hash_index[mapKey]!=nullptr){
                    bpmap_hash_index[mapKey]->insert({getMapValue(barcodeInt),thread_pos});
                }else{
                    bpmap_hash_index[mapKey] = new robin_hood::unordered_map<uint32,uint32>();
                    bpmap_hash_index[mapKey]->insert({getMapValue(barcodeInt),thread_pos});
                }
                mapUse[mapKey]=false;
//                bpMap[barcodeInt] = position;
            }
        }
    }
    long long num_wait = waitNum;
    printf("Wait Number is %lld\n",num_wait);
//    printf("for  cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();

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
void ChipMaskHDF5::readDataSetHashVector(vector<bpmap_vector_value> **&bpmap_hash_vector, Position1 *& position_index, int index){
    herr_t status;
    //open dataset with datasetName
    std::string datasetName = DATASETNAME + std::to_string(index);
    hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);

    //read attribute of the dataset

    //uint32 attributeValues[ATTRIBUTEDIM];
    //hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
    //status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
    //uint32 rowOffset = attributeValues[0];
    //uint32 colOffset = attributeValues[1];
    //cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

    hid_t dspaceID = H5Dget_space(datasetID);

    hid_t dtype_id = H5Dget_type(datasetID);

    hid_t plistID = H5Dget_create_plist(datasetID);

    int rank = H5Sget_simple_extent_ndims(dspaceID);

    hsize_t dims[rank];
    status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);

    uint64 matrixLen = 1;
    for (int i = 0; i < rank; i++) {
        matrixLen *= dims[i];
    }

    int segment = 1;
    if (rank >= 3) {
        segment = dims[2];
    }

    uint64 *bpMatrix_buffer = new uint64[matrixLen]();

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

    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);



    bpmap_hash_vector = new vector<bpmap_vector_value>*[HashTable];
    position_index   = new Position1[dims[0]*dims[1]];
#pragma omp parallel for num_threads(64)
    for (int i=0;i<HashTable;i++){
        bpmap_hash_vector[i] = new vector<bpmap_vector_value>();
    }

/*
 *  多线程插入
 */
//    cerr << "dims[0] " << dims[0] << " dims[1] " << dims[1] << endl;
    atomic<int> indexPos(0);
    atomic<long long> waitNum(0);
    atomic<bool> *mapUse = new atomic<bool>[HashTable];
#pragma omp parallel for num_threads(64)
    for (int i=0;i<HashTable;i++){
        mapUse[i]=false;
    }
#pragma omp parallel for num_threads(64)
    for (uint32 r = 0; r < dims[0]; r++) {
        //bpMatrix[r] = new uint64*[dims[1]];
        for (uint32 c = 0; c < dims[1]; c++) {
            //bpMatrix[r][c] = bpMatrix_buffer + r*dims[1]*dims[2] + c*dims[2];
            Position1 position = {c, r};
            int thread_pos = indexPos++;
            position_index[thread_pos] = position;
            if (rank >= 3) {
                segment = dims[2];
                for (int s = 0; s < segment; s++) {
                    uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
                    if (barcodeInt == 0) {
                        continue;
                    }
                    // 不处理barcode 长度大于25的情况
//                    uint32 mapKey = getMapKey(barcodeInt)%HashTable;
                    uint32 mapKey = getMapKey(barcodeInt);
                    bool UseFalse = false;
                    bool UseTrue  = true;
                    while(!mapUse[mapKey].compare_exchange_weak(UseFalse,UseTrue)){
                        UseFalse = false;
                        waitNum++;
//                        usleep(1);
                        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
                    }
                    bpmap_hash_vector[mapKey]->push_back((bpmap_vector_value){getMapValue(barcodeInt),thread_pos});
                    mapUse[mapKey]=false;
                }
            }
            else {
                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
                if (barcodeInt == 0) {
                    continue;
                }
//                uint32 mapKey = getMapKey(barcodeInt)%HashTable;
                uint32 mapKey = getMapKey(barcodeInt);
                bool UseFalse = false;
                bool UseTrue  = true;
                while(!mapUse[mapKey].compare_exchange_weak(UseFalse,UseTrue)){
                    UseFalse = false;
                    waitNum++;
//                    usleep(1);
                    std::this_thread::sleep_for(std::chrono::nanoseconds(1));
                }
                bpmap_hash_vector[mapKey]->push_back((bpmap_vector_value){getMapValue(barcodeInt),thread_pos});
                mapUse[mapKey]=false;
//                bpMap[barcodeInt] = position;
            }
        }
    }
    long long num_wait = waitNum;
    printf("Wait Number is %lld\n",num_wait);
//    printf("for  cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();

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
void ChipMaskHDF5::readDataSetHashList(int *&bpmap_head, int *&bpmap_nxt, uint64 *&bpmap_key,int *&bpmap_value,Position1 *&position_index, int index){
    herr_t status;
    //open dataset with datasetName
    std::string datasetName = DATASETNAME + std::to_string(index);
    hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);

    //read attribute of the dataset

    //uint32 attributeValues[ATTRIBUTEDIM];
    //hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
    //status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
    //uint32 rowOffset = attributeValues[0];
    //uint32 colOffset = attributeValues[1];
    //cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

    hid_t dspaceID = H5Dget_space(datasetID);

    hid_t dtype_id = H5Dget_type(datasetID);

    hid_t plistID = H5Dget_create_plist(datasetID);

    int rank = H5Sget_simple_extent_ndims(dspaceID);

    hsize_t dims[rank];
    status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);

    uint64 matrixLen = 1;
    for (int i = 0; i < rank; i++) {
        matrixLen *= dims[i];
    }

    int segment = 1;
    if (rank >= 3) {
        segment = dims[2];
    }

    uint64 *bpMatrix_buffer = new uint64[matrixLen]();

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

    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);



    uint32 bpmap_num = 0;
    bpmap_head = new int[MOD];
    bpmap_nxt = new int[dims[0]*dims[1]*dims[2]];
    bpmap_key = new uint64[dims[0]*dims[1]*dims[2]];
    bpmap_value = new int[dims[0]*dims[1]*dims[2]];
    position_index   = new Position1[dims[0]*dims[1]];
    //int *mapKeyNum = new int[MOD];
    //for (int i=0;i<MOD;i++){
    //    mapKeyNum[i] = 0;
    //}
//#pragma omp parallel for num_threads(64)
    for (int i=0;i<MOD;i++){
        bpmap_head[i] = -1;
    }

/*
 *  多线程插入
 */
    cerr << "dims[0] " << dims[0] << " dims[1] " << dims[1] << " dims[2] " << dims[2] << endl;
    int indexPos=0;
//#pragma omp parallel for num_threads(8)
    for (uint32 r = 0; r < dims[0]; r++) {
        //bpMatrix[r] = new uint64*[dims[1]];
        for (uint32 c = 0; c < dims[1]; c++) {
            //bpMatrix[r][c] = bpMatrix_buffer + r*dims[1]*dims[2] + c*dims[2];
            Position1 position = {c, r};
            int thread_pos = indexPos++;
            position_index[thread_pos] = position;
            if (rank >= 3) {
                segment = dims[2];
                for (int s = 0; s < segment; s++) {
                    uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
                    if (barcodeInt == 0) {
                        continue;
                    }
                    uint32 mapKey = barcodeInt%MOD;
                    //mapKeyNum[mapKey]++;
                    bpmap_key[bpmap_num] = barcodeInt;
                    bpmap_value[bpmap_num] = thread_pos;

                    bpmap_nxt[bpmap_num] = bpmap_head[mapKey];
                    bpmap_head[mapKey] = bpmap_num;

                    bpmap_num++;
//                    mapUse[mapKey]=false;
                }
            }
            else {
                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
                if (barcodeInt == 0) {
                    continue;
                }
                uint32 mapKey = barcodeInt%MOD;
                //mapKeyNum[mapKey]++;
                bpmap_key[bpmap_num] = barcodeInt;
                bpmap_value[bpmap_num] = thread_pos;
                bpmap_nxt[bpmap_num] = bpmap_head[mapKey];
                bpmap_head[mapKey] = bpmap_num;
                bpmap_num++;
            }
        }
    }
//    int *KeyNum = new int[MOD];
//    for (int i=0;i<MOD;i++){
//        KeyNum[i]=0;
//    }
//    long long now_time(0),target_time(0);
//    int min_map_size = MOD+1;
//    int max_map_size = 0;
//    for (int i=0;i<MOD;i++){
//        min_map_size = min(min_map_size,mapKeyNum[i]);
//        max_map_size = max(max_map_size,mapKeyNum[i]);
//        KeyNum[mapKeyNum[i]]++;
//    }
//    cerr << "Min Map Num is "<< min_map_size<<endl;
//    cerr << "Max Map Num is "<< max_map_size<<endl;
//    for (int i=0;i<max_map_size;i++){
//        cerr << " Map Num " << i << " is "<<KeyNum[i] << endl;
//        if (i<=1){
//            target_time += 6ll*KeyNum[i];
//            now_time    += 6ll*KeyNum[i];
//        }else{
//            target_time += (5ll+i)*KeyNum[i];
//            now_time    += 6ll*KeyNum[i]*i;
//        }
//    }
//    cerr << "target time "<< 1.0*now_time/target_time <<endl;
//    printf("for  cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();

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

void ChipMaskHDF5::readDataSetHashListOrder(int *&bpmap_head, int *&bpmap_len, uint64 *&bpmap_key,int *&bpmap_value,Position1 *&position_index, int index){
    herr_t status;
    //open dataset with datasetName
    std::string datasetName = DATASETNAME + std::to_string(index);
    hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);

    //read attribute of the dataset

    //uint32 attributeValues[ATTRIBUTEDIM];
    //hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
    //status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
    //uint32 rowOffset = attributeValues[0];
    //uint32 colOffset = attributeValues[1];
    //cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

    hid_t dspaceID = H5Dget_space(datasetID);

    hid_t dtype_id = H5Dget_type(datasetID);

    hid_t plistID = H5Dget_create_plist(datasetID);

    int rank = H5Sget_simple_extent_ndims(dspaceID);

    hsize_t dims[rank];
    status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);

    uint64 matrixLen = 1;
    for (int i = 0; i < rank; i++) {
        matrixLen *= dims[i];
    }

    int segment = 1;
    if (rank >= 3) {
        segment = dims[2];
    }

    uint64 *bpMatrix_buffer = new uint64[matrixLen]();

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

    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);



    uint32 bpmap_num = 0;
    bpmap_head = new int[MOD];
    int *mbpmap_head = new int[MOD];
    bpmap_len  = new int[MOD];
    int *bpmap_nxt = new int[dims[0]*dims[1]*dims[2]];
    bpmap_key = new uint64[dims[0]*dims[1]*dims[2]];
    uint64 *mbpmap_key = new uint64[dims[0]*dims[1]*dims[2]];
    bpmap_value = new int[dims[0]*dims[1]*dims[2]];
    int *mbpmap_value = new int[dims[0]*dims[1]*dims[2]];
    position_index   = new Position1[dims[0]*dims[1]];
    //int *mapKeyNum = new int[MOD];
    //for (int i=0;i<MOD;i++){
    //    mapKeyNum[i] = 0;
    //}
#pragma omp parallel for num_threads(16)
    for (int i=0;i<MOD;i++){
        mbpmap_head[i] = -1;
        bpmap_len[i] = 0;
    }

/*
 *  多线程插入
 */
    cerr << "dims[0] " << dims[0] << " dims[1] " << dims[1] << " dims[2] " << dims[2] << endl;
    TDEF(MAPINSERT)
    TSTART(MAPINSERT)
    int indexPos=0;
//#pragma omp parallel for num_threads(8)
    for (uint32 r = 0; r < dims[0]; r++) {
        //bpMatrix[r] = new uint64*[dims[1]];
        for (uint32 c = 0; c < dims[1]; c++) {
            //bpMatrix[r][c] = bpMatrix_buffer + r*dims[1]*dims[2] + c*dims[2];
            Position1 position = {c, r};
            int thread_pos = indexPos++;
            position_index[thread_pos] = position;
            if (rank >= 3) {
                segment = dims[2];
                for (int s = 0; s < segment; s++) {
                    uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
                    if (barcodeInt == 0) {
                        continue;
                    }
                    uint32 mapKey = barcodeInt%MOD;
                    bpmap_len[mapKey]++;
                    mbpmap_key[bpmap_num] = barcodeInt;
                    mbpmap_value[bpmap_num] = thread_pos;

                    bpmap_nxt[bpmap_num] = mbpmap_head[mapKey];
                    mbpmap_head[mapKey] = bpmap_num;

                    bpmap_num++;
//                    mapUse[mapKey]=false;
                }
            }
            else {
                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
                if (barcodeInt == 0) {
                    continue;
                }
                uint32 mapKey = barcodeInt%MOD;
                bpmap_len[mapKey]++;
                mbpmap_key[bpmap_num] = barcodeInt;
                mbpmap_value[bpmap_num] = thread_pos;
                bpmap_nxt[bpmap_num] = mbpmap_head[mapKey];
                mbpmap_head[mapKey] = bpmap_num;
                bpmap_num++;
            }
        }
    }
    TEND(MAPINSERT)
    TPRINT(MAPINSERT,"MAP INSERT TIME IS ")
    TDEF(MAPLEN)
    TSTART(MAPLEN)

//#pragma omp parallel for num_threads(64)
    bpmap_head[0] = 0;
    for (int i=1;i<MOD;i++){
        bpmap_head[i] = bpmap_head[i-1]+bpmap_len[i-1];
    }
#pragma omp parallel for num_threads(32)
    for (int key = 0;key < MOD;key++){
        int head_now = bpmap_head[key];
        int indexlen = 0;
        for(int i=mbpmap_head[key];i!=-1;i=bpmap_nxt[i]){
            bpmap_value[head_now+indexlen] = mbpmap_value[i];
            bpmap_key[head_now+indexlen]   = mbpmap_key[i];
            indexlen++;
        }
//        bpmap_head[key] = now_index;
    }
    TEND(MAPLEN)
    TPRINT(MAPLEN,"MAP LEN TIME IS ");
//    delete mbpmap_key;
//    delete mbpmap_value;
//    delete bpmap_nxt;

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
void ChipMaskHDF5::readDataSetHashListNoIndex(int *&bpmap_head, int *&bpmap_nxt, uint64 *&bpmap_key,Position1 *&position_index, int index){
    herr_t status;
    //open dataset with datasetName
    std::string datasetName = DATASETNAME + std::to_string(index);
    hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);

    //read attribute of the dataset

    //uint32 attributeValues[ATTRIBUTEDIM];
    //hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
    //status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
    //uint32 rowOffset = attributeValues[0];
    //uint32 colOffset = attributeValues[1];
    //cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

    hid_t dspaceID = H5Dget_space(datasetID);

    hid_t dtype_id = H5Dget_type(datasetID);

    hid_t plistID = H5Dget_create_plist(datasetID);

    int rank = H5Sget_simple_extent_ndims(dspaceID);

    hsize_t dims[rank];
    status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);

    uint64 matrixLen = 1;
    for (int i = 0; i < rank; i++) {
        matrixLen *= dims[i];
    }

    int segment = 1;
    if (rank >= 3) {
        segment = dims[2];
    }

    uint64 *bpMatrix_buffer = new uint64[matrixLen]();

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

    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);



    uint32 bpmap_num = 0;
    bpmap_head = new int[MOD];
    bpmap_nxt = new int[dims[0]*dims[1]*dims[2]];
    bpmap_key = new uint64[dims[0]*dims[1]*dims[2]];
    position_index   = new Position1[dims[0]*dims[1]*dims[2]];
    //int *mapKeyNum = new int[MOD];
    //for (int i=0;i<MOD;i++){
    //    mapKeyNum[i] = 0;
    //}
//#pragma omp parallel for num_threads(64)
    for (int i=0;i<MOD;i++){
        bpmap_head[i] = -1;
    }

/*
 *  多线程插入
 */
    cerr << "dims[0] " << dims[0] << " dims[1] " << dims[1] << " dims[2] " << dims[2] << endl;
    int indexPos=0;
//#pragma omp parallel for num_threads(8)
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
                    uint32 mapKey = barcodeInt%MOD;
                    //mapKeyNum[mapKey]++;
                    bpmap_key[bpmap_num] = barcodeInt;
                    position_index[bpmap_num] = position;
                    bpmap_nxt[bpmap_num] = bpmap_head[mapKey];
                    bpmap_head[mapKey] = bpmap_num;

                    bpmap_num++;
                }
            }
            else {
                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
                if (barcodeInt == 0) {
                    continue;
                }
                uint32 mapKey = barcodeInt%MOD;
                bpmap_key[bpmap_num] = barcodeInt;
                position_index[bpmap_num] = position;
                bpmap_nxt[bpmap_num] = bpmap_head[mapKey];
                bpmap_head[mapKey] = bpmap_num;
                bpmap_num++;
            }
        }
    }
//    int *KeyNum = new int[MOD];
//    for (int i=0;i<MOD;i++){
//        KeyNum[i]=0;
//    }
//    long long now_time(0),target_time(0);
//    int min_map_size = MOD+1;
//    int max_map_size = 0;
//    for (int i=0;i<MOD;i++){
//        min_map_size = min(min_map_size,mapKeyNum[i]);
//        max_map_size = max(max_map_size,mapKeyNum[i]);
//        KeyNum[mapKeyNum[i]]++;
//    }
//    cerr << "Min Map Num is "<< min_map_size<<endl;
//    cerr << "Max Map Num is "<< max_map_size<<endl;
//    for (int i=0;i<max_map_size;i++){
//        cerr << " Map Num " << i << " is "<<KeyNum[i] << endl;
//        if (i<=1){
//            target_time += 6ll*KeyNum[i];
//            now_time    += 6ll*KeyNum[i];
//        }else{
//            target_time += (5ll+i)*KeyNum[i];
//            now_time    += 6ll*KeyNum[i]*i;
//        }
//    }
//    cerr << "target time "<< 1.0*now_time/target_time <<endl;
//    printf("for  cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();

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
void ChipMaskHDF5::readDataSetHashListNoIndexWithBloomFilter(int *&bpmap_head, int *&bpmap_nxt, uint64 *&bpmap_key,Position1 *&position_index,BloomFilter* &bloomFilter,int index){
    herr_t status;
    //open dataset with datasetName
    std::string datasetName = DATASETNAME + std::to_string(index);
    hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);

    //read attribute of the dataset

    //uint32 attributeValues[ATTRIBUTEDIM];
    //hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
    //status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
    //uint32 rowOffset = attributeValues[0];
    //uint32 colOffset = attributeValues[1];
    //cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

    hid_t dspaceID = H5Dget_space(datasetID);

    hid_t dtype_id = H5Dget_type(datasetID);

    hid_t plistID = H5Dget_create_plist(datasetID);

    int rank = H5Sget_simple_extent_ndims(dspaceID);

    hsize_t dims[rank];
    status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);

    uint64 matrixLen = 1;
    for (int i = 0; i < rank; i++) {
        matrixLen *= dims[i];
    }

    int segment = 1;
    if (rank >= 3) {
        segment = dims[2];
    }

    uint64 *bpMatrix_buffer = new uint64[matrixLen]();

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

    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);



    uint32 bpmap_num = 0;
    bpmap_head = new int[MOD];
    bpmap_nxt = new int[dims[0]*dims[1]*dims[2]];
    bpmap_key = new uint64[dims[0]*dims[1]*dims[2]];
    position_index   = new Position1[dims[0]*dims[1]*dims[2]];
    bloomFilter = new BloomFilter();
//    bloomFilter ->push(462212724823577);
    //int *mapKeyNum = new int[MOD];
    //for (int i=0;i<MOD;i++){
    //    mapKeyNum[i] = 0;
    //}
//#pragma omp parallel for num_threads(64)
    for (int i=0;i<MOD;i++){
        bpmap_head[i] = -1;
    }

/*
 *  多线程插入
 */
    cerr << "dims[0] " << dims[0] << " dims[1] " << dims[1] << " dims[2] " << dims[2] << endl;
    int indexPos=0;
//#pragma omp parallel for num_threads(8)
    for (uint32 r = 0; r < dims[0]; r++) {
        //bpMatrix[r] = new uint64*[dims[1]];
        for (uint32 c = 0; c < dims[1]; c++) {
            //bpMatrix[r][c] = bpMatrix_buffer + r*dims[1]*dims[2] + c*dims[2];
            Position1 position = {c, r};
            if (rank >= 3) {
                segment = dims[2];
                for (int s = 0; s < segment; s++) {
                    uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
//                    cerr << barcodeInt << endl;
                    if (barcodeInt == 0) {
                        continue;
                    }
                    bloomFilter->push(barcodeInt);
//                    cout << 0 << endl;
                    uint32 mapKey = barcodeInt%MOD;
//                    cout << 1 << endl;
                    //mapKeyNum[mapKey]++;
                    bpmap_key[bpmap_num] = barcodeInt;
//                    cout << 2 << endl;
                    position_index[bpmap_num] = position;
//                    cout << 3 << endl;
                    bpmap_nxt[bpmap_num] = bpmap_head[mapKey];
//                    cout << 4 << endl;
                    bpmap_head[mapKey] = bpmap_num;
//                    cout << 5 << endl;
                    bpmap_num++;
                }
            }
            else{
                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
//                cerr << barcodeInt << endl;
                if (barcodeInt == 0) {
                    continue;
                }
                bloomFilter->push(barcodeInt);
//                cout << 0 << endl;
                uint32 mapKey = barcodeInt%MOD;
//                cout << 1 << endl;
                bpmap_key[bpmap_num] = barcodeInt;
//                cout << 2 << endl;
                position_index[bpmap_num] = position;
//                cout << 3 << endl;
                bpmap_nxt[bpmap_num] = bpmap_head[mapKey];
//                cout << 4 << endl;
                bpmap_head[mapKey] = bpmap_num;
//                cout << 5 << endl;
                bpmap_num++;
            }
        }
    }
//    int *KeyNum = new int[MOD];
//    for (int i=0;i<MOD;i++){
//        KeyNum[i]=0;
//    }
//    long long now_time(0),target_time(0);
//    int min_map_size = MOD+1;
//    int max_map_size = 0;
//    for (int i=0;i<MOD;i++){
//        min_map_size = min(min_map_size,mapKeyNum[i]);
//        max_map_size = max(max_map_size,mapKeyNum[i]);
//        KeyNum[mapKeyNum[i]]++;
//    }
//    cerr << "Min Map Num is "<< min_map_size<<endl;
//    cerr << "Max Map Num is "<< max_map_size<<endl;
//    for (int i=0;i<max_map_size;i++){
//        cerr << " Map Num " << i << " is "<<KeyNum[i] << endl;
//        if (i<=1){
//            target_time += 6ll*KeyNum[i];
//            now_time    += 6ll*KeyNum[i];
//        }else{
//            target_time += (5ll+i)*KeyNum[i];
//            now_time    += 6ll*KeyNum[i]*i;
//        }
//    }
//    cerr << "target time "<< 1.0*now_time/target_time <<endl;
//    printf("for  cost %.3f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();

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
void ChipMaskHDF5::readDataSetHashListOneArrayWithBloomFilter(int *&bpmap_head, int *&bpmap_nxt,
                                                              bpmap_key_value *&position_all, BloomFilter *&bloomFilter,
                                                              int index){
    herr_t status;
    //open dataset with datasetName
    std::string datasetName = DATASETNAME + std::to_string(index);
    hid_t datasetID = H5Dopen2(fileID, datasetName.c_str(), H5P_DEFAULT);

    //read attribute of the dataset

    //uint32 attributeValues[ATTRIBUTEDIM];
    //hid_t attributeID = H5Aopen_by_name(fileID, datasetName.c_str(), ATTRIBUTENAME, H5P_DEFAULT, H5P_DEFAULT);
    //status = H5Aread(attributeID, H5T_NATIVE_UINT32, &attributeValues[0]);
    //uint32 rowOffset = attributeValues[0];
    //uint32 colOffset = attributeValues[1];
    //cout << "row offset: " << rowOffset << "\tcol offset: "<< colOffset << endl;

    hid_t dspaceID = H5Dget_space(datasetID);

    hid_t dtype_id = H5Dget_type(datasetID);

    hid_t plistID = H5Dget_create_plist(datasetID);

    int rank = H5Sget_simple_extent_ndims(dspaceID);

    hsize_t dims[rank];
    status = H5Sget_simple_extent_dims(dspaceID, dims, NULL);

    uint64 matrixLen = 1;
    for (int i = 0; i < rank; i++) {
        matrixLen *= dims[i];
    }

    int segment = 1;
    if (rank >= 3) {
        segment = dims[2];
    }

    uint64 *bpMatrix_buffer = new uint64[matrixLen]();

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

    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);



    uint32 bpmap_num = 0;
    bpmap_head = new int[MOD];
    bpmap_nxt = new int[dims[0]*dims[1]*dims[2]];
    position_all = new bpmap_key_value[dims[0]*dims[1]*dims[2]];
    bloomFilter = new BloomFilter();
//    bloomFilter ->push(462212724823577);
    //int *mapKeyNum = new int[MOD];
    //for (int i=0;i<MOD;i++){
    //    mapKeyNum[i] = 0;
    //}
//#pragma omp parallel for num_threads(64)
    for (int i=0;i<MOD;i++){
        bpmap_head[i] = -1;
    }

/*
 *  多线程插入
 */
    cerr << "dims[0] " << dims[0] << " dims[1] " << dims[1] << " dims[2] " << dims[2] << endl;
    int indexPos=0;
//#pragma omp parallel for num_threads(8)
    for (uint32 r = 0; r < dims[0]; r++) {
        //bpMatrix[r] = new uint64*[dims[1]];
        for (uint32 c = 0; c < dims[1]; c++) {
            //bpMatrix[r][c] = bpMatrix_buffer + r*dims[1]*dims[2] + c*dims[2];
            Position1 position = {c, r};
            if (rank >= 3) {
                segment = dims[2];
                for (int s = 0; s < segment; s++) {
                    uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
//                    cerr << barcodeInt << endl;
                    if (barcodeInt == 0) {
                        continue;
                    }
//                    bloomFilter->push_xor(barcodeInt);
                    uint32 mapKey = barcodeInt%MOD;
                    position_all[bpmap_num].key = barcodeInt;
                    position_all[bpmap_num].value = position;
                    bpmap_nxt[bpmap_num] = bpmap_head[mapKey];
                    bpmap_head[mapKey] = bpmap_num;
                    bpmap_num++;
                }
            }
            else{
                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
//                cerr << barcodeInt << endl;
                if (barcodeInt == 0) {
                    continue;
                }
//                bloomFilter->push_xor(barcodeInt);
                uint32 mapKey = barcodeInt%MOD;
                position_all[bpmap_num].key = barcodeInt;
                position_all[bpmap_num].value = position;
                bpmap_nxt[bpmap_num] = bpmap_head[mapKey];
                bpmap_head[mapKey] = bpmap_num;
                bpmap_num++;
            }
        }
    }
#pragma omp parallel num_threads(2)
{
    int num_id = omp_get_thread_num();
    if (num_id == 0){
        for (uint32 r = 0; r < dims[0]; r++) {
            for (uint32 c = 0; c < dims[1]; c++) {
                if (rank >= 3) {
                    segment = dims[2];
                    for (int s = 0; s < segment; s++) {
                        uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
//                        bloomFilter->push_mod(barcodeInt);
                        uint32 a = barcodeInt&0xffffffff;
                        a = (a ^ 61) ^ (a >> 16);
                        a = a + (a << 3);
                        a = a ^ (a >> 4);
                        a = a * 0x27d4eb2d;
                        a = a ^ (a >> 15);
                        a = (a>>6)&0x3fff;
                        // 6 74
                        uint32 mapkey = (barcodeInt >> 32)|(a << 18);
                        bloomFilter->hashtable[mapkey>>6] |= (1ll<<(mapkey&0x3f));
                    }
                }
                else{
                    uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
//                    bloomFilter->push_mod(barcodeInt);
                    uint32 a = barcodeInt&0xffffffff;
                    a = (a ^ 61) ^ (a >> 16);
                    a = a + (a << 3);
                    a = a ^ (a >> 4);
                    a = a * 0x27d4eb2d;
                    a = a ^ (a >> 15);
                    a = (a>>6)&0x3fff;
                    // 6 74
                    uint32 mapkey = (barcodeInt >> 32)|(a << 18);
                    bloomFilter->hashtable[mapkey>>6] |= (1ll<<(mapkey&0x3f));
                }
            }
        }
    }else{
        for (uint32 r = 0; r < dims[0]; r++) {
            for (uint32 c = 0; c < dims[1]; c++) {
                Position1 position = {c, r};
                if (rank >= 3) {
                    segment = dims[2];
                    for (int s = 0; s < segment; s++) {
                        uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
//                        bloomFilter->push_Classification(barcodeInt);
                        uint32 mapkey = barcodeInt&0xffffffff;
                        bloomFilter->hashtableClassification[mapkey>>6] |= (1ll<<(mapkey&0x3f));
                    }
                }
                else{
                    uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
//                    bloomFilter->push_Classification(barcodeInt);
                    uint32 mapkey = barcodeInt&0xffffffff;
                    bloomFilter->hashtableClassification[mapkey>>6] |= (1ll<<(mapkey&0x3f));
                }
            }
        }
    }
}







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