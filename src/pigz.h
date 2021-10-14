//
// Created by ylf9811 on 2021/10/14.
//

#ifndef PAC2022_PIGZ_H
#define PAC2022_PIGZ_H

#include "readerwriterqueue.h"
#include "atomicops.h"
#include <atomic>

int main_pigz(int argc, char **argv, moodycamel::ReaderWriterQueue<std::pair<char *, int>> *Q, std::atomic_int *wDone,
              std::pair<char *, int> &L);

#endif //PAC2022_PIGZ_H
