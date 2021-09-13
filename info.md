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







