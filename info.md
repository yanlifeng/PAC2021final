# PAC2021 final -- ChiFanBuZhangRou

TODOs

- [x] Add rabbitio
- [x] Use other map
- [ ] Parallel in load map
- [ ] Use secondary index
- [x] Use list-hash
- [ ] Use aili-code ？？？
- [ ] Optimize match algorithm
- [ ] Use gcc7 / gcc11
- [ ] Use icc
- [ ] Add pugz
- [ ] Read se not pe
- [ ] Use parallel zip when write
- [ ] Check todos 👇（//TODO）
- [ ] Why write .fq.gz size diff ?
- [ ] Write fq in order?

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
| real data 128/500 pugz thread2    |      | >6000%     | 165（120） | 170      | 196   |

把dataPool和dataQueue弄大了也没啥用。

表里面的生产者时间其实也没有太大的参考价值，因为可能消费者太慢把dataQueue堵住了，生产者在等待；可以发现init读gz的时候P和C的时间基本一样，这里应该是C在等P，P搞出来最后一块数据C处理完就都结束了，而读fq或者pugz的话P和C的时间有一点差距，应该是C太慢，queue满了，P中间等待了，最后P搞完队列里还有些数据，然后C处理完。

