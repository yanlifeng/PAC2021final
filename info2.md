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
- [x] mod all barcode to 1e9, use it dirctely, cal time
- [x] test G‘s map
- [ ] test 0 3 6 9
- [x] merge mip write part
- [x] check👆
- [x] add bloom filter
- [x] make bf bitset smaller or more small bitset to replace the big one
- [x] make bf bitset bigger
- [x] try bf with 3 bitset
- [x] calculate bf size
- [x] add mpi pugz
- [x] add mpi pigz
- [x] merge hashHead hashMap
- [ ] check asm to find out why gcc11 has a good perfermance
- [x] check pugz&producer and writer&pigz part
- [ ] inline query function by 👋
- [x] fix pugz size bug？？
- [x] make pigz not flush to disk
- [ ] vectorization in while misxx
- [ ] optmize chunk format paty
- [ ] optimize add position to name part
- [ ] use rabbit io
- [x] why sometimes gz version be killed(segmentation fault)
- [ ] optimize pugz(2 threads -> 4 threads)
- [x] optimize pigz write disk problem
- [x] why size-=1
- [ ] test other compile para
- [ ] change pugz compile para





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

## 1014

上午先把zz的分支给合过来，算了版本差两个，先不弄了。

下午改了改昨天不能omp的问题，是编译参数的问题，并且清空了一下内存，简单比较了一下👇的区别

```c++
new int[n] VS new int[n]()
```

把原来代码的这里改了一下，现在基本上-z 1能有72。

但是有一定的概率会被kill掉，并且不是内存占用太大的原因，就是写的段错误，这就蛋疼了啊，错误不好浮现。。。。。而且mpi不报行号GG

|                                  | gg times | total times | time  |
| -------------------------------- | -------- | ----------- | ----- |
| mpirun -n 2 gz in gz out 24/32/8 | 3        | 10          | 73/74 |
| mpirun -n 2 fq in fq out 24/32   | 0        | 22          | 66/67 |
| mpirun -n 2 fq in gz out 24/32/8 | 2        | 23          | 69/70 |
| mpirun -n 2 gz in fq out 24/32   | 0        | 23          | 70/71 |
|                                  |          |             |       |
|                                  |          |             |       |

先试试不用mpirun来几次看看会不会出现段错误。

👆还没试。。。

## 1016

把zz的代码合过来了，标准流程65s左右（shm），列举了目前几个todo，加了一些timer。

发现shm和parallel disk的时候pigz能差10s左右，目前猜测是同步写文件等待产生的，看看能不能搞成异步。

## 1017

早上早点来测测哪个节点性能好。

| node          | getmap（gz/fq） | total（gz/fq） |
| ------------- | --------------- | -------------- |
| compute012    | 16/15           | 59/51          |
| compute005    | 15/17           | 57/56          |
| compute007    | 18/18           | 62/56          |
| compute008    | 14/20           | 57/72          |
| compute010🌟🌟🌟 | 14/14           | 57/49          |
| compute014    | 15/17           | 59/55          |
| compute015    | 20/17           | 66/55          |
| compute004    | 17/18           | 61/55          |

compute004

```
gz
[PAC20217111@compute003 data]$ ./cc.sh
Process 1 of 2 ,processor name is compute003
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
Process 0 of 2 ,processor name is compute003
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now use pugz, 2 threads
now use pigz, 8 threads
now out name is /dev/shm/combine_read0.fq
mergeDone 0
mergeDone 0
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000176
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000206
read and close hdf5 cost 8.423941
new,init bf and hashMap cost 0.192147
read and close hdf5 cost 8.601177
new,init bf and hashMap cost 0.193374
parallel for cost 8.673084
###############load barcodeToPosition map finished, time used: 17 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 17.4457
mps
0 0 1 1 1
processor 1 get results done,cost 0.0009
parallel for cost 8.792366
###############load barcodeToPosition map finished, time used: 17 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 17.8035
mps
0 0 1 1 1
processor 0 get results done,cost 0.0007
============pigz init cost 0.000063
path is /dev/shm/combine_read0.fq
-----------------in pigz init cost 0.0000260
start compress...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in init cost 0.2173131
pugz0 done, cost 57
processor 0 producer get 1984 chunk done, cost 39.03800
pugz0 done, cost 58
processor 1 producer get 2974 chunk done, cost 40.85720
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 40.8191
pugz1 done, cost 60
processor 0 consumer cost 42.2536
processor 0 wSum is 22258049527
processor 0 writer done, cost 42.2539
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in do-while cost 42.0361378
pigz pSum is 22258049527
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in end cost 0.0574210
-----------------in pigz compress cost 42.3109009
compress done
-----------------in pigz finish cost 0.0000219
============pigz process0 cost 42.310956
============pigz process1 cost 0.000001
============pigz end cost 0.003845
pigz done, cost 42.3154
###processor 0 wait cost 3.660699
###processor 0 format cost 2.168895
###processor 0 new cost 0.152588
###processor 0 pe cost 33.696407
###processor 0 all cost 38.616104
watind barrier
pugz1 done, cost 61
processor 1 consumer cost 43.5574
processor 1 writer done, cost 43.5574
###processor 1 wait cost 1.980945
###processor 1 format cost 2.660482
###processor 1 new cost 0.139075
###processor 1 pe cost 37.347276
###processor 1 all cost 40.793619
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
1 work done
final and delete cost 43.2047
0 work done
process cost 61
my time : 61

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/V300091300_L03_read_1.fq.gz --in2 /dev/shm/V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 24 --thread2 32 --usePugz --pugzThread 2 --usePigz --pigzThread 8
spatialRNADrawMap, time used: 61 seconds
```

compute012

```
gz
[PAC20217111@compute012 data]$ rm -rf /dev/shm/*combine_read* && sleep 4 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/V300091300_L03_read_1.fq.gz --in2 /dev/shm/V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 24 --thread2 32 --usePugz --pugzThread 2 --usePigz --pigzThread 8
Process 0 of 2 ,processor name is compute012
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now use pugz, 2 threads
now use pigz, 8 threads
now out name is /dev/shm/combine_read0.fq
Process 1 of 2 ,processor name is compute012
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
mergeDone 0
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000178
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000171
read and close hdf5 cost 8.000669
read and close hdf5 cost 8.003879
new,init bf and hashMap cost 0.083650
new,init bf and hashMap cost 0.124764
parallel for cost 7.130631
###############load barcodeToPosition map finished, time used: 15 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 15.2585
mps
0 0 1 1 1
processor 1 get results done,cost 0.0010
parallel for cost 7.732617
###############load barcodeToPosition map finished, time used: 16 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 15.9042
mps
0 0 1 1 1
processor 0 get results done,cost 0.0008
============pigz init cost 0.000042
path is /dev/shm/combine_read0.fq
-----------------in pigz init cost 0.0000360
start compress...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in init cost 0.0071330
pugz0 done, cost 56
processor 0 producer get 1984 chunk done, cost 39.82136
pugz0 done, cost 56
processor 1 producer get 2974 chunk done, cost 40.82755
processor 0 get -1
processor 1 send data done, now send -1
processor 1 send -1 done
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 40.4785
pugz1 done, cost 59
processor 0 consumer cost 43.0058
processor 0 wSum is 22258049527
processor 0 writer done, cost 43.0061
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in do-while cost 42.9979169
pigz pSum is 22258049527
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in end cost 0.0575669
-----------------in pigz compress cost 43.0626380
compress done
-----------------in pigz finish cost 0.0000181
============pigz process0 cost 43.062700
============pigz process1 cost 0.000000
============pigz end cost 0.001551
pigz done, cost 43.0655
###processor 0 wait cost 4.844267
###processor 0 format cost 2.099919
###processor 0 new cost 0.117409
###processor 0 pe cost 33.044374
###processor 0 all cost 38.837631
watind barrier
pugz1 done, cost 59
processor 1 consumer cost 44.0098
processor 1 writer done, cost 44.0098
###processor 1 wait cost 3.184241
###processor 1 format cost 2.292191
###processor 1 new cost 0.147797
###processor 1 pe cost 35.804281
###processor 1 all cost 40.548262
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
1 work done
final and delete cost 43.3664
0 work done
process cost 59
my time : 59

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/V300091300_L03_read_1.fq.gz --in2 /dev/shm/V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 24 --thread2 32 --usePugz --pugzThread 2 --usePigz --pigzThread 8
spatialRNADrawMap, time used: 59 seconds


fq
[PAC20217111@compute012 data]$ rm -rf /dev/shm/*combine_read* && sleep 4 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
Process 1 of 2 ,processor name is compute012
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
Process 0 of 2 ,processor name is compute012
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now out name is /dev/shm/combine_read0.fq
mergeDone 0
###############load barcodeToPosition map begin...
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000187
new and hdf5 pre cost 0.000161
read and close hdf5 cost 7.885903
read and close hdf5 cost 7.885263
new,init bf and hashMap cost 0.074352
new,init bf and hashMap cost 0.106234
parallel for cost 6.569177
###############load barcodeToPosition map finished, time used: 14 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 14.5725
mps
0 0 1 1 1
processor 1 get results done,cost 0.0007
parallel for cost 7.186687
###############load barcodeToPosition map finished, time used: 15 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 15.2199
mps
0 0 1 1 1
processor 0 get results done,cost 0.0006
processor 0 producer get 1984 chunk done, cost 28.39740
processor 0 consumer cost 30.4706
processor 1 producer get 2974 chunk done, cost 34.79901
processor 1 consumer cost 36.5190
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 35.8693
processor 1 writer done, cost 36.5194
###processor 1 wait cost 0.591279
###processor 1 format cost 2.505424
###processor 1 new cost 0.143143
###processor 1 pe cost 34.459930
###processor 1 all cost 36.494834
watind barrier
processor 0 writer done, cost 35.8717
###processor 0 wait cost 0.351024
###processor 0 format cost 2.103566
###processor 0 new cost 0.102690
###processor 0 pe cost 28.572079
###processor 0 all cost 30.444797
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
1 work done
final and delete cost 35.8724
0 work done
process cost 51
my time : 51

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
spatialRNADrawMap, time used: 51 seconds
```

compute005

```
gz
[PAC20217111@compute005 data]$ rm -rf /dev/shm/*combine_read* && sleep 4 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/V300091300_L03_read_1.fq.gz --in2 /dev/shm/V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 24 --thread2 32 --usePugz --pugzThread 2 --usePigz --pigzThread 8
Process 0 of 2 ,processor name is compute005
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now use pugz, 2 threads
now use pigz, 8 threads
now out name is /dev/shm/combine_read0.fq
Process 1 of 2 ,processor name is compute005
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
mergeDone 0
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000197
new and hdf5 pre cost 0.000178
read and close hdf5 cost 7.983673
read and close hdf5 cost 8.008758
new,init bf and hashMap cost 0.085322
new,init bf and hashMap cost 0.083203
parallel for cost 6.403619
###############load barcodeToPosition map finished, time used: 14 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 14.5155
mps
0 0 1 1 1
processor 1 get results done,cost 0.0010
parallel for cost 6.922235
###############load barcodeToPosition map finished, time used: 15 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 15.0574
mps
0 0 1 1 1
processor 0 get results done,cost 0.0006
============pigz init cost 0.000053
path is /dev/shm/combine_read0.fq
-----------------in pigz init cost 0.0000260
start compress...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in init cost 0.0086870
pugz0 done, cost 54
processor 0 producer get 1984 chunk done, cost 38.92601
pugz0 done, cost 54
processor 1 producer get 2974 chunk done, cost 40.00893
processor 0 get -1
processor 1 send data done, now send -1
processor 1 send -1 done
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 39.7324
pugz1 done, cost 57
processor 0 consumer cost 42.1150
processor 0 wSum is 22258049527
processor 0 writer done, cost 42.1153
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in do-while cost 42.1054239
pigz pSum is 22258049527
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in end cost 0.0576129
-----------------in pigz compress cost 42.1717460
compress done
-----------------in pigz finish cost 0.0000200
============pigz process0 cost 42.171801
============pigz process1 cost 0.000000
============pigz end cost 0.001375
pigz done, cost 42.1744
###processor 0 wait cost 7.148757
###processor 0 format cost 2.061954
###processor 0 new cost 0.237257
###processor 0 pe cost 29.735631
###processor 0 all cost 37.205092
watind barrier
pugz1 done, cost 58
processor 1 consumer cost 43.1822
processor 1 writer done, cost 43.1822
###processor 1 wait cost 4.573623
###processor 1 format cost 2.171423
###processor 1 new cost 0.174380
###processor 1 pe cost 33.250231
###processor 1 all cost 39.131708
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
final and delete cost 42.6425
0 work done
process cost 58
my time : 58

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/V300091300_L03_read_1.fq.gz --in2 /dev/shm/V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 24 --thread2 32 --usePugz --pugzThread 2 --usePigz --pigzThread 8
spatialRNADrawMap, time used: 57 seconds
1 work done

fq

[PAC20217111@compute005 data]$ rm -rf /dev/shm/*combine_read* && sleep 4 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
Process 1 of 2 ,processor name is compute005
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
Process 0 of 2 ,processor name is compute005
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now out name is /dev/shm/combine_read0.fq
mergeDone 0
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000173
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000170
read and close hdf5 cost 7.919240
new,init bf and hashMap cost 0.077104
read and close hdf5 cost 8.228332
new,init bf and hashMap cost 0.154349
parallel for cost 6.722351
###############load barcodeToPosition map finished, time used: 15 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 14.7607
mps
0 0 1 1 1
processor 0 get results done,cost 0.0006
parallel for cost 8.281942
###############load barcodeToPosition map finished, time used: 17 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 16.9024
mps
0 0 1 1 1
processor 1 get results done,cost 0.0008
processor 0 producer get 1984 chunk done, cost 28.29557
processor 0 consumer cost 30.3705
processor 1 producer get 2974 chunk done, cost 36.79365
processor 1 consumer cost 38.5923
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
processor 1 writer done, cost 38.5928
merge done, cost 40.7315
###processor 1 wait cost 0.866960
###processor 1 format cost 2.659929
###processor 1 new cost 0.145838
###processor 1 pe cost 36.457780
###processor 1 all cost 38.540482
watind barrier
processor 0 writer done, cost 40.7337
###processor 0 wait cost 0.431712
###processor 0 format cost 2.185818
###processor 0 new cost 0.105282
###processor 0 pe cost 28.637534
###processor 0 all cost 30.365396
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
final and delete cost 40.7344
0 work done
process cost 55
my time : 55

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
spatialRNADrawMap, time used: 56 seconds
1 work done
```

compute007

```
gz
[PAC20217111@compute007 data]$ rm -rf /dev/shm/*combine_read* && sleep 4 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/V300091300_L03_read_1.fq.gz --in2 /dev/shm/V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 24 --thread2 32 --usePugz --pugzThread 2 --usePigz --pigzThread 8
Process 0 of 2 ,processor name is compute007
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now use pugz, 2 threads
now use pigz, 8 threads
now out name is /dev/shm/combine_read0.fq
Process 1 of 2 ,processor name is compute007
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
mergeDone 0
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000197
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000183
read and close hdf5 cost 8.470440
read and close hdf5 cost 8.471711
new,init bf and hashMap cost 0.183665
new,init bf and hashMap cost 0.182481
parallel for cost 8.561019
parallel for cost 8.683377
###############load barcodeToPosition map finished, time used: 18 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 17.4542
mps
0 0 1 1 1
processor 1 get results done,cost 0.0009
###############load barcodeToPosition map finished, time used: 18 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 17.5714
mps
0 0 1 1 1
processor 0 get results done,cost 0.0007
============pigz init cost 0.000045
path is /dev/shm/combine_read0.fq
-----------------in pigz init cost 0.0000179
start compress...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in init cost 0.4151361
pugz0 done, cost 58
processor 0 producer get 1984 chunk done, cost 40.98045
pugz0 done, cost 59
processor 1 producer get 2974 chunk done, cost 41.18140
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 41.3969
pugz1 done, cost 60
processor 1 consumer cost 42.7595
processor 1 writer done, cost 42.7595
###processor 1 wait cost 2.118826
###processor 1 format cost 2.390167
###processor 1 new cost 0.142766
###processor 1 pe cost 37.054794
###processor 1 all cost 40.886889
watind barrier
pugz1 done, cost 62
processor 0 consumer cost 44.1931
processor 0 wSum is 22258049527
processor 0 writer done, cost 44.1933
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in do-while cost 43.7775738
pigz pSum is 22258049527
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in end cost 0.0574570
-----------------in pigz compress cost 44.2502050
compress done
-----------------in pigz finish cost 0.0000300
============pigz process0 cost 44.250261
============pigz process1 cost 0.000000
============pigz end cost 0.008550
pigz done, cost 44.2595
###processor 0 wait cost 4.177049
###processor 0 format cost 2.188567
###processor 0 new cost 0.121327
###processor 0 pe cost 35.151993
###processor 0 all cost 40.169655
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
1 work done
final and delete cost 44.2661
0 work done
process cost 62
my time : 62

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/V300091300_L03_read_1.fq.gz --in2 /dev/shm/V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 24 --thread2 32 --usePugz --pugzThread 2 --usePigz --pigzThread 8
spatialRNADrawMap, time used: 62 seconds

fq
[PAC20217111@compute007 data]$ rm -rf /dev/shm/*combine_read* && sleep 4 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
Process 0 of 2 ,processor name is compute007
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now out name is /dev/shm/combine_read0.fq
mergeDone 0
Process 1 of 2 ,processor name is compute007
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000180
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000161
read and close hdf5 cost 8.214679
read and close hdf5 cost 8.253049
new,init bf and hashMap cost 0.160793
new,init bf and hashMap cost 0.156819
parallel for cost 8.431186
###############load barcodeToPosition map finished, time used: 17 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 17.0412
mps
0 0 1 1 1
processor 1 get results done,cost 0.0007
parallel for cost 8.627693
###############load barcodeToPosition map finished, time used: 18 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 17.2837
mps
0 0 1 1 1
processor 0 get results done,cost 0.0005
processor 0 producer get 1984 chunk done, cost 29.91204
processor 0 consumer cost 32.0669
processor 1 producer get 2974 chunk done, cost 36.58900
processor 1 consumer cost 38.3471
processor 0 get -1
processor 1 send data done, now send -1
processor 1 send -1 done
processor 1 writer done, cost 38.3476
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 38.1068
###processor 1 wait cost 0.997855
###processor 1 format cost 2.628181
###processor 1 new cost 0.126705
###processor 1 pe cost 36.203278
###processor 1 all cost 38.328640
watind barrier
processor 0 writer done, cost 38.1086
###processor 0 wait cost 0.535733
###processor 0 format cost 2.170028
###processor 0 new cost 0.105982
###processor 0 pe cost 30.203140
###processor 0 all cost 32.061972
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
1 work done
final and delete cost 38.1154
0 work done
process cost 55
my time : 55

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
spatialRNADrawMap, time used: 56 seconds

```

compute008

```
gz
[PAC20217111@compute008 data]$ ./cc.sh
Process 1 of 2 ,processor name is compute008
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
Process 0 of 2 ,processor name is compute008
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now use pugz, 2 threads
now use pigz, 8 threads
now out name is /dev/shm/combine_read0.fq
mergeDone 0
mergeDone 0
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000182
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000187
read and close hdf5 cost 7.991441
read and close hdf5 cost 7.996402
new,init bf and hashMap cost 0.084762
new,init bf and hashMap cost 0.086976
parallel for cost 5.901750
###############load barcodeToPosition map finished, time used: 14 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 14.0212
mps
0 0 1 1 1
processor 1 get results done,cost 0.0008
parallel for cost 5.931086
###############load barcodeToPosition map finished, time used: 14 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 14.0604
mps
0 0 1 1 1
processor 0 get results done,cost 0.0008
============pigz init cost 0.000040
path is /dev/shm/combine_read0.fq
-----------------in pigz init cost 0.0000200
start compress...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in init cost 0.4183841
pugz0 done, cost 52
processor 0 producer get 1984 chunk done, cost 38.48731
pugz0 done, cost 54
processor 1 producer get 2974 chunk done, cost 40.20071
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 40.4317
pugz1 done, cost 56
processor 0 consumer cost 41.6690
processor 0 wSum is 22258049527
processor 0 writer done, cost 41.6692
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in do-while cost 41.2496870
pigz pSum is 22258049527
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in end cost 0.0576541
-----------------in pigz compress cost 41.7257578
compress done
-----------------in pigz finish cost 0.0000229
============pigz process0 cost 41.725810
============pigz process1 cost 0.000000
============pigz end cost 0.000633
pigz done, cost 41.7286
###processor 0 wait cost 9.268026
###processor 0 format cost 1.994989
###processor 0 new cost 0.131142
###processor 0 pe cost 26.863962
###processor 0 all cost 36.672822
watind barrier
pugz1 done, cost 57
processor 1 consumer cost 43.3802
processor 1 writer done, cost 43.3802
###processor 1 wait cost 6.649961
###processor 1 format cost 2.229878
###processor 1 new cost 0.127270
###processor 1 pe cost 32.547339
###processor 1 all cost 39.204861
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
1 work done
final and delete cost 43.3385
0 work done
process cost 57
my time : 57

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/V300091300_L03_read_1.fq.gz --in2 /dev/shm/V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 24 --thread2 32 --usePugz --pugzThread 2 --usePigz --pigzThread 8
spatialRNADrawMap, time used: 57 seconds

fq
[PAC20217111@compute008 data]$ rm -rf /dev/shm/*combine_read* && sleep 4 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
Process 1 of 2 ,processor name is compute008
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
Process 0 of 2 ,processor name is compute008
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now out name is /dev/shm/combine_read0.fq
mergeDone 0
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000164
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000176
read and close hdf5 cost 7.880119
read and close hdf5 cost 7.883158
new,init bf and hashMap cost 0.096953
new,init bf and hashMap cost 0.220151
parallel for cost 6.454005
###############load barcodeToPosition map finished, time used: 14 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 14.4767
mps
0 0 1 1 1
processor 0 get results done,cost 0.0006
parallel for cost 11.871690
###############load barcodeToPosition map finished, time used: 20 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 20.0291
mps
0 0 1 1 1
processor 1 get results done,cost 0.0008
processor 0 producer get 1984 chunk done, cost 30.01932
processor 0 consumer cost 32.3746
processor 1 producer get 2974 chunk done, cost 49.91949
processor 1 consumer cost 52.2389
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 57.7887
processor 1 writer done, cost 52.2397
###processor 1 wait cost 0.759143
###processor 1 format cost 3.145860
###processor 1 new cost 0.147949
###processor 1 pe cost 50.049358
###processor 1 all cost 52.227792
watind barrier
processor 0 writer done, cost 57.7908
###processor 0 wait cost 0.337761
###processor 0 format cost 1.962817
###processor 0 new cost 0.096252
###processor 0 pe cost 30.390674
###processor 0 all cost 32.344887
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
1 work done
final and delete cost 57.7914
0 work done
process cost 72
my time : 72

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
spatialRNADrawMap, time used: 72 seconds
```

compute010

```
gz
[PAC20217111@compute010 data]$ ./cc.sh
Process 0 of 2 ,processor name is compute010
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now use pugz, 2 threads
now use pigz, 8 threads
now out name is /dev/shm/combine_read0.fq
Process 1 of 2 ,processor name is compute010
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
mergeDone 0
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000185
new and hdf5 pre cost 0.000173
read and close hdf5 cost 8.030203
read and close hdf5 cost 8.036187
new,init bf and hashMap cost 0.086633
new,init bf and hashMap cost 0.086035
parallel for cost 5.978657
###############load barcodeToPosition map finished, time used: 14 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 14.1431
mps
0 0 1 1 1
processor 0 get results done,cost 0.0007
============pigz init cost 0.000041
path is /dev/shm/combine_read0.fq
-----------------in pigz init cost 0.0000188
start compress...
parallel for cost 6.062790
###############load barcodeToPosition map finished, time used: 14 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 14.2224
mps
0 0 1 1 1
processor 1 get results done,cost 0.0008
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in init cost 0.4225070
pugz0 done, cost 53
processor 0 producer get 1984 chunk done, cost 38.73215
pugz0 done, cost 54
processor 1 producer get 2974 chunk done, cost 40.08248
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 40.4411
pugz1 done, cost 56
processor 0 consumer cost 41.9231
processor 0 wSum is 22258049527
processor 0 writer done, cost 41.9233
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in do-while cost 41.4996631
pigz pSum is 22258049527
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in end cost 0.0574100
-----------------in pigz compress cost 41.9796119
compress done
-----------------in pigz finish cost 0.0000222
============pigz process0 cost 41.979665
============pigz process1 cost 0.000000
============pigz end cost 0.000688
pigz done, cost 41.9816
###processor 0 wait cost 8.337979
###processor 0 format cost 2.000743
###processor 0 new cost 0.122997
###processor 0 pe cost 27.812561
###processor 0 all cost 37.014950
watind barrier
pugz1 done, cost 57
processor 1 consumer cost 43.2641
processor 1 writer done, cost 43.2641
###processor 1 wait cost 6.034956
###processor 1 format cost 2.154869
###processor 1 new cost 0.143006
###processor 1 pe cost 32.411165
###processor 1 all cost 39.386309
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
1 work done
final and delete cost 43.3448
0 work done
process cost 57
my time : 57

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/V300091300_L03_read_1.fq.gz --in2 /dev/shm/V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 24 --thread2 32 --usePugz --pugzThread 2 --usePigz --pigzThread 8
spatialRNADrawMap, time used: 57 seconds

fq
[PAC20217111@compute010 data]$ rm -rf /dev/shm/*combine_read* && sleep 4 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
Process 1 of 2 ,processor name is compute010
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
Process 0 of 2 ,processor name is compute010
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now out name is /dev/shm/combine_read0.fq
mergeDone 0
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000191
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000166
read and close hdf5 cost 7.895348
read and close hdf5 cost 7.933743
new,init bf and hashMap cost 0.076202
new,init bf and hashMap cost 0.100627
parallel for cost 5.828639
###############load barcodeToPosition map finished, time used: 14 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 13.8427
mps
0 0 1 1 1
processor 0 get results done,cost 0.0006
parallel for cost 5.904521
###############load barcodeToPosition map finished, time used: 14 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 13.9819
mps
0 0 1 1 1
processor 1 get results done,cost 0.0008
processor 0 producer get 1984 chunk done, cost 26.98687
processor 0 consumer cost 29.0251
processor 1 producer get 2974 chunk done, cost 33.38385
processor 1 consumer cost 35.0239
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
processor 1 writer done, cost 35.0244
merge done, cost 35.1607
###processor 1 wait cost 0.498956
###processor 1 format cost 2.409965
###processor 1 new cost 0.135312
###processor 1 pe cost 32.886512
###processor 1 all cost 35.006682
watind barrier
processor 0 writer done, cost 35.1627
###processor 0 wait cost 0.319696
###processor 0 format cost 1.939143
###processor 0 new cost 0.101153
###processor 0 pe cost 27.126623
###processor 0 all cost 29.015084
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
1 work done
final and delete cost 35.1633
0 work done
process cost 49
my time : 49

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
spatialRNADrawMap, time used: 49 seconds
```

compute013

```
gz
[PAC20217111@compute014 data]$ ./cc.sh
Process 0 of 2 ,processor name is compute014
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now use pugz, 2 threads
now use pigz, 8 threads
now out name is /dev/shm/combine_read0.fq
Process 1 of 2 ,processor name is compute014
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
mergeDone 0
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000177
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000186
read and close hdf5 cost 8.012235
read and close hdf5 cost 8.019614
new,init bf and hashMap cost 0.083944
new,init bf and hashMap cost 0.083605
parallel for cost 6.318918
###############load barcodeToPosition map finished, time used: 15 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 14.4692
mps
0 0 1 1 1
processor 1 get results done,cost 0.0010
parallel for cost 6.886917
###############load barcodeToPosition map finished, time used: 15 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 15.0262
mps
0 0 1 1 1
processor 0 get results done,cost 0.0006
============pigz init cost 0.000071
path is /dev/shm/combine_read0.fq
-----------------in pigz init cost 0.0000219
start compress...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in init cost 0.0056272
pugz0 done, cost 54
processor 0 producer get 1984 chunk done, cost 39.28690
pugz0 done, cost 55
processor 1 producer get 2974 chunk done, cost 40.59858
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 40.3253
pugz1 done, cost 57
processor 0 consumer cost 42.4939
processor 0 wSum is 22258049527
processor 0 writer done, cost 42.4942
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in do-while cost 42.4874170
pigz pSum is 22258049527
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in end cost 0.0573978
-----------------in pigz compress cost 42.5504630
compress done
-----------------in pigz finish cost 0.0000231
============pigz process0 cost 42.550515
============pigz process1 cost 0.000000
============pigz end cost 0.001709
pigz done, cost 42.5535
###processor 0 wait cost 6.317080
###processor 0 format cost 2.096271
###processor 0 new cost 0.167590
###processor 0 pe cost 30.392684
###processor 0 all cost 37.632100
watind barrier
pugz1 done, cost 58
processor 1 consumer cost 43.7862
processor 1 writer done, cost 43.7862
###processor 1 wait cost 5.272305
###processor 1 format cost 2.226142
###processor 1 new cost 0.137952
###processor 1 pe cost 33.587350
###processor 1 all cost 39.878051
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
1 work done
final and delete cost 43.2315
0 work done
process cost 58
my time : 58

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/V300091300_L03_read_1.fq.gz --in2 /dev/shm/V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 24 --thread2 32 --usePugz --pugzThread 2 --usePigz --pigzThread 8
spatialRNADrawMap, time used: 59 seconds

fq
[PAC20217111@compute014 data]$ rm -rf /dev/shm/*combine_read* && sleep 4 && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
Process 0 of 2 ,processor name is compute014
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now out name is /dev/shm/combine_read0.fq
mergeDone 0
Process 1 of 2 ,processor name is compute014
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000163
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000174
read and close hdf5 cost 8.197635
read and close hdf5 cost 8.246239
new,init bf and hashMap cost 0.156594
new,init bf and hashMap cost 0.159096
parallel for cost 8.117793
parallel for cost 8.200136
###############load barcodeToPosition map finished, time used: 17 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 16.7321
mps
0 0 1 1 1
processor 0 get results done,cost 0.0006
###############load barcodeToPosition map finished, time used: 17 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 16.7839
mps
0 0 1 1 1
processor 1 get results done,cost 0.0006
processor 0 producer get 1984 chunk done, cost 29.69789
processor 0 consumer cost 31.9075
processor 1 producer get 2974 chunk done, cost 36.64232
processor 1 consumer cost 38.4106
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
processor 1 writer done, cost 38.4112
merge done, cost 38.4638
###processor 1 wait cost 0.742487
###processor 1 format cost 2.562079
###processor 1 new cost 0.155219
###processor 1 pe cost 36.199150
###processor 1 all cost 38.397568
watind barrier
processor 0 writer done, cost 38.4665
###processor 0 wait cost 0.578184
###processor 0 format cost 2.138513
###processor 0 new cost 0.103340
###processor 0 pe cost 30.150868
###processor 0 all cost 31.902517
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
final and delete cost 38.4672
0 work done
process cost 55
my time : 55

../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/p1.fq --in2 /dev/shm/p2.fq --out /dev/shm/combine_read.fq --mismatch 2 --thread 24 --thread2 32
spatialRNADrawMap, time used: 55 seconds
1 work done
```

compute015

```
gz
[PAC20217111@compute015 data]$ ./cc.sh
Process 0 of 2 ,processor name is compute015
processor num is 2
processor 0 thread is 24
outGzSpilt is 0
now use pugz, 2 threads
now use pigz, 8 threads
now out name is /dev/shm/combine_read0.fq
Process 1 of 2 ,processor name is compute015
processor 1 thread is 32
now out name is /dev/shm/combine_read1.fq
mergeDone 0
mergeDone 0
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000205
now use pugz0 to decompress(2 threads)
now use pugz1 to decompress(2 threads)
###############load barcodeToPosition map begin...
new and hdf5 pre cost 0.000182
read and close hdf5 cost 8.033340
read and close hdf5 cost 8.033145
new,init bf and hashMap cost 0.088184
new,init bf and hashMap cost 0.198487
parallel for cost 6.667828
###############load barcodeToPosition map finished, time used: 15 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 1 get map thread done,cost 14.8326
mps
0 0 1 1 1
processor 1 get results done,cost 0.0012
parallel for cost 11.305626
###############load barcodeToPosition map finished, time used: 20 seconds
getBarcodePositionMap_uniqBarcodeTypes: 0
processor 0 get map thread done,cost 19.5799
mps
0 0 1 1 1
processor 0 get results done,cost 0.0007
============pigz init cost 0.000045
path is /dev/shm/combine_read0.fq
-----------------in pigz init cost 0.0000391
start compress...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in init cost 0.0042880
pugz0 done, cost 56
processor 1 producer get 2974 chunk done, cost 41.46547
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 37.0358
pugz1 done, cost 59
processor 1 consumer cost 44.1034
processor 1 writer done, cost 44.1034
###processor 1 wait cost 4.053728
###processor 1 format cost 2.315779
###processor 1 new cost 0.252660
###processor 1 pe cost 35.203392
###processor 1 all cost 40.623659
watind barrier
pugz0 done, cost 62
processor 0 producer get 1984 chunk done, cost 42.84014
pugz1 done, cost 65
processor 0 consumer cost 45.7804
processor 0 wSum is 22258049527
processor 0 writer done, cost 45.7806
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in do-while cost 45.7752080
pigz pSum is 22258049527
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in end cost 0.0575409
-----------------in pigz compress cost 45.8370590
compress done
-----------------in pigz finish cost 0.0000210
============pigz process0 cost 45.837126
============pigz process1 cost 0.000000
============pigz end cost 0.001552
pigz done, cost 45.8399
###processor 0 wait cost 2.221505
###processor 0 format cost 2.525633
###processor 0 new cost 0.167236
###processor 0 pe cost 42.482757
###processor 0 all cost 44.966081
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:	0
after_filter_query_cnt:	0
find_query_cnt:	0
total_reads:	195794682
fixed_sequence_contianing_reads:	0	0.00%
pass_filter_reads:	195794682
mapped_reads:	144646378	73.88%
barcode_exactlyOverlap_reads:	118394902	60.47%
barcode_misOverlap_reads:	26251476	13.41%
barcode_withN_reads:	0	0.00%
Q10_bases_in_barcode:	99.38%
Q20_bases_in_barcode:	97.16%
Q30_bases_in_barcode:	91.39%
Q10_bases_in_umi:	98.17%
Q20_bases_in_umi:	93.36%
Q30_bases_in_umi:	85.47%
umi_filter_reads:	3876700	1.98%
umi_with_N_reads:	77	0.00%
umi_with_polyA_reads:	8509	0.00%
umi_with_low_quality_base_reads:	3868114	1.98%
========================================================================
final and delete cost 45.8404
0 work done
process cost 65
my time : 65

1 work done../ST_BarcodeMap-0.0.1 --in /dev/shm/DP8400016231TR_D1.barcodeToPos.h5 --in1 /dev/shm/V300091300_L03_read_1.fq.gz --in2 /dev/shm/V300091300_L04_read_1.fq.gz --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 24 --thread2 32 --usePugz --pugzThread 2 --usePigz --pigzThread 8
spatialRNADrawMap, time used: 66 seconds


fq

```

嘶

👆todos加几个比较急的，今天弄完

## 1017

- [ ] why sometimes gz version be killed(segmentation fault)
- [ ] optimize pugz(2 threads -> 4 threads or 2 threads 40s)
- [x] optimize pigz write disk problem

先搞搞3吧。

现在010节点上，输出信息👇：

```
pugz0 done, cost 53
processor 0 producer get 1984 chunk done, cost 38.81280
pugz0 done, cost 54
processor 1 producer get 2974 chunk done, cost 39.98529
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1
processor merge get 13353484789 data
processor 0 get data done
merge done, cost 40.2831
pugz1 done, cost 56
processor 0 consumer cost 42.0017
processor 0 wSum is 22253539332
processor 0 writer done, cost 42.0020
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in do-while cost 41.5865941
pigz pSum is 22253539332
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~in in end cost 0.0580671
-----------------in pigz compress cost 42.0589049
compress done
-----------------in pigz finish cost 0.0000210
============pigz process0 cost 42.058955
============pigz process1 cost 0.000000
============pigz end cost 0.000646
pigz done, cost 42.0608
###processor 0 wait cost 8.691439
###processor 0 format cost 1.988438
###processor 0 new cost 0.131712
###processor 0 pe cost 28.123457
###processor 0 all cost 36.540631
watind barrier
pugz1 done, cost 57
processor 1 consumer cost 43.1772
processor 1 writer done, cost 43.1772
###processor 1 wait cost 5.907558
###processor 1 format cost 2.174287
###processor 1 new cost 0.141517
###processor 1 pe cost 32.303529
###processor 1 all cost 38.963934
watind barrier
all merge done
```

嘶，好像是pugz拖慢了后面的速度。。。。

--thread 22 --thread2 32 --usePugz --pugzThread 4 --usePigz --pigzThread 10

👆这样不会卡在pugz上，也不会卡在pigz上，能到54s。

<img src="/Users/ylf9811/Library/Application Support/typora-user-images/image-20211016194632959.png" alt="image-20211016194632959" style="zoom:30%;" />

好，现在确定还是用两个节点。

把输出搞到/users就基本解决了pigz输出的问题。

现在先找找为啥被kill，综合之前的情况看，gz in的时候问题不大，gz out的时候有概率会gg，而且下午试了40次单节点gz out也没有出错，暂时把bug定位到mpi且gz out的部分，还有下午试了pugz6线程会gg？

好像确实是这样的。

```
//TODO lastInfo.size > size_
```

？？？咋

搞完上面的bug之后，又测了测，发现outGzSpilt的话就不会re，基本锁定了是mpi的问题，仔细看了看mpi部分吗，把原来long--longlong的改了，把merge result的部分注释了，还是有1/5的概率会gg。

## 1018

现在试试把通信子改成用全局的，还是会1/10的概率GG。应该可以定位到mpi的问题了，但是之前测的，不用pigz的话mpi通信也不会有问题？还有一种可能就是thread开的太多会导致re？或者pigz/pugz内存使用太大导致re？

嘶 都试了试，感觉好像可能大概八成是因为线程数开的太多了，保证不超过32的话，测试的是35次才会第一个re，并且测了测故意多开好多线程，第三次就gg了。

⌨️没电了。。活了活了

好啊，现在关于段错误的问题，大约是开的线程越多，发生的概率越高；并且，取消mpi的通信的话，32+32+8+16不会报错；但是保留mpi，读写fq也不会报错，所以段错误即和pigz有关，也和mpi通信有关，

![image-20211017235043450](/Users/ylf9811/Library/Application Support/typora-user-images/image-20211017235043450.png)

![image-20211018001425782](/Users/ylf9811/Library/Application Support/typora-user-images/image-20211018001425782.png)

直接yue了，好像psum wsum经常出错啊？？？？？？算了 不太配回宿舍睡觉了

in out fq好像没啥问题

最新版光用pugz就gg了，感觉进程0少了一个块22253530289。

现在回退到e5085d6a636c3762ce2b1fb56a9223c51aa2775d试试pugz会不会出错。22253581457？？？我他妈

再回退一个22c159e6d656dcf6eaf8ed79a4d36fb869f4cfb8 -- 22249186189？？？

processor 1 get results done,cost 0.0008
processor 0  producer get1 20754231334 data
processor 0  producer get2 20754944810 data
processor 0 producer get 1653 chunk done, cost 32.57374
processor 0 consumer cost 32.8174
processor 1  producer get1 20754231334 data
processor 1  producer get2 20754944810 data
processor 1 producer get 3305 chunk done, cost 33.75462
processor 1 consumer cost 34.0031
processor 1 send data done, now send -1
processor 1 send -1 done
processor 0 get -1 1 1
processor merge get 14832036611 data
processor 0 get data done
merge done, cost 34.7669
processor 1 writer done, cost 34.0040
###processor 1 wait cost 1.013396
###processor 1 format cost 2.649212
###processor 1 new cost 0.132881
###processor 1 pe cost 30.223167
###processor 1 all cost 33.379192
watind barrier
processor 0 wSum is 22253458630
processor 0 writer done, cost 34.7691
###processor 0 wait cost 18.218757
###processor 0 format cost 1.678114
###processor 0 new cost 0.082381
###processor 0 pe cost 14.598178
###processor 0 all cost 31.640586
watind barrier
all merge done
=======================print ans from process 0=========================
all merge done
total_query_cnt:        0
after_filter_query_cnt: 0
find_query_cnt: 0
total_reads:    195794682
fixed_sequence_contianing_reads:        0       0.00%
pass_filter_reads:      195794682
mapped_reads:   144646378       73.88%
barcode_exactlyOverlap_reads:   118394902       60.47%
barcode_misOverlap_reads:       26251476        13.41%
barcode_withN_reads:    0       0.00%



好啊，最终pugz -2 V34，基本上不会错（～15）

但是pigz就gg了，不过输出的行数信息啥的都是对的，就是wSum大小上少了几Mb，现在准备把要输出的4000多个chunk的size打印一下，看看到底是哪里g了。merge get的size是对的，也就是说，是本节点的那1/3的chunk出错了，

## 1019

昨晚简直是他妈的至暗时刻，好在天亮的时候找到bug了，现在一切都好起来了，hashMap构建掩盖在read h5里面了，初始化9s，1：1fq查询的话30s左右，整个程序现在40s，也就是说，32线程1:1查询能28+2查询完成，测测28的，41s，26->43  24->45 基本上输出gz的话分24+8，分开输出，整个程序能到45s，还可以。

现在开始搞搞pugz

|           |             |      |
| --------- | ----------- | ---- |
| 30+2/30+2 | 56/56/57/57 | 57   |
| 30+4/30+4 | 36/38/39/40 | 44   |
| 30+8/30+8 | 31/33/33/36 | 45   |

👆的测试方法pugz线程被查询抢了，不太准，现在试试只开pugz。

|          | pugz in pac                                                  | pugz to shm |
| -------- | ------------------------------------------------------------ | ----------- |
| p1.fq.gz |                                                              | 34          |
| p2.fq.gz |                                                              | 37          |
| V3       | 47（no out 41，out 1 byte 41，out half btye 44，only new no memcpy 42） | 41          |
| V4       |                                                              | 44          |

gg memcpy就是慢。。。。木得办法，现在先想想稳定4个线程的事



## 1020

## 1021

```
../ST_BarcodeMap-0.0.1 --in /users/ylf/DP8400016231TR_D1.barcodeToPos.h5 --in1 /users/ylf/p1.fq --in2 /users/ylf/p2.fq --out /dev/shm/combine_read.fq.gz --mismatch 2 --thread 16 --thread2 32 --usePigz --pigzThread 16
spatialRNADrawMap, time used: 44 seconds


../ST_BarcodeMap-0.0.1 --in /users/ylf/DP8400016231TR_D1.barcodeToPos.h5 --in1 /users/ylf/p1.fq.gz --in2 /users/ylf/p2.fq.gz --out /users/ylf/combine_read.fq.gz --mismatch 2 --thread 14 --thread2 32 --usePigz --pigzThread 18 --usePugz --pugzThread 4
spatialRNADrawMap, time used: 49 seconds



../ST_BarcodeMap-0.0.1 --in /users/ylf/DP8400016231TR_D1.barcodeToPos.h5 --in1 /users/ylf/p1.fq.gz --in2 /users/ylf/p2.fq.gz --out /users/ylf/combine_read.fq.gz --mismatch 2 --thread 15 --thread2 32 --usePigz --pigzThread 18 --usePugz --pugzThread 3
spatialRNADrawMap, time used: 49 seconds


mpigxx -c src/prog_util.cpp -o obj/prog_util.o -std=c++11 -I. -Icommon -w -Wextra -Weffc++ -Wpedantic -Wundef -Wuseless-cast -Wconversion -Wshadow -Wdisabled-optimization -Wparentheses -Wpointer-arith -O2 -flto=jobserver -march=native -mtune=native -g -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -fopenmp

mpigxx -c src/tgetopt.cpp -o obj/tgetopt.o -std=c++11 -I. -Icommon -w -Wextra -Weffc++ -Wpedantic -Wundef -Wuseless-cast -Wconversion -Wshadow -Wdisabled-optimization -Wparentheses -Wpointer-arith -O2 -flto=jobserver -march=native -mtune=native -g -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -fopenmp

mpigxx ./obj/chipMaskHDF5.o ./obj/barcodeProcessor.o ./obj/FastqStream.o ./obj/sequence.o ./obj/barcodePositionMap.o ./obj/pigz.o ./obj/FastqIo.o ./obj/writerThread.o ./obj/barcodePositionConfig.o ./obj/chipMaskMerge.o ./obj/read.o ./obj/bloomFilter.o ./obj/htmlreporter.o ./obj/writer.o ./obj/result.o ./obj/chipMaskFormatChange.o ./obj/tgetopt.o ./obj/prog_util.o ./obj/barcodeToPositionMulti.o ./obj/main.o ./obj/fixedfilter.o ./obj/fastqreader.o ./obj/barcodeToPositionMultiPE.o ./obj/options.o ./obj/barcodeListMerge.o ./obj/deflate.o ./obj/try.o ./obj/symbols.o ./obj/yarn.o ./obj/squeeze.o ./obj/lz77.o ./obj/katajainen.o ./obj/hash.o ./obj/cache.o ./obj/utilPigz.o ./obj/blocksplitter.o ./obj/tree.o -o ST_BarcodeMap-0.0.1 -std=c++11 -I. -Icommon -w -Wextra -Weffc++ -Wpedantic -Wundef -Wuseless-cast -Wconversion -Wshadow -Wdisabled-optimization -Wparentheses -Wpointer-arith   -O2 -flto=jobserver -march=native -mtune=native -g -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -lz -lpthread -lhdf5 -lboost_serialization -fopenmp -lrt -lm  -lrt -ldeflate


../ST_BarcodeMap-0.0.1 --in /users/ylf/DP8400016231TR_D1.barcodeToPos.h5 --in1 /users/ylf/V300091300_L03_read_1.fq.gz --in2 /users/ylf/V300091300_L04_read_1.fq.gz --out /users/ylf/combine_read.fq.gz --mismatch 2 --thread 15 --thread2 32 --usePigz --pigzThread 18 --usePugz --pugzThread 4
spatialRNADrawMap, time used: 51 seconds
```

