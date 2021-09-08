# PAC2021 final -- ChiFanBuZhangRou

TODOs

- [x] Add rabbitio
- [x] Use other map
- [ ] Parallel in load map
- [ ] Use secondary index
- [ ] Optimize match algorithm
- [ ] Add pugz
- [ ] Use parallel zip when write
- [ ] check todos 👇（//TODO）
- [ ] why write .fq.gz size diff ?
- [ ] 

## 0908

淦之前写的info整不见了，麻了。

| version                                      | total  | hdf5  | others |      |      |
| -------------------------------------------- | ------ | ----- | ------ | ---- | ---- |
| init                                         | ～1100 | ～100 | ～1000 |      |      |
| add rabbitio from rabbitqc                   | 750    | 100   | 650    |      |      |
| no gz                                        | 386    | 100   | 286    |      |      |
| replace std::unordered_map to robin_hood::xx |        | 50    |        |      |      |

//TODO 为了过编译把boost部分涉及到map的操作做了复制，还有析构的时候swap都注释掉了

