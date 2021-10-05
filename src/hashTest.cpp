#include <iostream>
#include <sparsehash/dense_hash_map>
#include <sparsehash/sparse_hash_map>
#include <sparsepp/spp.h>

#include <cstring>
#include <algorithm>
#include <sys/time.h>
#include <vector>
#include <random>
#include <unordered_map>
#include <omp.h>

#define mm long long
//#define mm int

#define nn int
//using google::sparse_hash_map;      // namespace where class lives by default
//using google::dense_hash_map;      // namespace where class lives by default
//
//using spp::sparse_hash_map;


using std::cout;
using std::endl;
static const int mod = 1073807359;
static const long long mod2 = (1ll << 50);


double MainGetTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double) tv.tv_sec + (double) tv.tv_usec / 1000000;
}

typedef struct node {
    int pre;
    mm v;
    int pos;
} node;


int main() {

    /*************************************************************************************************/
    double t0 = MainGetTime();


    std::random_device rd;
    std::mt19937_64 gen(rd());

    std::uniform_int_distribution<mm> dis;


    std::vector<mm> G;
    std::vector<mm> P;
    int id;
    int cnt;
    int M = 6e7;
    nn N = 1 << 30;

    for (int i = 0; i < M; i++) {
        G.push_back(dis(gen) % mod2);
    }
    for (nn i = 0; i < N; i++) {
        P.push_back(dis(gen) % mod2);
    }
    printf("init cost %lf\n", MainGetTime() - t0);

    /*************************************************************************************************/


    t0 = MainGetTime();
    google::dense_hash_map<mm, int> deMap;

    deMap.set_empty_key(NULL);
    id = 0;
    for (auto it:G) {
        id++;
        deMap[it] = id;
    }
    printf("google::dense_hash_map insert cost %lf\n", MainGetTime() - t0);

    t0 = MainGetTime();
    cnt = 0;
//#pragma omp parallel for num_threads(32) reduction(+:cnt)
    for (nn i = 0; i < P.size(); i++) {
        auto it = P[i];
        if (deMap.count(it))cnt++;
//        if (mps.find(it) != mps.end())cnt++;
    }


    printf("google::dense_hash_map find %d\n", cnt);
    printf("google::dense_hash_map find cost %lf\n", MainGetTime() - t0);

    /*************************************************************************************************/
    t0 = MainGetTime();
    google::sparse_hash_map<mm, int> spMap;


    id = 0;
    for (auto it:G) {
        id++;
        spMap[it] = id;
    }
    printf("google::sparse_hash_map insert cost %lf\n", MainGetTime() - t0);

    t0 = MainGetTime();
    cnt = 0;
//#pragma omp parallel for num_threads(32) reduction(+:cnt)
    for (nn i = 0; i < P.size(); i++) {
        auto it = P[i];
        if (spMap.count(it))cnt++;
//        if (months.find(it) != months.end())cnt++;

    }


    printf("google::sparse_hash_map find %d\n", cnt);
    printf("google::sparse_hash_map find cost %lf\n", MainGetTime() - t0);
    /*************************************************************************************************/

    t0 = MainGetTime();
    spp::sparse_hash_map<mm, int> sppMap;


    id = 0;
    for (auto it:G) {
        id++;
        sppMap[it] = id;
    }
    printf("spp::sparse_hash_map insert cost %lf\n", MainGetTime() - t0);

    t0 = MainGetTime();
    cnt = 0;
//#pragma omp parallel for num_threads(32) reduction(+:cnt)
    for (nn i = 0; i < P.size(); i++) {
        auto it = P[i];
        if (sppMap.count(it))cnt++;
//        if (months.find(it) != months.end())cnt++;

    }


    printf("spp::sparse_hash_map find %d\n", cnt);
    printf("spp::sparse_hash_map find cost %lf\n", MainGetTime() - t0);
    /*************************************************************************************************/
    t0 = MainGetTime();
    id = 0;

    int *hashHead = new int[mod + 10];
    for (int i = 0; i < mod; i++)hashHead[i] = -1;
    node *hashMap = new node[M + 10];
    int hashNum = 0;
    for (auto it:G) {
        id++;
        int key = it % mod;
        int ok = 0;
        for (int i = hashHead[key]; i != -1; i = hashMap[i].pre)
            if (hashMap[i].v == it) {
                hashMap[i].pos = id;
                ok = 1;
                break;
            }
        if (ok == 0) {
            hashMap[hashNum].v = it;
            hashMap[hashNum].pos = id;
            hashMap[hashNum].pre = hashHead[key];
            hashHead[key] = hashNum;
            hashNum++;
        }
    }
    printf("gogo insert cost %lf\n", MainGetTime() - t0);
    t0 = MainGetTime();
    cnt = 0;
//#pragma omp parallel for num_threads(32) reduction(+:cnt)
    for (nn i = 0; i < P.size(); i++) {
        auto it = P[i];
        int ok = 0;
        int pos = -1;
        int key = it % mod;
        for (int i = hashHead[key]; i != -1; i = hashMap[i].pre) {
            if (hashMap[i].v == it) {
                pos = hashMap[i].pos;
                ok = 1;
                break;
            }
        }
        if (ok)cnt++;
    }


    printf("gogo find %d\n", cnt);
    printf("gogo find cost %lf\n", MainGetTime() - t0);

    /*************************************************************************************************/


    t0 = MainGetTime();
    std::unordered_map<mm, int> stlMap;
    id = 0;
    for (auto it:G) {
        id++;
        stlMap[it] = id;
    }
    printf("stl unordered_map insert cost %lf\n", MainGetTime() - t0);


    t0 = MainGetTime();
    cnt = 0;
//#pragma omp parallel for num_threads(32) reduction(+:cnt)
    for (nn i = 0; i < P.size(); i++) {
        auto it = P[i];
        if (stlMap.count(it))cnt++;
//        if (mp2s.find(it) != mp2s.end())cnt++;

    }


    printf("stl unordered_map find %d\n", cnt);
    printf("stl unordered_map find cost %lf\n", MainGetTime() - t0);

    /*************************************************************************************************/

    return 0;
}

