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

herr_t ChipMaskHDF5::writeDataSet(std::string chipID, slideRange &sliderange,
                                  unordered_map<uint64, Position1> &bpMap,
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


void ChipMaskHDF5::readDataSet(int &hashNum, int *&hashHead, node *&hashMap, int &dims1, uint64 *&bloomFilter,
                               int index) {
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
    dims1 = dims[1];

    int segment = 1;
    if (rank >= 3) {
        segment = dims[2];
    }

//    uint64 *bpMatrix_buffer = new uint64[matrixLen]();
    uint64 *bpMatrix_buffer = new uint64[matrixLen];

    printf("new buffer cost %.6f\n", HD5GetTime() - t0);
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


    hashNum = 0;
    hashHead = new int[mod + 10];
#pragma omp parallel for num_threads(64)
    for (int i = 0; i < mod; i++) {
        hashHead[i] = -1;
    }
    uint64 totSize = 1ll * dims[0] * dims[1] * dims[2];
    hashMap = new node[totSize];

    bloomFilter = new uint64[1 << 28];

#pragma omp parallel for num_threads(64)
    for (int i = 0; i < (1 << 28); i++) {
        bloomFilter[i] = 0;
    }

    printf("new + init hash map and bf cost %.3f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();



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
                    //add item to bf
                    {
//                        uint32 idx0 = Hash0(barcodeInt);
//                        uint32 idx1 = Hash1(barcodeInt);
//                        uint32 idx2 = Hash2(barcodeInt);
                        uint32 idx3 = Hash3(barcodeInt);
//                        uint32 idx4 = Hash4(barcodeInt);


//                        bloomFilter[idx0 >> 6] |= 1ll << (idx0 & 0x3F);
//                        bloomFilter[idx1 >> 6] |= 1ll << (idx1 & 0x3F);
//                        bloomFilter[idx2 >> 6] |= 1ll << (idx2 & 0x3F);
                        bloomFilter[idx3 >> 6] |= 1ll << (idx3 & 0x3F);
//                        bloomFilter[idx4 >> 6] |= 1ll << (idx4 & 0x3F);
                    }

//                    printf("ready to insert is %lld\n", barcodeInt);
                    //add item to hash map
                    {
                        int key = barcodeInt % mod;
//                        int key = mol(barcodeInt);
//                        if (key >= mod)key -= mod;

//                        if (key != barcodeInt % mod) {
//                            printf("%lld %d %lld\n", barcodeInt, key, barcodeInt % mod);
//                            exit(0);
//                        }
//                        printf("after mod is %d\n", key);
                        int ok = 0;
                        //find item and update postion
                        for (int i = hashHead[key]; i != -1; i = hashMap[i].pre)
                            if (hashMap[i].v == barcodeInt) {
//                                hashMap[i].p1 = position;
//                                hashMap[i].x = c;
//                                hashMap[i].y = r;
                                hashMap[i].p = r * dims[1] + c;
                                ok = 1;
                                break;
                            }
                        //not find item, then add to map
                        if (ok == 0) {
                            hashMap[hashNum].v = barcodeInt;
//                            hashMap[hashNum].p1 = position;
//                            hashMap[hashNum].x = c;
//                            hashMap[hashNum].y = r;
                            hashMap[hashNum].p = r * dims[1] + c;

                            hashMap[hashNum].pre = hashHead[key];
                            hashHead[key] = hashNum;
                            hashNum++;
                        }
                    }
                }
            } else {
                uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
                if (barcodeInt == 0) {
                    continue;
                }
                //add item to bf
                {
//                        uint32 idx0 = Hash0(barcodeInt);
//                        uint32 idx1 = Hash1(barcodeInt);
//                        uint32 idx2 = Hash2(barcodeInt);
                    uint32 idx3 = Hash3(barcodeInt);
//                        uint32 idx4 = Hash4(barcodeInt);


//                        bloomFilter[idx0 >> 6] |= 1ll << (idx0 & 0x3F);
//                        bloomFilter[idx1 >> 6] |= 1ll << (idx1 & 0x3F);
//                        bloomFilter[idx2 >> 6] |= 1ll << (idx2 & 0x3F);
                    bloomFilter[idx3 >> 6] |= 1ll << (idx3 & 0x3F);
//                        bloomFilter[idx4 >> 6] |= 1ll << (idx4 & 0x3F);
                }

//                    printf("ready to insert is %lld\n", barcodeInt);
                //add item to hash map
                {
                    int key = barcodeInt % mod;
//                        int key = mol(barcodeInt);
//                        if (key >= mod)key -= mod;

//                        if (key != barcodeInt % mod) {
//                            printf("%lld %d %lld\n", barcodeInt, key, barcodeInt % mod);
//                            exit(0);
//                        }
//                        printf("after mod is %d\n", key);
                    int ok = 0;
                    //find item and update postion
                    for (int i = hashHead[key]; i != -1; i = hashMap[i].pre)
                        if (hashMap[i].v == barcodeInt) {
//                                hashMap[i].p1 = position;
//                                hashMap[i].x = c;
//                                hashMap[i].y = r;
                            hashMap[i].p = r * dims[1] + c;
                            ok = 1;
                            break;
                        }
                    //not find item, then add to map
                    if (ok == 0) {
                        hashMap[hashNum].v = barcodeInt;
//                            hashMap[hashNum].p1 = position;
//                            hashMap[hashNum].x = c;
//                            hashMap[hashNum].y = r;
                        hashMap[hashNum].p = r * dims[1] + c;

                        hashMap[hashNum].pre = hashHead[key];
                        hashHead[key] = hashNum;
                        hashNum++;
                    }
                }
            }
        }
    }


//    printf("now hashHead postion is %d\n", hashHead);
//    printf("test val is %d\n", hashHead[109547259]);
//    printf("now map size is %d\n", hashNum);

    printf("for  cost %.3f\n", HD5GetTime() - t0);


//    printf("now analyze hash map\n");
//
//    int64 sum = 0;
//    int64 mx = 0;
//    int64 mn = 1ll << 62;
//    int64 cntt[100] = {0};
//#pragma omp parallel for num_threads(64)
//    for (int key = 0; key < mod; key++) {
//        int64 cnt = 0;
//        for (int i = hashHead[key]; i != -1; i = hashMap[i].pre)
//            cnt++;
//#pragma critical
//        {
//            sum += cnt;
//            cntt[cnt]++;
//            mx = max(mx, cnt);
//            mn = min(mn, cnt);
//        }
//    }
//    printf("hash map mx size is %lld\n", mx);//7
//    printf("hash map mn size is %lld\n", mn);//0
//    printf("hash map tot size is %lld\n", sum);
//
//    printf("hash map avg size is %.8f\n", 1.0 * sum / mod);
//    printf("================\n");
//    for (int i = 0; i <= mx; i++)printf("%d %lld\n", i, cntt[i]);
//    printf("================\n");
//

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

//void HDF5Read_pack(){
//
//}
//
//void HDF5Decompressor_pack(){
//
//}





void ChipMaskHDF5::readDataSetHashListOneArrayWithBloomFilter(int *&bpmap_head, int *&bpmap_nxt,
                                                              bpmap_key_value *&position_all, BloomFilter *&bloomFilter,
                                                              int index) {

    auto t0 = HD5GetTime();
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

//    uint64 *bpMatrix_buffer = new uint64[matrixLen]();
//    uint64 *bpMatrix_buffer = new uint64[matrixLen];


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

    hsize_t nchunks;
    status = H5Dget_num_chunks(datasetID,H5S_ALL,&nchunks);
    size_t chunk_dims[rank];
    int flag=0;
    for(int r=0;r<rank;r++){
        if(dims[r]==1){
            flag++;
            chunk_dims[r]=1;
        }else{
            chunk_dims[r]=0;
        }

    }
    hsize_t offset_next[rank];
    for(int cid=1;cid<nchunks;cid++){
        H5Dget_chunk_info(datasetID,H5S_ALL,cid,offset_next,NULL,NULL,NULL);
        for(int r=0;r<rank;r++){
            if(!chunk_dims[r]&&offset_next[r]){
                chunk_dims[r]=offset_next[r];
                flag++;
            }
        }
        if(flag>=rank){
            break;
        }
    }

    hsize_t chunk_len=1;
    for(int r=0;r<rank;r++){
        chunk_len*=chunk_dims[r];
    }
    uint64** compressed_buffer = new uint64*[nchunks];
    uint64** buffer=new uint64*[nchunks];
    hsize_t **offset = new hsize_t*[nchunks];
    for (int i=0;i<nchunks;i++){
        compressed_buffer[i] = new uint64[chunk_len];
        buffer[i] = new uint64[chunk_len];
        offset[i] = new hsize_t[rank];
    }
    hsize_t *chunk_size = new hsize_t[nchunks];


    uint32 bpmap_num = 0;
    bpmap_head = new int[MOD];
    bpmap_nxt = new int[dims[0] * dims[1] * dims[2]];
    position_all = new bpmap_key_value[dims[0] * dims[1] * dims[2]];
    bloomFilter = new BloomFilter();

//    bloomFilter ->push(462212724823577);
    //int *mapKeyNum = new int[MOD];
    //for (int i=0;i<MOD;i++){
    //    mapKeyNum[i] = 0;
    //}

#pragma omp parallel for num_threads(16)
    for (int i = 0; i < MOD; i++) {
        bpmap_head[i] = -1;
    }


    printf("new and hdf5 pre cost %.6f\n", HD5GetTime() - t0);
    t0 = HD5GetTime();

    int chunk_num =0;
    int now_chunk[4];
    bool is_complete_hdf5=false;
#pragma omp parallel num_threads(4)
    {
        int num_id = omp_get_thread_num();
        if (num_id == 0){
            libdeflate_decompressor* decompressor=libdeflate_alloc_decompressor();
            for(int chunk_index=0;chunk_index<nchunks;chunk_index++){
                uint32_t filter=0;
                size_t actual_out=0;
                H5Dget_chunk_info(datasetID,H5S_ALL,chunk_index,offset[chunk_index],&filter,NULL,&chunk_size[chunk_index]);
                status = H5Dread_chunk(datasetID,H5P_DEFAULT,offset[chunk_index],&filter,compressed_buffer[chunk_index]);
//                int dstatus=libdeflate_zlib_decompress(decompressor,(void*)compressed_buffer[now_chunk],chunk_size[now_chunk],(void*)buffer[now_chunk],chunk_len*sizeof(uint64),&actual_out);
                int dstatus=libdeflate_zlib_decompress(decompressor,(void*)compressed_buffer[chunk_index],chunk_size[chunk_index],(void*)buffer[chunk_index],chunk_len*sizeof(uint64),&actual_out);
                chunk_num++;
            }
            printf("chunk num is %d   nchunks is %d\n",chunk_num,nchunks);
            printf("read and close hdf5 cost %.6f\n", HD5GetTime() - t0);
            is_complete_hdf5 = true;
            libdeflate_free_decompressor(decompressor);

        }
        if (num_id == 1){
            // HashTable
            now_chunk[1] = 0;

            while(now_chunk[1] < nchunks ){
//                printf("num id is %d  now chunk is %d  chunk num is %d  nchunks is %d\n",num_id,now_chunk[num_id],chunk_num,nchunks);
                while (now_chunk[1] < chunk_num){

                    for(int y=offset[now_chunk[1]][0];y<min(offset[now_chunk[1]][0]+chunk_dims[0],dims[0]);y++){
                        for(int x=offset[now_chunk[1]][1];x<min(offset[now_chunk[1]][1]+chunk_dims[1],dims[1]);x++){
                            Position1 position = {x, y};
                            uint64 barcodeInt = buffer[now_chunk[1]][(y-offset[now_chunk[1]][0])*chunk_dims[1]+x-offset[now_chunk[1]][1]];
                            if (barcodeInt == 0) {
                                continue;
                            }
                            uint32 mapKey = barcodeInt % MOD;
                            position_all[bpmap_num].key = barcodeInt;
                            position_all[bpmap_num].value = position;
                            bpmap_nxt[bpmap_num] = bpmap_head[mapKey];
                            bpmap_head[mapKey] = bpmap_num;
                            bpmap_num++;
                        }
                    }

                    now_chunk[1]++;

                }
                usleep(10);
            }
        }
        if (num_id == 2){
            // HashTable
            now_chunk[2] = 0;
            while(now_chunk[2] < nchunks){
//                printf("num id is %d  now chunk is %d  chunk num is %d  nchunks is %d\n",num_id,now_chunk[num_id],chunk_num,nchunks);
                while (now_chunk[2] < chunk_num){

                    for(int y=offset[now_chunk[2]][0];y<min(offset[now_chunk[2]][0]+chunk_dims[0],dims[0]);y++){
                        for(int x=offset[now_chunk[2]][1];x<min(offset[now_chunk[2]][1]+chunk_dims[1],dims[1]);x++){

                            uint64 barcodeInt = buffer[now_chunk[2]][(y-offset[now_chunk[2]][0])*chunk_dims[1]+x-offset[now_chunk[2]][1]];
                            if (barcodeInt == 0) {
                                continue;
                            }
                            uint32 a = barcodeInt & 0xffffffff;
                            a = (a ^ 61) ^ (a >> 16);
                            a = a + (a << 3);
                            a = a ^ (a >> 4);
                            a = a * 0x27d4eb2d;
                            a = a ^ (a >> 15);
                            a = (a >> 6) & 0x3fff;
                            // 6 74
                            uint32 mapkey = (barcodeInt >> 32) | (a << 18);
                            bloomFilter->hashtable[mapkey >> 6] |= (1ll << (mapkey & 0x3f));
                        }
                    }
                    now_chunk[2]++;

                }
                usleep(10);
            }

        }

        if (num_id == 3){
            // HashTable
            now_chunk[3] = 0;
            while(now_chunk[3] < nchunks){
//                printf("num id is %d  now chunk is %d  chunk num is %d  nchunks is %d\n",num_id,now_chunk[num_id],chunk_num,nchunks);
                while (now_chunk[3] < chunk_num){

                    for(int y=offset[now_chunk[3]][0];y<min(offset[now_chunk[3]][0]+chunk_dims[0],dims[0]);y++){
                        for(int x=offset[now_chunk[3]][1];x<min(offset[now_chunk[3]][1]+chunk_dims[1],dims[1]);x++){
                            Position1 position = {x, y};
                            uint64 barcodeInt = buffer[now_chunk[3]][(y-offset[now_chunk[3]][0])*chunk_dims[1]+x-offset[now_chunk[3]][1]];
                            if (barcodeInt == 0) {
                                continue;
                            }
                            uint32 mapkey = barcodeInt & 0xffffffff;
                            bloomFilter->hashtableClassification[mapkey >> 6] |= (1ll << (mapkey & 0x3f));
                        }
                    }
                    now_chunk[3]++;

                }
                usleep(10);
            }

        }

    }


    status = H5Dclose(datasetID);
    status = H5Fclose(fileID);
//        for(int chunk_index=0;chunk_index<nchunks;chunk_index++){
//            H5Dget_chunk_info(datasetID,H5S_ALL,chunk_index,offset[chunk_index],&filter,NULL,&chunk_size);
//            status = H5Dread_chunk(datasetID,H5P_DEFAULT,offset[chunk_index],&filter,compressed_buffer[chunk_index]);
//            libdeflate_decompressor* decompressor=libdeflate_alloc_decompressor();
//            size_t actual_out=0;
//            int dstatus=libdeflate_zlib_decompress(decompressor,(void*)compressed_buffer[chunk_index],chunk_size,(void*)buffer[chunk_index],chunk_len*sizeof(uint64),&actual_out);
//            for(int y=offset[chunk_index][0];y<offset[chunk_index][0]+chunk_dims[0]&&y<dims[0];y++){
//                for(int x=offset[chunk_index][1];x<offset[chunk_index][1]+chunk_dims[1]&&x<dims[1];x++){
//                    bpMatrix_buffer[y*dims[1]+x]=buffer[chunk_index][(y-offset[chunk_index][0])*chunk_dims[1]+x-offset[chunk_index][1]];
//                }
//            }
//            libdeflate_free_decompressor(decompressor);
//        }




//    status = H5Dread(datasetID, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, bpMatrix_buffer);
//    status = H5Aclose(attributeID);
//    status = H5Dclose(datasetID);
//    status = H5Fclose(fileID);
    printf("read and close hdf5 cost %.6f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();
//
//
//
//
//    printf("new,init bf and hashMap cost %.6f\n", HD5GetTime() - t0);
//    t0 = HD5GetTime();
//
///*
// *  多线程插入
// */
////    cerr << "dims[0] " << dims[0] << " dims[1] " << dims[1] << " dims[2] " << dims[2] << endl;
//    int indexPos = 0;
////#pragma omp parallel for num_threads(8)
//
//#pragma omp parallel num_threads(3)
//    {
//        int num_id = omp_get_thread_num();
//        if (num_id == 0) {
//            for (uint32 r = 0; r < dims[0]; r++) {
//                //bpMatrix[r] = new uint64*[dims[1]];
//                for (uint32 c = 0; c < dims[1]; c++) {
//                    //bpMatrix[r][c] = bpMatrix_buffer + r*dims[1]*dims[2] + c*dims[2];
//                    Position1 position = {c, r};
//                    if (rank >= 3) {
//                        segment = dims[2];
//                        for (int s = 0; s < segment; s++) {
//                            uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
//                            //                    cerr << barcodeInt << endl;
//                            if (barcodeInt == 0) {
//                                continue;
//                            }
//                            //                    bloomFilter->push_xor(barcodeInt);
//                            uint32 mapKey = barcodeInt % MOD;
//                            position_all[bpmap_num].key = barcodeInt;
//                            position_all[bpmap_num].value = position;
//                            bpmap_nxt[bpmap_num] = bpmap_head[mapKey];
//                            bpmap_head[mapKey] = bpmap_num;
//                            bpmap_num++;
//                        }
//                    } else {
//                        uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
//                        //                cerr << barcodeInt << endl;
//                        if (barcodeInt == 0) {
//                            continue;
//                        }
//                        //                bloomFilter->push_xor(barcodeInt);
//                        uint32 mapKey = barcodeInt % MOD;
//                        position_all[bpmap_num].key = barcodeInt;
//                        position_all[bpmap_num].value = position;
//                        bpmap_nxt[bpmap_num] = bpmap_head[mapKey];
//                        bpmap_head[mapKey] = bpmap_num;
//                        bpmap_num++;
//                    }
//                }
//            }
//        }
//        if (num_id == 1) {
//            for (uint32 r = 0; r < dims[0]; r++) {
//                for (uint32 c = 0; c < dims[1]; c++) {
//                    if (rank >= 3) {
//                        segment = dims[2];
//                        for (int s = 0; s < segment; s++) {
//                            uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
////                        bloomFilter->push_mod(barcodeInt);
//                            uint32 a = barcodeInt & 0xffffffff;
//                            a = (a ^ 61) ^ (a >> 16);
//                            a = a + (a << 3);
//                            a = a ^ (a >> 4);
//                            a = a * 0x27d4eb2d;
//                            a = a ^ (a >> 15);
//                            a = (a >> 6) & 0x3fff;
//                            // 6 74
//                            uint32 mapkey = (barcodeInt >> 32) | (a << 18);
//                            bloomFilter->hashtable[mapkey >> 6] |= (1ll << (mapkey & 0x3f));
//                        }
//                    } else {
//                        uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
////                    bloomFilter->push_mod(barcodeInt);
//                        uint32 a = barcodeInt & 0xffffffff;
//                        a = (a ^ 61) ^ (a >> 16);
//                        a = a + (a << 3);
//                        a = a ^ (a >> 4);
//                        a = a * 0x27d4eb2d;
//                        a = a ^ (a >> 15);
//                        a = (a >> 6) & 0x3fff;
//                        // 6 74
//                        uint32 mapkey = (barcodeInt >> 32) | (a << 18);
//                        bloomFilter->hashtable[mapkey >> 6] |= (1ll << (mapkey & 0x3f));
//                    }
//                }
//            }
//        }
//        if (num_id == 2) {
//            for (uint32 r = 0; r < dims[0]; r++) {
//                for (uint32 c = 0; c < dims[1]; c++) {
//                    Position1 position = {c, r};
//                    if (rank >= 3) {
//                        segment = dims[2];
//                        for (int s = 0; s < segment; s++) {
//                            uint64 barcodeInt = bpMatrix_buffer[r * dims[1] * segment + c * segment + s];
////                        bloomFilter->push_Classification(barcodeInt);
//                            uint32 mapkey = barcodeInt & 0xffffffff;
//                            bloomFilter->hashtableClassification[mapkey >> 6] |= (1ll << (mapkey & 0x3f));
//                        }
//                    } else {
//                        uint64 barcodeInt = bpMatrix_buffer[r * dims[1] + c];
////                    bloomFilter->push_Classification(barcodeInt);
//                        uint32 mapkey = barcodeInt & 0xffffffff;
//                        bloomFilter->hashtableClassification[mapkey >> 6] |= (1ll << (mapkey & 0x3f));
//                    }
//                }
//            }
//        }
//    }
//
//    printf("parallel for cost %.6f\n", HD5GetTime() - t0);


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

//    delete[] bpMatrix_buffer;
    //delete[] bpMatrix;
}
