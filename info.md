# PAC2021 final -- ChiFanBuZhangRou

TODOs

- [x] Add rabbitio
- [x] Use other map
- [ ] Parallel in load map----8*hashmap，omp merge
- [ ] Use secondary index
- [x] Use list-hash
- [ ] Use aili-code ？？？
- [ ] Optimize match algorithm
- [x] Use gcc7 / gcc11
- [x] Use icc
- [x] Add pugz
- [ ] Read se not pe
- [x] Use parallel zip when write
- [ ] Check todos 👇（//TODO）
- [x] Why write .fq.gz size diff ?
- [ ] Write fq in order?
- [x] queue1 more bigger (128x128 -> 128x1024)
- [ ] change block size in pugz(32kb -> 4mb)
- [x] change block size in pigz
- [ ] change queue1 to dataPool to decrease new and delete operations
- [x] fix pigzWrite bug？？
- [ ] mod all barcode to 1e9, use it dirctely, cal time
- [x] test G‘s map
- [ ] test 0 3 6 9
- [x] merge mip write part
- [x] check👆
- [x] add bloom filter
- [ ] make bf bitset smaller or more small bitset to replace the big one
- [ ] make bf bitset bigger
- [ ] try bf with 3 bitset
- [x] calculate bf size
- [x] add mpi pugz
- [x] add mpi pigz
- [ ] merge hashHead hashMap
- [ ] check asm to find why gcc11 has a good perfermance
- [ ] check pugz&producer and writer&pigz part
- [ ] inline query function by 👋
- [ ] fix pugz size bug？？
- [ ] make pigz not flush to disk
- [ ] 

## 0908

淦之前写的info整不见了，麻了。

| version                                               | total  | hdf5  | others |      |      |
| ----------------------------------------------------- | ------ | ----- | ------ | ---- | ---- |
| init                                                  | ～1100 | ～100 | ～1000 |      |      |
| add rabbitio from rabbitqc                            | 750    | 100   | 650    |      |      |
| no gz                                                 | 386    | 100   | 286    |      |      |
| no gz && replace std::unordered_map to robin_hood::xx | 220    | 50    | 170    |      |      |
| no gz && use list hash                                | 179    | 28    | 151    |      |      |
| use list hash                                         | 500    | 29    | 191+x  |      |      |

//TODO 为了过编译把boost部分涉及到map的操作做了复制，还有析构的时候swap都注释掉了

## 0909

目前先搞搞消费者吧，先不整gz，让读和写线程不会是瓶颈。上面是简单把map换了，换前后都能拉起来64线程，说明消费者还有提升空间，今天准备试试链式hash，感觉思路和主办方给的二级索引应该差不多。

确实有点子提升，但是没有特别快，更新👆。

写的时候有点子小小的bug，基础不牢地动山摇啊，本来以为传指针进去就不用&了，但之前的用法都是修改指针指向的空间里面的信息，这可以不用&，但是今天属于是直接该指针的值了。

## 0910

好像听说并行文件系统每次写波动太大，开放了计算节点上的user目录，这里简单测测对我们有没有影响：

|                                                       |                 |
| ----------------------------------------------------- | --------------- |
| Use list hash                                         | 499+500+510+500 |
| no gz 👆                                               | 193+189+191     |
| no gz icpc  -Ofast -xHost -qopt-zmm-usage=high        | 192             |
| no gz g++  -Ofast -funroll-loops -flto  -march=native | 186+188         |
| gz   g++  -Ofast -funroll-loops -flto  -march=native  | 530+544         |

目前读写gz大约只能用起30个核心，比起之前的50个核心有了一定的提升。

简单换一个gcc版本和icpc试试，但是编译的时候报错了，初步猜测是编译好的hdf5和boost库是用的gcc4，相差太多不能用，用gcc7重新搞一份。

## 0911

没啥用啊，用gcc7编译的boost还是报错，并且找了个简单的例子，也还是报错，目前猜测就是gcc7和gcc11自己安装的时候少了什么步骤。现在的内存使用大约是list hash map就用了10.5G左右，然后后面整个消费者们在用3G左右，如果写gz的话会来不及压缩导致积压在内存里，能到20G以上。

简单换了一下编译器和编译参数，update👆。

多多少少有点用吧可能。

今天这机器不大行啊。

c，zz别开箱子了可。

## 0912

今天看了看pugz，具体原理emmm论文太长了属实是读不下去，改天去图书馆慢慢看吧。

稍微修改了代码，好像多线程现在就是流式处理的？？？把原来wite to stdout改成了to file，去shm下简单测了测性能，基本上8线程能到30s以下，非常不错符合赛题的要求，现在再测测直接把20G的玩意整到内存里看看咋样。

## 0913

感觉pugz也一定程度上可以流式处理，简单加到了决赛题里面

|                                      | Total |      |
| ------------------------------------ | ----- | ---- |
| add pugz (one thread), in gz, out fq | 187   |      |
| Submit4, in gz, out fq               | 216   |      |
|                                      |       |      |

单线程的话要比普通的解压缩好一点，但是并没有直接pugz单线程和gunzip那样明显，而且现在多线程会出现错误，即解压出来的两块大小虽然一样但是里面的回车不一样多。。。。

## 0914

giao

ConcurrentQueue不是有序的，麻了。换成了同一个人搞得ReaderWriterQueue，一对一，现在能过check了，简单测试一下：

|                           | insert | pugz1/pugz2/producer/consumer/total | run use number of consumer             |      |
| ------------------------- | ------ | ----------------------------------- | -------------------------------------- | ---- |
| init in gz out fq         | 24     | x/x/189/190/214                     | <30 ~28                                |      |
| pugz thread1 in gz out fq | 23     | 140/140/158/160/190                 | 不稳定，有时候能到30多，大部分时间50多 |      |
| pugz thread2 in gz out fq | 27     | 130/140/160/170/190                 | 👆                                      |      |
|                           |        |                                     |                                        |      |
|                           |        |                                     |                                        |      |
|                           |        |                                     |                                        |      |
|                           |        |                                     |                                        |      |

感觉效果一般啊，而且运行他给的gz文件还老是出错。。。。自己重新压缩了两个gz（一个要半小时，，，，淦）

|                               | pugz1/pugz2/producer/consumer/total |      |
| ----------------------------- | ----------------------------------- | ---- |
| no process gz in no out，init | x/x/185/190/210                     |      |
| 👆，pugz thread 1              | 46/50/46/50/75                      |      |
| 👆，pugz thread 2              | 35/37/35/37/62                      |      |
| 👆，pugz thread 4              | 30/33/52/53/78                      |      |
| 👆，pugz thread 8              | 31/30/56/59/84                      |      |

虽然两个线程最好有点子奇怪，但是基本上时间还比较合理，但是不加no process的话要跑100+s，不太合理。

## 0915

|                                   |      |            | producer   | consumer | total |
| --------------------------------- | ---- | ---------- | ---------- | -------- | ----- |
| in gz，out fq                     | 12G  | ～3500%    | 187        | 190      | 214   |
| pugz thread2👆                     | 35G  | >6000%     | 153(100)   | 160      | 190   |
| pugz thread4👆                     | x    | 不稳定。。 | x          | x        | x     |
| dataPool 1024,dataQueue 2048 init | 13G  | ～3000%    | 189        | 190      | 218   |
| pugz thread2👆                     | 30G  | 不稳定     | 150        | 180      | 204   |
| in fq，out fq                     | 17G  | ～6400%    | 144        | 160      | 188   |
| real data init                    | 12G  | ～3500%    | 189        | 190      | 218   |
| real data 128/500 pugz thread2    | <30G | ～6000%    | 157（120） | 160      | 190   |
|                                   |      |            |            |          |       |

把dataPool和dataQueue弄大了也没啥用。

表里面的生产者时间其实也没有太大的参考价值，因为可能消费者太慢把dataQueue堵住了，生产者在等待；可以发现init读gz的时候P和C的时间基本一样，这里应该是C在等P，P搞出来最后一块数据C处理完就都结束了，而读fq或者pugz的话P和C的时间有一点差距，应该是C太慢，queue满了，P中间等待了，最后P搞完队列里还有些数据，然后C处理完。



## 0916

最近今天属于是脑子不太清醒，整不明白为啥消费者多用了接近一倍但是时间没咋变？

今天先赶赶进度，把bgzip加上，要不压缩太慢了，昨天在fat上试了试，bgzip压缩出来的gz文件gunzip是可以读的，但是pugz不支持。

下面测试一下，输出到/dev/shm

| Unzip           | p1.fq |      |
| --------------- | ----- | ---- |
| unpigz thread 1 | 83    |      |
| unpigz thread 2 | 66    |      |
| unpigz thread 4 | 66    |      |
| unpigz thread 8 | 66    |      |
| pugz thread 1   | 59    |      |
| pugz thread 2   | 40    |      |
| pugz thread 4   | 27    |      |
| gunzip          | 150   |      |

| Zip               | p1.fq  |      |
| ----------------- | ------ | ---- |
| pigz thread 1     | >1800  |      |
| pigz thread 2     | ～1000 |      |
| pigz thread 4     | ～500  |      |
| pigz thread 8     | ～250  |      |
| gzip              | ~1800  |      |
| bgzip thread 1 -1 | 400    |      |
| bgzip thread 2 -1 | 200    |      |
| bgzip thread 4 -1 | 100    |      |
| bgzip thread 8 -1 | 52     |      |
|                   |        |      |
|                   |        |      |
| pigz thread 1 -4  | 420    |      |
| pigz thread 2 -4  | 210    |      |
| pigz thread 4 -4  | 105    |      |
| pigz thread 8 -4  | 53     |      |
| pigz thread 16 -4 |        |      |
| gzip -4           | 450～  |      |
| bgzip thread 1 -4 | 240    |      |
| bgzip thread 2 -4 | 120    |      |
| bgzip thread 4 -4 | 60     |      |
| bgzip thread 8 -4 | 32     |      |
|                   |        |      |
|                   |        |      |

有点奇怪为啥代码里面的gzwrite压缩起来那么快？

## 0917

淦，代码里面是-4，只就用gzip或者pigz -4也很快，而且简单测试之后发现pigz的线程加速比还是挺好的；而bgzip作为分块压缩似乎比pigz效果更好，但是pugz没法处理bgzip压缩出来的gz文件，pugz可以用两个线程处理pigz压缩出来的文件（//TODO 线程数多的时候可能会报错）。

综上，综合考虑速度可易用性，准备使用pugz进行多线程的解压缩（这个可以正常处理gzip压缩出来的文件，也可以两个线程//TODO处理pigz压缩出来的文件，但是无法处理bgzip压缩出来的文件---只会解压第一块），然后使用pigz进行压缩（这样压缩出来的文件到下一个流程使用普通的gunzip完全可以，使用我们自己的代码开两个线程读也是可以的）。

## 0918

解封了芜湖

|                             |      |         |            |      |      |
| --------------------------- | ---- | ------- | ---------- | ---- | ---- |
| in gz                       | 12G  | ～3500% | 189        | 190  | 218  |
| in gz，pugz thread 2        | <30G | ～6000% | 157（120） | 160  | 190  |
| /users in gz                | 12G  | ～3500% | 190        | 190  | 219  |
| /users in gz，pugz thread 2 | <30G | ～6000% | 157（120） | 160  | 190  |
| /users in gz，pugz thread 1 | <30G | >6000%  | 151（120） | 160  | 183  |
| /users in fq                |      |         |            |      |      |

给pugz和producer之间的queue加个大小限制吧。

淦 好像死锁了。。。。

## 0919

未曾设想过的错误，，，，sleep单位居然是s，我怎么记得之前用的都是ms啊。。。然后队列1满了的时候pugz线程就开始了长达1000s的沉睡。。。。

改成usleep之后有加了个producer结束的tag，如果producer结束了pugz线程就停止解压了。

|                                             |      |                  |                 |      |      |
| ------------------------------------------- | ---- | ---------------- | --------------- | ---- | ---- |
| in gz，pugz thread 2，queue 128*128         | <18G | ～6000%          | 157（150，157） | 160  | 192  |
| in gz，pugz thread 1，queue 128*128         | <18G | ～6000%          | 157（150，157） | 160  | 192  |
| get map and pugz same time thread 1         | <18G | >6000%           | 180 180 180     | 180  | 187  |
| get map and pugz same time thread 1         | <18G | >6000%           | 180 180 190     | 190  | 197  |
| queue1：1<<20，in gz，out gz，pugz thread 1 | ~60G | 6400->5000->6400 | 186 89 94       | 189  | 191  |

发现开始的几秒似乎线程都用不起来，应该是P在等pugz，这块时间可以先简单的和hashmap insert放到一块，反正互不影响。

//TODO 把queue1开的大一点方便一开始掩盖pugz的时候能预处理出更多的块。

//TODO 修改pugz每一块的大小

//TODO 给queue1改成一个内存池，减少new和delete

好啊，基本上生产者们搞得大体差不多了（P和pugz），现在搞一下write。

|                                             |      |              |             |      |      |
| ------------------------------------------- | ---- | ------------ | ----------- | ---- | ---- |
| 👆 pugz thread 1，in gz，out gz              | 30G  | 4000%～5000% | 204 200 210 | 210  | 566  |
| 👆 pugz thread 2，in gz，out gz              | 30G  | 4000%～5000% | 204 200 210 | 210  | 566  |
| queue1：1<<20，in gz，out gz，pugz thread 1 | <60G | ～5000%      | 197 90 90   | 198  | 530  |
| queue1：1<<20，in gz，out gz，pugz thread 2 | <60G | ～6000%      | 194 57 62   | 195  | 538  |
|                                             |      |              |             |      |      |
|                                             |      |              |             |      |      |

## 0920

简单看了看pigz的文档，没有api但是stack overflow里面有一个pigz的开发者回答的问题，说是要支持pipe，看更新日志大约回答这个问题两个月之后pigz真的加了这块功能，但是稍微看了看参数没找到咋用。。。还是直接改代码吧。

把pigz加到决赛的代码里（直接把pigz.c复制到multi.cpp里面的，很丑陋。。。。然后改了yarn.h中的关键字，然后把.h文件都加了extren C，把comliain(x,...)给注释了，勉强可以运行了）

现在简单测试一下，先用原来的write把fq写到硬盘上，再用开个线程用pigz压缩。

|                                                   |               |               |           |         |      |
| ------------------------------------------------- | ------------- | ------------- | --------- | ------- | ---- |
| add pigz thread 2，in gz，out gz，pugz thread 1👆  | <60G          | ～5000%/6400% | 193 90 95 | 196+242 | 443  |
| add pigz thread 16，in gz，out gz，pugz thread 1👆 | <60G          | ～6000%/6400% | 191 90 95 | 195+47  | 244  |
| add pigz thread 32，in gz，out gz，pugz thread 2👆 | <60G（～50G） | ～6000%/6400% | 191 55 59 | 195+39  | 238  |
| add pigz thread 64，in gz，out gz，pugz thread 2👆 | <60G（～50G） | ～6000%/6400% | 184 55 59 | 188+30  | 219  |

## 0921 

```
rm -rf combine_read.fq && rm -rf combine_read.fq.gz && ../ST_BarcodeMap-0.0.1 --in DP8400016231TR_D1.barcodeToPos.h5 --in1 V300091300_L03_read_1.fq.gz  --in2 V300091300_L04_read_1.fq.gz --out combine_read.fq --mismatch 2 --thread 64 --usePugz --pugzThread 2 --usePigz --pigzThread 32
```

```c++
		int th_num = mOptions->pigzThread;
    printf("th num is %d\n", th_num);
    string th_num_s = to_string(th_num);
    printf("th num s is %s\n", th_num_s.c_str());
    printf("th num s len is %d\n", th_num_s.length());

    infos[2] = new char[th_num_s.length() + 1];
    memcpy(infos[2], th_num_s.c_str(), th_num_s.length());
```

这段代码好像emmm，char*后面找不到结束符，用的时候就可能出问题，现在的策略是再加一句

```c++
infos[2][th_num_s.length()] = '\0';
```

感觉不太优雅。

update table 👆

## 0922

原来的pigz代码里包括了输入文件xx.fq是否存在的检查，现在只能是把out参数的xx.fq.gz改成xx.fq，用源代码writer的new ofstream生成一个空的xx.fq欺骗一下pigz（5000行代码是在不想动）。

然后现在在writer和pigz之间加了一个queue，大小还是1<<20，把pigz的块大小改成了4M。

|                                             |      |               | producer(pugz) | comsumer(pigz) | total |
| ------------------------------------------- | ---- | ------------- | -------------- | -------------- | ----- |
| no more disk write and read，pugz 2，pigz 4 | ~60G | ～5000%/6400% | 195 61 66      | 198 201        | 201   |
| no more disk write and read，pugz 2，pigz 2 | ~60G | ～5000%/6400% | 193 62 67      | 194 283        | 294   |
| no more disk write and read，pugz 1，pigz 4 | ~60G | ～5000%/6400% | 183 95 100     | 187 189        | 190   |
| 👆pigz block size 8M，pugz 1，pigz 4         | ~60G | ～5000%/6400% |                |                | 203   |

这个测得时间有点子波动。。。算了先不管了，反正大体上，单独测试的话压缩和解压缩大约30s左右，在程序里测试的话压缩大约60s，解压缩16个线程也能60s以内，但是现在消费者耗时远高于60s，可以先把重点转移到消费者上了。

简单分析了现在的hash map，一共2.6e8个元素，mod是1e9+7，有2e8个是不冲突的，所以mod取这么大的情况下并不需要在链表这里做什么操作。但是同样的，内存需求也是很大的，直接的hash表大约占3G，后面链表存储的2.6e8个int64大约占4G。

换几个mod试试：（简单的读fq写fq）

|            |       |       | get map |         |      |
| ---------- | ----- | ----- | ------- | ------- | ---- |
| 1e9+7      | ～12G | 6400% | 27      | 169 174 | 175  |
| 73939133   | ～10G | 6400% | 40      | 512 529 | 529  |
| 1073807359 | ～13G | 6400% | 26      | 165 169 | 170  |
| 2000000011 | ～17G | 6400% | 30      | 171 176 | 176  |
|            |       |       |         |         |      |

好啊，没啥用感觉，毕竟1e9+7的时候就有80%左右的元素hash是没有冲突的，然后10%只冲突一次，以此类推。

然后做getPosition之前其实可以根据原来代码的逻辑先对umi做一下filter，如果没有通过那么getPosition也可以直接不做了。淦他要统计mMapToSlideRead这变量，，，，所以这种方式不能用。。。。。。GG

|                                             |      |               | producer(pugz) | comsumer(pigz) | total |
| ------------------------------------------- | ---- | ------------- | -------------- | -------------- | ----- |
| no more disk write and read，pugz 1，pigz 4 | ~60G | ～5000%/6400% | 183 95 100     | 187 189        | 190   |
| 👆 use 1073807359                            | ~60G | ～5000%/6400% | 180 85 91      | 183 183        | 185   |
|                                             |      |               |                |                |       |
|                                             |      |               |                |                |       |
|                                             |      |               |                |                |       |

//TODO 把barcode缩一下范围直接搞个数组来测测跑多久。

晚上跑完步突然听到大师兄问线程加速比咋样，emmm好问题。

简单测测

|                          |      |        | getmap/producer | comsumer | total |
| ------------------------ | ---- | ------ | --------------- | -------- | ----- |
| in fq，out fq，thread 64 | 14G  | 6400%  | 26/165          | 170      | 170   |
| in fq，out fq，thread 32 | 13G  | 3200%+ | 29/201          | 206      | 200   |
| in fq，out fq，thread 16 | 12G  | 1600%+ | 26/315          | 323      | 324   |
| in fq，out fq，thread 8  | 11G  | 800%+  | 26/582          | 597      | 590   |
| in fq，out fq，thread 4  | 11G  | 400%+  | 26/1146         | 1175     | 1150  |
| in fq，out fq，thread 2  |      |        | 30/2323         | 2384     | 2384  |
| in fq，out fq，thread 1  |      |        | 29/4046         | 4149     | 4149  |

按照之前qc使用c++ thread库实现P-C模型的经验来看，一般只要cpu跑起来了两倍的线程，速度就是两倍，但现在线程加速比不够好，初步猜测是伪共享或者remote numa访存效率低的原因，对于numa架构简单学习了一下：

https://zhuanlan.zhihu.com/p/62795773

简单说就是提高访问内存的效率，⬆️文中提到了内存需求很大，超过一个numa节点对应内存的时候频繁swap带来问题，现在代码里面的情况应该略有不同，是两个numa节点的core都用起来了，但是存储关键信息的hashmap只在某个节点的内存上，这样有一半core在访存的时候会比较慢。简单想到的解决办法就是在另个numa节点对应的内存里开一个hashmap的副本，那个节点访问自己的map。

编译了个软件看了看compute006的numa架构：

![server](/Users/ylf9811/Downloads/server.png)

## 0922

把postion换一下

## 0923

没换完，测一下no out，in fq，only socket0

|           | getmap | producer/comsumer      | total |      |
| --------- | ------ | ---------------------- | ----- | ---- |
| thread 32 | 30     | 187/192-30=157/162     | 192   |      |
| thread 16 | 27     | 275/282-27=248/255     | 282   |      |
| thread 8  | 26     | 490/503-26=464/477     | 503   |      |
| thread 4  | 26     | 942/966-26=916/940     | 966   |      |
| thread 2  | 27     | 1841/1889-27=1814/1862 | 1889  |      |
| thread 1  | 27     | 3648/3742-27=3621/3715 | 3742  |      |



|           | getmap | producer/comsumer      | total |      |
| --------- | ------ | ---------------------- | ----- | ---- |
| thread 32 | 27     | 196/201-27=169/174     | 201   |      |
| thread 16 | 27     | 313/321-27=286/294     | 321   |      |
| thread 8  | 27     | 567/582-27=540/555     | 582   |      |
| thread 4  | 27     | 1092/1120-27=1065/1093 | 1120  |      |
| thread 2  | 31     | 2384/2449-31=2353/2418 | 2449  |      |
| thread 1  |        |                        |       |      |



烦哦，这运行怎么忽快忽慢的。

pac

|                                     | get map | producer | consumer | total | time |
| ----------------------------------- | ------- | -------- | -------- | ----- | ---- |
| ./yrun.sh 64                        | 27      | 144      | 149      | 177   | 3m8  |
|                                     | 27      | 148      | 153      | 180   | 3m9  |
|                                     | 26      | 144      | 150      | 178   | 3m6  |
|                                     | 27      | 144      | 148      | 176   | 3m4  |
| ./yrun.sh 64 remove Positon         | 28      | 140      | 145      | 175   | 3m4  |
|                                     | 27      | 151      | 156      | 184   | 3m10 |
|                                     | 27      | 141      | 145      | 173   | 3m0  |
|                                     | 29      | 152      | 157      | 187   | 3m16 |
|                                     |         |          |          |       |      |
| ./yrun.sh 32                        | 26      | 171      | 177      | 203   | 3m28 |
|                                     | 27      | 169      | 174      | 201   | 3m29 |
|                                     | 27      | 171      | 176      | 204   | 3m32 |
|                                     | 26      | 173      | 178      | 207   | 3m29 |
|                                     |         |          |          |       |      |
| ./yrun.sh 32 remove Positon         | 21      | 152      | 157      | 178   |      |
|                                     | 21      | 153      | 158      | 180   |      |
|                                     | 21      | 150      | 155      | 176   |      |
|                                     | 21      | 151      | 155      | 177   |      |
|                                     | 21      | 153      | 158      | 179   |      |
|                                     | 21      | 143      | 148      | 169   |      |
|                                     |         |          |          |       |      |
| ./yrun.sh 32 remove Positon socket0 | 22      | 144      | 149      | 171   |      |
|                                     | 22      | 143      | 148      | 170   |      |
|                                     | 21      | 144      | 160      | 182   |      |
|                                     | 22      | 144      | 148      | 170   |      |
|                                     | 21      | 144      | 148      | 169   |      |
|                                     | 21      | 144      | 148      | 170   |      |
|                                     |         |          |          |       |      |
| ./yrun.sh 16                        | 26      | 293      | 301      | 329   | 5m30 |
|                                     | 28      | 287      | 295      | 327   | 5m28 |
|                                     | 28      | 288      | 296      | 324   | 5m27 |
|                                     | 26      | 288      | 296      | 323   | 5m28 |
|                                     |         |          |          |       |      |
| ./yrun.sh 16 remove Positon         |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
| /yrun.sh 16 remove Positon socket0  | 21      | 223      | 229      | 251   |      |
|                                     | 21      | 223      | 230      | 252   |      |
|                                     | 21      | 223      | 229      | 251   |      |
|                                     | 22      | 223      | 229      | 251   |      |
|                                     | 21      | 223      | 229      | 250   |      |
|                                     |         |          |          |       |      |
| ./yrun.sh 8 remove Positon          |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
| ./yrun.sh 8 remove Positon socket0  | 22      | 395      | 406      | 428   |      |
|                                     | 21      | 394      | 405      | 427   |      |
|                                     | 21      | 394      | 405      | 426   |      |
|                                     | 22      | 394      | 405      | 427   |      |
|                                     | 21      | 394      | 404      | 427   |      |
|                                     |         |          |          |       |      |
| ./yrun.sh 4 remove Positon          |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
| ./yrun.sh 4 remove Positon socket0  | 21      | 748      | 768      | 790   |      |
|                                     | 21      | 752      | 772      | 794   |      |
|                                     | 21      | 747      | 767      | 789   |      |
|                                     | 21      | 750      | 770      | 792   |      |
|                                     | 21      | 751      | 770      | 792   |      |
|                                     |         |          |          |       |      |
| ./yrun.sh 2 remove Positon          |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
|                                     |         |          |          |       |      |
| ./yrun.sh 2 remove Positon socket0  | 21      | 1463     | 1502     | 1523  |      |
|                                     | 22      | 1466     | 1504     | 1526  |      |
|                                     |         |          |          |       |      |

fat

|              | get map | producer | consumer | total | time  |
| ------------ | ------- | -------- | -------- | ----- | ----- |
| ./yrun.sh 40 | 40      | 174      | 180      | 220   | 3m40s |
|              | 34      | 169      | 175      | 210   | 3m31  |
|              | 35      | 169      | 174      | 209   | 3m29  |
|              | 33      | 170      | 176      | 209   | 3m30  |
|              |         |          |          | 210   |       |
|              |         |          |          | 214   |       |
|              |         |          |          |       |       |
| ./yrun.sh 20 | 34      | 317      | 327      | 363   | 6m2   |
|              | 34      | 314      | 323      | 357   | 5m58  |
|              | 36      | 319      | 329      | 366   | 6m6   |
|              | 35      | 322      | 331      | 367   | 6m7   |
|              | 34      | 298      | 307      | 341   | 5m42  |
|              | 33      | 306      | 315      | 349   | 5m49  |
|              |         |          |          | 352   |       |
| ./yrun.sh 10 |         |          |          |       |       |
|              |         |          |          |       |       |
|              |         |          |          |       |       |
|              |         |          |          |       |       |
|              |         |          |          |       |       |

```
rm -rf combine_read.fq && sleep 10 && time ../ST_BarcodeMap-0.0.1 --in DP8400016231TR_D1.barcodeToPos.h5 --in1 p1.fq --in2 p2.fq --out combine_read.fq --mismatch 2 --thread 32
```

## 0924 0925

一直在做测试

发现了惊天大秘密，006和003运行的不一样快，003要快一些。

|                                     | get map | producer | consumer | total | time         |
| ----------------------------------- | ------- | -------- | -------- | ----- | ------------ |
| ./yrun.sh 64 remove Positon         | 21      | 145      | 150      | 171   |              |
|                                     | 22      | 128      | 133      | 155   |              |
|                                     | 21      | 142      | 147      | 169   |              |
|                                     |         |          |          |       |              |
| ./yrun.sh 32 remove Positon         | 21      | 152      | 157      | 178   |              |
|                                     | 21      | 153      | 158      | 180   |              |
|                                     | 21      | 150      | 155      | 176   |              |
|                                     | 21      | 151      | 155      | 177   |              |
|                                     | 21      | 153      | 158      | 179   |              |
|                                     | 21      | 158      | 163      | 184   |              |
|                                     | 22      | 155      | 159      | 182   |              |
|                                     | 21      | 157      | 162      | 184   |              |
|                                     | 21      | 151      | 156      | 177   |              |
|                                     | 21      | 152      | 165      | 187   | 180*1.5=270  |
|                                     |         |          |          |       |              |
|                                     |         |          |          |       |              |
| ./yrun.sh 32 remove Positon socket0 | 22      | 144      | 149      | 171   |              |
|                                     | 22      | 143      | 148      | 170   |              |
|                                     | 21      | 144      | 160      | 182   |              |
|                                     | 22      | 144      | 148      | 170   |              |
|                                     | 21      | 144      | 148      | 169   |              |
|                                     | 21      | 144      | 148      | 170   |              |
|                                     | 21      | 143      | 148      | 169   |              |
|                                     | 21      | 144      | 148      | 171   |              |
|                                     | 21      | 143      | 148      | 170   |              |
|                                     | 21      | 144      | 148      | 178   |              |
|                                     | 21      | 144      | 148      | 170   |              |
|                                     | 21      | 143      | 148      | 170   | 170*1.47=250 |
|                                     |         |          |          |       |              |
| ./yrun.sh 16 remove Positon         | 22      | 242      | 249      | 271   |              |
|                                     | 21      | 241      | 248      | 270   |              |
|                                     | 21      | 243      | 250      | 271   |              |
|                                     | 21      | 242      | 249      | 271   |              |
|                                     | 21      | 243      | 249      | 271   | 270*1.81=490 |
|                                     |         |          |          |       |              |
| /yrun.sh 16 remove Positon socket0  | 21      | 223      | 229      | 251   |              |
|                                     | 21      | 223      | 230      | 252   |              |
|                                     | 21      | 223      | 229      | 251   |              |
|                                     | 22      | 223      | 229      | 251   |              |
|                                     | 21      | 223      | 229      | 250   |              |
|                                     | 21      | 223      | 230      | 251   |              |
|                                     | 21      | 223      | 230      | 251   |              |
|                                     | 21      | 223      | 230      | 251   |              |
|                                     | 21      | 223      | 230      | 256   |              |
|                                     | 21      | 223      | 230      | 252   | 252*1.69=427 |
|                                     |         |          |          |       |              |
| ./yrun.sh 8 remove Positon          | 22      | 453      | 465      | 488   |              |
|                                     | 21      | 456      | 469      | 490   |              |
|                                     | 21      | 456      | 468      | 490   |              |
|                                     | 21      | 456      | 468      | 490   |              |
|                                     | 21      | 454      | 466      | 487   |              |
|                                     |         |          |          |       |              |
| ./yrun.sh 8 remove Positon socket0  | 22      | 395      | 406      | 428   |              |
|                                     | 21      | 394      | 405      | 427   |              |
|                                     | 21      | 394      | 405      | 426   |              |
|                                     | 22      | 394      | 405      | 427   |              |
|                                     | 21      | 394      | 404      | 427   |              |
|                                     | 21      | 394      | 405      | 427   |              |
|                                     | 21      | 394      | 405      | 427   |              |
|                                     | 21      | 395      | 405      | 427   |              |
|                                     | 21      | 394      | 405      | 426   |              |
|                                     | 21      | 394      | 405      | 426   |              |
|                                     |         |          |          |       |              |
|                                     |         |          |          |       |              |
|                                     |         |          |          |       |              |
| ./yrun.sh 4 remove Positon          | 21      | 884      | 907      | 929   |              |
|                                     | 21      | 886      | 910      | 932   |              |
|                                     | 21      | 889      | 913      | 935   |              |
|                                     | 21      | 884      | 908      | 930   |              |
|                                     | 21      | 886      | 910      | 932   |              |
|                                     |         |          |          |       |              |
| ./yrun.sh 4 remove Positon socket0  | 21      | 748      | 768      | 790   |              |
|                                     | 21      | 752      | 772      | 794   |              |
|                                     | 21      | 747      | 767      | 789   |              |
|                                     | 21      | 750      | 770      | 792   |              |
|                                     | 21      | 751      | 770      | 792   |              |
|                                     | 21      | 750      | 770      | 791   |              |
|                                     | 21      | 750      | 770      | 791   |              |
|                                     | 21      | 750      | 770      | 791   |              |
|                                     | 21      | 750      | 770      | 792   |              |
|                                     | 21      | 750      | 770      | 791   |              |
|                                     |         |          |          |       |              |
| ./yrun.sh 2 remove Positon          | 21      | 1758     | 1804     | 1825  |              |
|                                     | 21      | 1756     | 1802     | 1824  |              |
|                                     |         |          |          |       |              |
|                                     |         |          |          |       |              |
|                                     |         |          |          |       |              |
| ./yrun.sh 2 remove Positon socket0  | 21      | 1463     | 1502     | 1523  |              |
|                                     | 22      | 1466     | 1504     | 1526  |              |
|                                     | 21      | 1467     | 1505     | 1527  |              |
|                                     | 21      | 1459     | 1497     | 1518  |              |
|                                     |         |          |          |       |              |
| ./yrun.sh 1 remove Positon          | 21      | 3033     | 3109     | 3131  |              |
|                                     |         |          |          |       |              |
| ./yrun.sh 1 remove Positon socket0  | 21      | 2887     | 2962     | 2984  |              |
|                                     |         |          |          |       |              |

## 0926

update 👆

经过简单的测试，指定单个numa节点效果要好一点，003比006要好一点，用Position还是int32，区别不大，暂时都用int32的版本。

好啊，经过简单的测试，虽然大师兄说的线程加速比没有看出什么猫腻来，但是确实单个numa节点要好一些，现在搞一个版本，每个numa节点有自己的map副本，这样就不存在远程numa访问的问题了。

简单试试，就是同时跑两个程序，每次32个线程，跑在不同的numa节点上；具体的，两个程序传一个参数--socketId，0或者1，然后生产者只把对应的块读进来放到队列里面交给消费者处理，写就暂时分别写两个文件。

|                                                | get map | producer                | consumer                      | total   | time |
| ---------------------------------------------- | ------- | ----------------------- | ----------------------------- | ------- | ---- |
| socket0 32 half data                           | 21      | 71                      | 75                            | 97      |      |
|                                                | 21      | 71                      | 75                            | 97      |      |
|                                                | 21      | 71                      | 75                            | 97      |      |
| socket1 32 half data                           | 21      | 70                      | 74                            | 96      |      |
|                                                |         |                         |                               |         |      |
|                                                |         |                         |                               |         |      |
| merge 👆                                        | 21/21   | 71/71                   | 75/75                         | 97/96   |      |
|                                                |         |                         |                               |         |      |
|                                                |         |                         |                               |         |      |
| in gz，out gz thread 64-1-4                    | 22      | 136 93/99               | 138/147                       | 169     |      |
|                                                |         |                         |                               |         |      |
| in gz，out gz thread 32-1-4  socket0 half data | 21      | 87 62/67                | 87/87                         | 109     |      |
|                                                |         |                         |                               |         |      |
| in gz，out gz thread 32-1-4  socket1 half data | 21      | 87 62/67                | 87/87                         | 109     |      |
|                                                |         |                         |                               |         |      |
| mpirun -n 1, in fq, out fq, thread 64          | 22      | 147                     | 152                           | 183     |      |
|                                                | 22      | 137                     | 142                           | 164     |      |
|                                                | 21      | 139                     | 144                           | 171     |      |
| mpirun -n 2, in fq, out fq, thread 32          | 22/22   | 68/68                   | 72/72(write:72/87)            | 95/110  |      |
|                                                | 22/22   | 68/68                   | 72/72(write:72/73)            | 94/95   |      |
| mpirun -n 2, in gz, out gz, thread 32          | 22/22   | 79/79(pugz:79/79 83/83) | 82/83(write:82/83 pigz:84/84) | 107/107 |      |
|                                                |         |                         |                               | 107/107 |      |
|                                                |         |                         |                               | 109/109 |      |
|                                                |         |                         |                               |         |      |

## 0928

试了试快速mod，没什么用，晚上尝试加布隆过滤器。

|                     | get map/bloom | producer | consumer | total |
| ------------------- | ------------- | -------- | -------- | ----- |
| thread 64，no mpi， | 26            | 143      | 148      | 175   |
|                     | 26            | 139      | 143      | 169   |
|                     | 25            | 142      | 146      | 173   |
| thread 64，4 hash   | 35            | 194      | 201      | 237   |
| thread 64，1 hash   | 30            |          |          | 193   |

```
1 hash
total_query_cnt:	156371927497
after_filter_query_cnt:	144998762505

4 hash
total_query_cnt:	156371927497
after_filter_query_cnt:	156099493240

total_query_cnt:	156377424777
after_filter_query_cnt:	2431340523
find_query_cnt:	145470356

new wang hash
total_query_cnt:	156377424777
after_filter_query_cnt:	917789027
find_query_cnt:	145470356

```

## 0930

好啊，今天必须要出点活了，现在的情况是简单试了bloomfilter，确实能把99%的查询的筛掉，但是用了三个hash函数，并且是stl的bitset作为数据结构可能都不太高效，准备替换成手写试试。

此外，为了更好的评价每个版本的性能指标，需要对各个模块计时，目前的策略是把原来逻辑里面的break都干掉，然后通过注释bloomfilter的代码来评价时间。

好啊，现在把misoverlap部分的break注释，测测时间和查询次数：

```
no misoverlap break
total_query_cnt:        157473409122
after_filter_query_cnt: 1010068985
find_query_cnt: 233394250


```

这样测没啥用啊感觉，主要还是测bf占多少时间，改改策略，一个版本不做bf，另一个做bf但是不用他的结果。

```
做bf：
42+197


不做bf：
41+78

```



好啊，现在手写一个bloomfilter，用uint64*来作为数据结构存储，因为很多hash函数都是uint64        -> uint32之类的，所以存储的话bit数大约是1<<32，也就是1<<26这么大的uint64数组，然后查询key的时候要对应转化为。。。

试过了没啥用。

确实能过滤很多很多，但是并没有快，甚至慢了10s（110--120）

然后试了试把原来的hashmap换一种hash方式，没啥用；试了试直接把hash数组开大4倍，没啥用。

然后试试二级索引、内存小一点的bf、挑0，3，6，9。。。位做筛选、

## 1003

01 02放假摸鱼。

垂死病中惊坐起，没有用最新的rabbitio，用的是rabbitqc中的，果然自己挖的坑。。。。

|                  | getmap | tot  | tot-getmap |
| ---------------- | ------ | ---- | ---------- |
| no process，32*2 | 28     | 45   | 17         |
| no process，64*1 | 28     | 61   | 33         |
| no process，32*1 | 28     | 67   | 39         |
| ReRabbitQC，48   | /      |      | 31         |
| RabbitQC，48     |        |      | 31         |
| no process，32*2 | 21     | 64   | 42         |

嘶，测了测感觉多的拷贝也没啥影响啊。先写写mpi合并输出的版本吧。

## 1004

👆说的版本，改之前时间大约24/28-111、25/28-109

改之后size不太对emmm，时间大约25/28-116、25/28-118、24/27-118

淦，他size又对了？？上午上课的时候痴呆了？还是错误浮现不出来了？

//TODO//TODO//TODO//TODO//TODO

用rabbitqc看一看输出的结果，

STD：<img src="/Users/ylf9811/Library/Application Support/typora-user-images/image-20211004190937957.png" alt="image-20211004190937957" style="zoom:50%;" />

now：<img src="/Users/ylf9811/Library/Application Support/typora-user-images/image-20211004191001061.png" alt="image-20211004191001061" style="zoom:50%;" />

好像真的复现不出来了。。。。。

暂时先把重点放在bf上吧。

一个numa节点：28-177、28-179、

in and out in shm、one numa node：28-170

老甘给的链接里有好几个map，简单写个测试性能的玩意：

```

#include <iostream>
#include <sparsehash/dense_hash_map>
#include <cstring>
#include <algorithm>
#include <sys/time.h>
#include <vector>
#include <random>
#include <unordered_map>

#define mm long long
//#define mm int

using google::dense_hash_map;      // namespace where class lives by default
using std::cout;
using std::endl;
using std::tr1::hash;  // or __gnu_cxx::hash, or maybe tr1::hash, depending on your OS
static const int mod = 1073807359;


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
    double t0 = MainGetTime();


    std::random_device rd;
    std::mt19937_64 gen(rd());

    std::uniform_int_distribution<mm> dis;


    std::vector<mm> G;
    std::vector<mm> P;
    int id;
    int cnt;
    int M = 2e8;
    int N = 1e9;

    for (int i = 0; i < M; i++) {
        G.push_back(dis(gen));
    }
    for (int i = 0; i < N; i++) {
        P.push_back(dis(gen));
    }
    printf("init cost %lf\n", MainGetTime() - t0);


    t0 = MainGetTime();
    dense_hash_map<mm, int> mps;

    mps.set_empty_key(NULL);
    id = 0;
    for (auto it:G) {
        id++;
        mps[it] = id;
    }
    printf("dense_hash_map insert cost %lf\n", MainGetTime() - t0);

    t0 = MainGetTime();
    cnt = 0;
    for (auto it:P) {
        if (mps.count(it))cnt++;
    }
    printf("dense_hash_map find %d\n", cnt);
    printf("dense_hash_map find cost %lf\n", MainGetTime() - t0);


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
    for (auto it:P) {
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

    t0 = MainGetTime();
    std::unordered_map<mm, int> mp2s;
    id = 0;
    for (auto it:G) {
        id++;
        mp2s[it] = id;
    }
    printf("stl unordered_map insert cost %lf\n", MainGetTime() - t0);


    t0 = MainGetTime();
    cnt = 0;
    for (auto it:P) {
        if (mp2s.count(it))cnt++;
    }
    printf("stl unordered_map find %d\n", cnt);
    printf("stl unordered_map find cost %lf\n", MainGetTime() - t0);

}

M=2e8 N=1e9 thread=1

[PAC20217111@compute003 ylf]$ ./hashTest
init cost 27.841362
dense_hash_map insert cost 18.319946
dense_hash_map find 166
dense_hash_map find cost 30.111485
gogo insert cost 7.738144
gogo find 166
gogo find cost 25.507501
stl unordered_map insert cost 70.406113
stl unordered_map find 166
stl unordered_map find cost 55.771334

👆 use find not count
[PAC20217111@compute003 ylf]$ ./hashTest
init cost 28.364878
dense_hash_map insert cost 15.537160
dense_hash_map find 174
dense_hash_map find cost 37.527437
sparse_hash_map insert cost 116.459556
sparse_hash_map find 174
sparse_hash_map find cost 175.995829
gogo insert cost 7.135087
gogo find 174
gogo find cost 24.261454
stl unordered_map insert cost 73.188713
stl unordered_map find 174
stl unordered_map find cost 61.126728


M=6e7 N=1e9 thread=1
[PAC20217111@compute003 ylf]$ ./hashTest
init cost 22.414524
dense_hash_map insert cost 4.985887
dense_hash_map find 55
dense_hash_map find cost 34.944564
sparse_hash_map insert cost 34.306814
sparse_hash_map find 55
sparse_hash_map find cost 53.169762
gogo insert cost 2.976034
gogo find 55
gogo find cost 21.728029
stl unordered_map insert cost 18.886106
stl unordered_map find 55
stl unordered_map find cost 63.939711

[PAC20217111@compute003 ylf]$ time ./hashTest
init cost 24.635170
google::dense_hash_map insert cost 4.901080
google::dense_hash_map find 63
google::dense_hash_map find cost 35.008083
google::sparse_hash_map insert cost 33.590294
google::sparse_hash_map find 63
google::sparse_hash_map find cost 51.789670
spp::sparse_hash_map insert cost 17.595097
spp::sparse_hash_map find 63
spp::sparse_hash_map find cost 43.207952
gogo insert cost 2.930788
gogo find 63
gogo find cost 21.738789
stl unordered_map insert cost 20.002673
stl unordered_map find 63
stl unordered_map find cost 64.700771


M=2e8 N=1e9 thread=32

[PAC20217111@compute003 ylf]$ ./hashTest
init cost 26.854261
dense_hash_map insert cost 18.031583
dense_hash_map find 182
dense_hash_map find cost 2.938416
gogo insert cost 7.980589
gogo find 182
gogo find cost 2.200198
stl unordered_map insert cost 69.550551
stl unordered_map find 182
stl unordered_map find cost 3.741968


M=2e8 N=1e10 thread=32
[PAC20217111@compute003 ylf]$ ./hashTest
init cost 319.879172
dense_hash_map insert cost 23.582168
dense_hash_map find 1639
dense_hash_map find cost 18.351356
gogo insert cost 7.843452
gogo find 1639
gogo find cost 14.136651
stl unordered_map insert cost 82.213892
stl unordered_map find 1639
stl unordered_map find cost 29.233407


M=2e8 N=1e9 thread=32
[PAC20217111@compute003 ylf]$ ./hashTest
init cost 7.282487
dense_hash_map insert cost 11.859019
dense_hash_map find 95475949
dense_hash_map find cost 2.299639
sparse_hash_map insert cost 74.191624
sparse_hash_map find 95475949
sparse_hash_map find cost 6.081009
gogo insert cost 4.767718
gogo find 95475949
gogo find cost 1.383346
stl unordered_map insert cost 68.503956
stl unordered_map find 95475949
stl unordered_map find cost 3.647662

```

## 1005

嘶，没啥用哎，感觉手写的hashmap应该就是最快的了，还是回到bf上弄弄。

找到之前两个进程merge输出的问题了，原来检测是否结束是p0的消费者结束了就set，可能p0的c们结束了，恰好队列空了（p1的mergeThread没来及塞数据），然后没输出全，现在多加了个flag就好了。

回退回比较简洁的commit 9a40937c573b6dc459d8e00d5df7930ba7d1815b。

|                           | getmap      | oo                      | tot  |                                                              |
| ------------------------- | ----------- | ----------------------- | ---- | ------------------------------------------------------------ |
| 👆提交 thread64            | 28          | 152                     | 180  |                                                              |
| 👆gcc8                     | 24          | 148                     | 173  |                                                              |
|                           | 29          | 135                     | 164  |                                                              |
|                           | 23          | 132                     | 155  |                                                              |
| 👆gcc4                     | 23          | 152                     | 175  |                                                              |
|                           | 26          |                         | 187  |                                                              |
| 👆icpc                     | 29          | 144                     | 173  |                                                              |
|                           | 24          | 140                     | 164  |                                                              |
|                           |             |                         |      |                                                              |
| old + bf*3 thread 64      | 46          | 146                     | 192  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	1079936634 |
|                           | 41          | 153                     | 194  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	1079936634 |
| old no bf thread 64       | 42          | 150                     | 193  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	0 |
|                           | 42          | 154                     | 197  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	0 |
|                           | 27（no bf） | 159                     | 187  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	0 |
|                           |             |                         |      |                                                              |
|                           |             |                         |      |                                                              |
| old no bf thread 32*2     | 26-28       | //77-//81               | 108  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	0 |
|                           | 26-27       | //77-//100              | 120  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	0 |
| old + bf：h1 thread 64    | 34          | 147                     | 181  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	10064669874 |
|                           | 33          | 145                     | 179  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	10064669874 |
|                           | 34          | 144                     | 179  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	10064669874 |
| old + bf：h0 thread 64    | 37          | 148                     | 186  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	9415169025 |
|                           | 34          | 152                     | 186  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	9415169025 |
|                           |             |                         |      |                                                              |
| old + bf：h3 thread 64    | 33          | 130                     | 163  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	9639439838 |
|                           | 33          | 145                     | 178  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	9639439838 |
|                           | 33          | 141                     | 174  | -                                                            |
|                           | 33          | 133                     | 166  | -                                                            |
|                           | 30          | 132                     | 162  | -                                                            |
| 👆 thread32 * 2            | 32-33       | 48/51/51-48/51/76？？？ | 110  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	9639439838 |
|                           |             | -------78               | 111  | -                                                            |
|                           |             | ---------54             | 84   |                                                              |
|                           |             | ---------56             | 88   |                                                              |
|                           | 31-33       | -------54               | 85   |                                                              |
| 👆mod -> dxh               |             |                         |      |                                                              |
| 32*2                      | 30-32       | --------90-92           | 123  | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	33984170386 |
|                           |             |                         |      |                                                              |
|                           |             |                         |      |                                                              |
|                           |             |                         |      |                                                              |
| 👆inline by hand           |             |                         |      |                                                              |
|                           |             |                         |      |                                                              |
| 👆 shm                     | 32-32       | 47/49/49-48/51/51       | 83   | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	9639439838 |
|                           |             | ---------51             | 82   |                                                              |
| old + bf：h1 thread 32 *2 | 31-34       | 62/65/65-59/63/63       | 97   | total_query_cnt:	156377424777<br/>after_filter_query_cnt:	10064669874 |
|                           |             | -----69                 | 101  |                                                              |

憨批了属于是，在测之前commit的时候突然就发现了一点点之前的bug：

<img src="/Users/ylf9811/Library/Application Support/typora-user-images/image-20211005234621491.png" alt="image-20211005234621491" style="zoom:50%;" />

改了之后最终只用前32后32位xor的简单hash效果最好，并且跑在两个numa节点上效果更好，不过似乎是卡在了写数据上，弄到shm就基本上是30+50=80了（不过测得都是fq，gz应该也差不多）。

## 1006

update👆

no bf单节点查询最好是150s，双节点是81s，使用hash3构造bf的话，单节点查询最好是130s，双节点是58s，比较奇怪的是为啥加速了两倍还多？？？但答案确实是对的。

最快的版本去fat节点测了一下，发现单节点和双节点时间几乎一样，难道是fat上的内存比较🐂没有远程numa访存的问题？好像也是有点用，效果不如pac机器上那么猛。

经过在fat节点上跑vtune的一些观察👇

thread 64 * 1，add bf：

![image-20211006171010632](/Users/ylf9811/Library/Application Support/typora-user-images/image-20211006171010632.png)

![image-20211006171033940](/Users/ylf9811/Library/Application Support/typora-user-images/image-20211006171033940.png)

![image-20211006171039232](/Users/ylf9811/Library/Application Support/typora-user-images/image-20211006171039232.png)

可以看到绝大部分时间还是来自hashMap的随机访存，bf能过滤掉90%的询问，并且不太占时间。

thread64 * 1，no bf：

![image-20211006171419243](/Users/ylf9811/Library/Application Support/typora-user-images/image-20211006171419243.png)

![image-20211006171424154](/Users/ylf9811/Library/Application Support/typora-user-images/image-20211006171424154.png)

然后zz说很多bf没有过滤掉的query（9e8/156e8）里面只有3e8能进for循环（hash[key]=-1）（实际上能查到的只有1e8），就把这个单独打出来：

![image-20211006172054447](/Users/ylf9811/Library/Application Support/typora-user-images/image-20211006172054447.png)

![image-20211006172204133](/Users/ylf9811/Library/Application Support/typora-user-images/image-20211006172204133.png)

![image-20211006172212027](/Users/ylf9811/Library/Application Support/typora-user-images/image-20211006172212027.png)

## 1007

展子哥终于安好了gcc8，试了试，在很久之前的纯hashMap的commit下比gcc4要快一点，比icpc也要快一丢丢，几乎一样。

今天实在是忍不了了，每次差1.5e11次太慢了。。。。尝试把20G的fq文件截取成2G，hashMap大小不变，这样应该和原来的计算模型差不多，简单改一下pac机器上的rabbitqc来做截取吧。

算了，太懒了，直接用head吧。

sp1 sp2 2G

|                | getmap | write done | tot  |      |      |
| -------------- | ------ | ---------- | ---- | ---- | ---- |
| gcc8 64 thread | 27     | 18         | 46   |      |      |
|                | 26     | 18         | 45   |      |      |
| gcc4           | 26     | 21         | 48   |      |      |
|                | 26     | 22         | 49   |      |      |
| icpc           | 28     | 20         | 49   |      |      |
|                | 26     | 19         | 46   |      |      |
|                |        |            |      |      |      |
|                |        |            |      |      |      |

```
small data ANS
total_reads:	20000000
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	20000000
mapped_reads:	14727394	73.64%
barcode_exactlyOverlap_reads:	11828527	59.14%
barcode_misOverlap_reads:	2898867	14.49%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.24%
Q20_bases_in_barcode:	96.66%
Q30_bases_in_barcode:	90.35%
Q10_bases_in_umi:	98.08%
Q20_bases_in_umi:	93.15%
Q30_bases_in_umi:	85.09%
umi_filter_reads:	411457	2.06%
umi_with_N_reads:	10	0.00%
umi_with_polyA_reads:	481	0.00%
umi_with_low_quality_base_reads:	410966	2.05%


2263572480
```

嘶

gcc8感觉是有点子用处的，而且晚上在fat上试了把hashMap的hash1改成hash3，没啥用。但是在pac机器上跑bf的版本的时候，编译出问题了，是向量化指令集有关的，但是似乎是pugz里面的文件报错的，现在并没有用，而且用的是mpiicc编译的，换成gcc8按理说不应该和之前不一样，简单加了include之后比之前的86快，现在79。。。。明天浮现一下这个问题仔细看看

## 1008

|                                                              | getmap | writedone | total |
| ------------------------------------------------------------ | ------ | --------- | ----- |
| big data fq，thread 32 * 2，has bf，gcc4，mpiicc             | 31-32  | 50-51     | 83    |
|                                                              |        |           | 83    |
|                                                              |        |           | 83    |
| remove include                                               |        |           | 82    |
| big data fq，thread 32 * 2，has bf，gcc8，mpiicc             | 32-32  | 49-49     | 81    |
|                                                              | 32-32  | 48-48     | 81    |
|                                                              | 32-32  | 48-48     | 80    |
| remove include                                               | ce     | ce        | ce    |
|                                                              |        |           |       |
| big data fq，thread 32 * 2，has bf，gcc4，mpigxx+mpigcc      | 31-32  | 50-51     | 83    |
|                                                              |        |           | 82    |
| big data fq，thread 32 * 2，has bf，gcc8，mpigxx+mpigcc（早上） | 30-31  | 47-47     | 78    |
| （下午）                                                     | 31-32  | 54-57     | 89    |
|                                                              |        |           | 93    |
|                                                              | 32-33  |           | 83    |
|                                                              |        |           | 93    |
| (晚上)                                                       | 31-31  | 46-46     | 77    |
|                                                              |        |           | 78    |
| big data fq，thread 32 * 2，has bf，gcc11，mpigxx+mpigcc（晚上）？？？？？ | 31-31  | 35-36     | 68    |
|                                                              |        |           | 67    |
|                                                              |        |           | 68    |
| big data fq，thread 32 * 2，has bf，gcc9，mpigxx+mpigcc（晚上） |        |           |       |
|                                                              |        |           |       |
| big data fq，thread 32 * 2，has bf，gcc7，mpigxx+mpigcc（晚上） | 31-31  | 48-48     | 79    |
|                                                              |        |           | 81    |
| big data fq，thread 32 * 2，has bf，gcc4，mpiicpc+mpiicc     |        |           |       |
|                                                              |        |           |       |
| big data fq，thread 32 * 2，has bf，gcc8，mpiicpc+mpiicc     |        |           |       |
|                                                              |        |           |       |
|                                                              |        |           |       |
| big data fq，thread 32 * 2，has bf，gcc4，mpicc+mpic++       | 31-32  | 72-72     | 104   |
|                                                              | 32-32  | 60-61     | 93    |
|                                                              |        |           | 97    |
| big data fq，thread 32 * 2，has bf，gcc8，mpicc+mpic++       | 31-32  | 59-60     | 92    |
|                                                              | 31-32  |           | 88    |
|                                                              |        |           | 87    |
|                                                              |        |           |       |

好啊，又浮现了一下昨天很奇怪的错误，果然开开gcc8pugz就没法过编译，需要加上向量化的头文件。而且gcc8似乎可能也许是快一丢丢。但是没道理，因为头文件影响的pugz现在根本没用，而且现在用的是mpiicc，mpiicc --version显示的是icc的version，mpicc才是gcc的，但是用mpicc（intel目录下的）直接编译会报错。

（淦 机器又慢起来了）

安好了mpich，在我的理解范围内试了试gcc4和gcc8的区别，gcc8稍快一点。

又试了试gcc11👆猛啊，具体为啥明天看看汇编。

## 1009

//TODO

早上打水的时候突然想到，能不能开个二级的bf，原来32位大小，现在两个16位大小的，判断ind=hash(barcode)是不是出现过，原来是bfInit[ind]==1，现在可以写成bfNow0[ind0]&&bfNow[ind1]，ind01分别是ind的高16位和低16位。

好啊，先吃饭。reboot一下。

好啊，现在读写内存的话，fq两个numa节点查询大约36s，一个numa节点查询大约50s，基本上没啥问题了，具体啥原因就让zz去看看吧，现在先单个numa节点把gz打开试试。

|                                | getmap | writer done | total | bf rate                                                      | work thread | pugz                                                         |
| ------------------------------ | ------ | ----------- | ----- | ------------------------------------------------------------ | ----------- | ------------------------------------------------------------ |
| 64*1，fq，shm                  | 31     | 49          | 80    | total_query_cnt:	156371927497<br/>after_filter_query_cnt:	9634745468 | ～64        |                                                              |
| 64*1，init gz，disk            | 31     | 176-176/495 | 529   | -                                                            | ～16        |                                                              |
| 64*1，init gz in，fq out，shm  | 33     | 174         | 208   |                                                              | ～16        |                                                              |
| 64*1，pugz8 gz in，fq out，shm | 41     | 103         | 144   |                                                              | ～30        | gunzip and push data to memory cost 30.2385<br/>gunzip and push data to memory cost 32.4541 |
|                                | 44     | 103         | 148   |                                                              | ～30        | gunzip and push data to memory cost 30.1984<br/>gunzip and push data to memory cost 32.4931 |
|                                |        |             |       |                                                              |             |                                                              |
| 32*2，init gz in，fq out，shm  | 31-31  | 172-172     | 204   |                                                              | ～7+7       |                                                              |
| 32*2，pugz1 gz in，fq out，shm | 33-36  | 75-78/      | 111   |                                                              | ～16+16     | gunzip and push data to memory cost 57<br/>gunzip and push data to memory cost 57<br/>gunzip and push data to memory cost 60<br/>gunzip and push data to memory cost 61 |
|                                | 32-32  | 71-71       | 104   |                                                              | ～16+16     | gunzip and push data to memory cost 56<br/>gunzip and push data to memory cost 56<br/>gunzip and push data to memory cost 60<br/>gunzip and push data to memory cost 60 |
| 32*2，pugz2 gz in，fq out，shm | 34-36  | 77-78       | 113   |                                                              | ～16+16     | gunzip and push data to memory cost 42<br/>gunzip and push data to memory cost 42<br/>gunzip and push data to memory cost 45<br/>gunzip and push data to memory cost 45 |
|                                | 34-34  | 74-74       | 109   |                                                              | ～16+16     | gunzip and push data to memory cost 41<br/>gunzip and push data to memory cost 42<br/>gunzip and push data to memory cost 44<br/>gunzip and push data to memory cost 44 |
|                                | 34-34  | 74-74       | 109   |                                                              | ～16+16     | gunzip and push data to memory cost 41<br/>gunzip and push data to memory cost 42<br/>gunzip and push data to memory cost 44<br/>gunzip and push data to memory cost 45<br/>gunzip and push data to memory cost 42<br/>gunzip and push data to memory cost 42<br/>gunzip and push data to memory cost 45<br/>gunzip and push data to memory cost 45 |
| 32*2，pugz4 gz in，fq out，shm | 34-35  | 77-77       | 112   |                                                              | ～15+15     | gunzip and push data to memory cost 30.443<br/>gunzip and push data to memory cost 30.5704<br/>gunzip and push data to memory cost 32.6724<br/>gunzip and push data to memory cost 32.9539 |
|                                | 34-34  | 75-76       | 110   |                                                              | ～15+15     | gunzip and push data to memory cost 30.4799<br/>gunzip and push data to memory cost 30.5536<br/>gunzip and push data to memory cost 32.8386<br/>gunzip and push data to memory cost 32.8623 |
| 32*2，pugz8 gz in，fq out，shm | 35-35  | 76-76       | 113   |                                                              | ～15+15     | gunzip and push data to memory cost 26.0721<br/>gunzip and push data to memory cost 26.3924<br/>gunzip and push data to memory cost 27.9054<br/>gunzip and push data to memory cost 28.2095 |
|                                | 35-35  | 77-77       | 112   |                                                              |             | gunzip and push data to memory cost 26.0642<br/>gunzip and push data to memory cost 26.3153<br/>gunzip and push data to memory cost 27.8736<br/>gunzip and push data to memory cost 28.1623 |
|                                |        |             |       |                                                              |             |                                                              |
| 64*1，pxgz gz 2+8，disk        | 34     |             |       |                                                              |             |                                                              |

有点子奇怪哦，为啥还能输出8个cost？？？？？

而且，多个pugz线程的话，就算他快到在构建hashMap期间就解压完了，后面query的时候也还是快不起来，只能用起15+15个线程，

## 1010

|                                 | getmap | write done | total                |                                                              |
| ------------------------------- | ------ | ---------- | -------------------- | ------------------------------------------------------------ |
| fq，32*2，gcc11                 | 25-27  | 34-35      | 63                   |                                                              |
| pugz8 gz，64*1，                | 30     | 60         |                      | gunzip and push data to memory cost 30<br/>gunzip and push data to memory cost 33 |
|                                 | 30     | 78         |                      | gunzip and push data to memory cost 29.7678<br/>gunzip and push data to memory cost 33 |
|                                 | 28     | 78         |                      | gunzip and push data to memory cost 31<br/>gunzip and push data to memory cost 33 |
| 👆optimize size_approx（晚上）   | 41     | 27         |                      | gunzip and push data to memory cost 31.9899<br/>gunzip and push data to memory cost 34.7715 |
|                                 | 38     | 29         |                      |                                                              |
| 👆32*2                           | 36-38  | 22-24      |                      | gunzip and push data to memory cost 28.8973<br/>gunzip and push data to memory cost 29.4034<br/>gunzip and push data to memory cost 30.8625<br/>gunzip and push data to memory cost 31.558 |
|                                 | 36-37  | 23-24      |                      | gunzip and push data to memory cost 28.9341<br/>gunzip and push data to memory cost 29.0582<br/>gunzip and push data to memory cost 31.0212<br/>gunzip and push data to memory cost 31.1593 |
| 👆all 32*2，pugz 8               | 36-37  | 39-40      | 76/～32 threads work | gunzip and push data to memory cost 28.8359<br/>gunzip and push data to memory cost 28.9144<br/>gunzip and push data to memory cost 30.8682<br/>gunzip and push data to memory cost 31.0669 |
|                                 |        |            | 76                   | gunzip and push data to memory cost 28.7751<br/>gunzip and push data to memory cost 28.9429<br/>gunzip and push data to memory cost 30.7982<br/>gunzip and push data to memory cost 31.1148 |
|                                 |        |            |                      |                                                              |
| 👆all 32*2，pugz 2               | 34-35  | 37-38      | 73（size wa！！！）  | gunzip and push data to memory cost 46<br/>gunzip and push data to memory cost 47<br/>gunzip and push data to memory cost 50<br/>gunzip and push data to memory cost 52 |
|                                 |        |            | 73                   | gunzip and push data to memory cost 47<br/>gunzip and push data to memory cost 48<br/>gunzip and push data to memory cost 51<br/>gunzip and push data to memory cost 52 |
|                                 |        |            | 73                   | gunzip and push data to memory cost 46<br/>gunzip and push data to memory cost 47<br/>gunzip and push data to memory cost 51<br/>gunzip and push data to memory cost 52 |
| 👆32*2，pugz 2，disk in，shm out | 36-36  |            | 75                   | gunzip and push data to memory cost 47<br/>gunzip and push data to memory cost 47<br/>gunzip and push data to memory cost 51<br/>gunzip and push data to memory cost 52 |
|                                 |        |            | 74                   |                                                              |

好啊，现在发现pugz的输入有点子问题，明明pugz很快，8thread只要20多s就完成了，全部放在了无限大的队列Q0里面，但是producer部分从Q0里面get数据的时候就有点慢了，需要不断的get，memcpy，delete，

![image-20211010210610712](/Users/ylf9811/Library/Application Support/typora-user-images/image-20211010210610712.png)

这个是fat节点上搞的测试，可以看到生产者的这个函数慢的一批，具体的是，里面的memcpy和无锁队列的巴拉巴拉，现在准备的解决方案是，先把比较耗时的size_approx拿到try后面，把两个size_approx合并。

update👆。

发现优化过size_approx之后，8个线程pugz就基本上又能供应起查询操作了。

试了两个线程pugz，效果和8差不多，但是有时候会输出文件的size不太对？

## 1011

昨晚的错误复现不出来了。淦

//TODO👆wa



|                                                              | getMap | writeDone | totalCost | pugzCost                                                     |
| ------------------------------------------------------------ | ------ | --------- | --------- | ------------------------------------------------------------ |
| rm -rf /dev/shm/*combine_read* && sleep 2 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in DP8400016231TR_D1.barcodeToPos.h5 --in1 V300091300_L03_read_1.fq.gz  --in2 V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq --mismatch 2 --thread 32 --usePugz --pugzThread 2 | 30-31  | 36-39     | 71        | gunzip and push data to memory cost 58<br/>gunzip and push data to memory cost 59<br/>gunzip and push data to memory cost 62<br/>gunzip and push data to memory cost 63 |
| rm -rf /dev/shm/*combine_read* && sleep 2 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in DP8400016231TR_D1.barcodeToPos.h5 --in1 V300091300_L03_read_1.fq.gz  --in2 V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq --mismatch 2 --thread 30 --usePugz --pugzThread 2 | 30-32  | 37-39     | 72        | gunzip and push data to memory cost 55<br/>gunzip and push data to memory cost 58<br/>gunzip and push data to memory cost 61<br/>gunzip and push data to memory cost 62 |
|                                                              |        |           | 71        |                                                              |
| rm -rf /dev/shm/*combine_read* && sleep 2 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in DP8400016231TR_D1.barcodeToPos.h5 --in1 V300091300_L03_read_1.fq.gz  --in2 V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq --mismatch 2 --thread 28 --usePugz --pugzThread 2 |        |           | 72        |                                                              |
| rm -rf /dev/shm/*combine_read* && sleep 2 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in DP8400016231TR_D1.barcodeToPos.h5 --in1 V300091300_L03_read_1.fq.gz  --in2 V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq --mismatch 2 --thread 26 --usePugz --pugzThread 2 |        |           | 75        |                                                              |
| rm -rf /dev/shm/*combine_read* && sleep 2 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in DP8400016231TR_D1.barcodeToPos.h5 --in1 V300091300_L03_read_1.fq.gz  --in2 V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --usePugz --pugzThread 2 |        |           | 77        |                                                              |
|                                                              |        |           | 76        |                                                              |
|                                                              |        |           |           |                                                              |
|                                                              |        |           |           |                                                              |

经过测试，pugz，发现双numa节点的话，每个节点28个查询线程就足够了，这样就腾出来4个线程给pigz，

```c++
				while (Q->try_dequeue(now) == 0) {
            if (Q->size_approx() == 0 && *wDone == 1) {
                ret = 0;
                break;
            }
            usleep(100);
        }
        if (Q->size_approx() == 0 && *wDone == 1) {
            ret = 0;
            break;
        }
```

瞅瞅瞅瞅，这些的啥玩意，原来输出文件的大小老是小一丢丢，可能while的时候一直==0，突然来了一个（也是最后一个，wDone置成1，接着-_-），接着dequeue出来了，然后接着if里面size==0，wDone==1，就GG了，最后一块数据就直接不要了。

## 1012

经过简单的测试，查询起码要40s，构建hashmap和bf要30s，pugz可以两个线程（40+s）被构建掩盖，基本上不影响（68-70），但是pigz要10个线程左右才能达到40s，也就是说不会影响查询，但是查询需要>28个线程才能达到40s左右，也就是说单节点压缩的话，只能空出来4个线程给pigz做压缩，显然是不太够的，双节点的话，27+5的结构似乎比较合理，先简单测试一下，不合并：

|                                                   | query | pigz    |      |
| ------------------------------------------------- | ----- | ------- | ---- |
| 单纯查询时间32*2                                  | 36/38 |         |      |
| 单纯查询时间30*2                                  | 37/38 |         |      |
| 单纯查询时间28*2                                  | 39/40 |         |      |
| 单纯查询时间26*2                                  | 41/42 |         |      |
|                                                   |       |         |      |
| --thread 28 --usePigz --pigzThread 2 --outGzSpilt | 40/40 | 102/121 |      |
| --thread 28 --usePigz --pigzThread 4 --outGzSpilt | 43/44 | 61/61   |      |
| --thread 28 --usePigz --pigzThread 6 --outGzSpilt | 42/44 | 44/44   |      |
|                                                   |       |         |      |
|                                                   |       |         |      |

好啊，不合并输出效果还是很好的，结果也是对的。

下面写写合并输出，emmmm，和大师兄讨论了一下，觉得两个mpi进程写同一个问价可能会很慢，可以再开一个mpi进程，它先启动一个接收线程收集p0p1发来的fq数据，然后在两个numa节点上分别启动6个线程做pigz。





## 1013

