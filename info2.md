# PAC2021 final -- ChiFanBuZhangRou

TODOs

- [x] Add rabbitio
- [x] Use other map
- [ ] Parallel in load map----8*hashmap，omp merge
- [ ] ~~Use secondary index~~
- [x] Use list-hash
- [ ] ~~Use aili-code ？？？~~
- [x] Optimize match algorithm
- [x] Use gcc7 / gcc11
- [x] Use icc
- [x] Add pugz
- [ ] ~~Read se not pe~~
- [x] Use parallel zip when write
- [ ] Check todos 👇（//TODO）
- [x] Why write .fq.gz size diff ?
- [ ] ~~Write fq in order?~~
- [x] queue1 more bigger (128x128 -> 128x1024)
- [ ] change block size in pugz(32kb -> 4mb)
- [x] change block size in pigz
- [ ] change queue1 queue2 to dataPool to decrease new and delete operations
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





## 1013

pigz的速度跟不上啊，两个numa节点分别压缩的话，起码要10-12个线程才能跟上C的查询速度，而且C要至少28*2，如果两个节点分别配置28+6的话能到75s，不过这个是写两个压缩文件，现在要合成一个的话，有下面几个思路：

- A按照大师兄说的，利用sched在new每个thread的时候手动绑定到某个核心上，这样就能比较精确的控制，准备写｜28+6｜｜6+28｜，前28个thread在p0上，中间6+6在p1上，后面28在p2上，这样的分配希望能达到上面测的分别压缩的75s。但这样有几个问题，比如6+6能不能跨两个numa节点开到一个p上？
- B第二种思路还是｜28+6｜｜6+28｜，前28个thread在p0上，中间6+6在p1上，后面28在p2上这种方法，只不是使用intel mpi的运行时参数做线程的绑定。这个需要明确一点，mpi能不能制定某个p绑定在那几个thread上，如果指定了，那new thread的时候是不是能保证在指定的threadset中？
- C第三种比较简单，也比较有把握，暂时准备先写一版试试。即还是p0做全部的压缩，只不过一开始任务分配的时候，p1多拿一点，p0的负载少一点，分几个线程去做压缩。经过简单的计算，至少要10个thread来做pigz，那就只能有22个来查询，另外numa节点上可以32个拉满，负载大约是3:2，那么在生产者任务分配的时候稍微改一下就好了。

|                                                              | query | pigz    | getmap | total |
| ------------------------------------------------------------ | ----- | ------- | ------ | ----- |
| 单纯查询时间32*2                                             | 36/38 |         |        |       |
| 单纯查询时间30*2                                             | 37/38 |         |        |       |
| 单纯查询时间28*2                                             | 39/40 |         |        |       |
| 单纯查询时间26*2                                             | 41/42 |         |        |       |
|                                                              |       |         |        |       |
| --thread 28 --usePigz --pigzThread 2 --outGzSpilt            | 40/40 | 102/121 |        |       |
| --thread 28 --usePigz --pigzThread 4 --outGzSpilt            | 43/44 | 61/61   |        |       |
| --thread 28 --usePigz --pigzThread 6 --outGzSpilt            | 42/44 | 44/44   |        |       |
|                                                              |       |         |        |       |
| C --thread 24 --thread2 32 --usePigz --pigzThread 12         | 42/45 | 45      | 30/31  | 76    |
| C --thread 24 --thread2 32 --usePigz --pigzThread 12         |       |         |        |       |
| C --thread 22 --thread2 32 --usePigz --pigzThread 12 --usePugz --pugzThread 2 |       |         |        |       |
|                                                              |       |         |        |       |
| C --thread 24 --thread2 32 --usePigz --pigzThread 8 -z 1     | 41/41 | 41      | 31/31  | 73    |
|                                                              |       |         |        |       |
| --thread 24 --thread2 32 --usePigz --pigzThread 8 --usePugz --pugzThread 2 -z 1 | 43/43 | 43      | 34/34  | 77    |
|                                                              |       |         |        |       |

还稍微试了试把producer挪到构建hashmap部分，或者把pugz全都放到构建hashmap之后做，都没有啥提升，现在的版本就还是pugz两个线程，在程序一开始就开始解压，然后生产者和消费者（数据按照2:3划分，线程数22:32）等hashmap构建好之后一起开始，并且同时开启12个pigz线程压缩。

👆-z 1的话大约4:3然后d