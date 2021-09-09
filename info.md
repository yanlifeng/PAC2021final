# PAC2021 final -- ChiFanBuZhangRou

TODOs

- [x] Add rabbitio
- [x] Use other map
- [ ] Parallel in load map
- [ ] Use secondary index
- [ ] Use list-hash
- [ ] Use aili-code ？？？
- [ ] Optimize match algorithm
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

//TODO 为了过编译把boost部分涉及到map的操作做了复制，还有析构的时候swap都注释掉了

## 0909

目前先搞搞消费者吧，先不整gz，让读和写线程不会是瓶颈。上面是简单把map换了，换前后都能拉起来64线程，说明消费者还有提升空间，今天准备试试链式hash，感觉思路和主办方给的二级索引应该差不多。

确实有点子提升，但是没有特别快，更新👆。

写的时候有点子小小的bug，基础不牢地动山摇啊，本来以为传指针进去就不用&了，但之前的用法都是修改指针指向的空间里面的信息，这可以不用&，但是今天属于是直接该指针的值了。
