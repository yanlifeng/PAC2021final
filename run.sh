cd data && mpirun -n 2 ../ST_BarcodeMap-0.0.1 --in ~/ST_BarcodeMap-main/data/DP8400016231TR_D1.barcodeToPos.h5 --in1 ~/ST_BarcodeMap-main/data/V300091300_L03_read_1.fq.gz --in2 ~/ST_BarcodeMap-main/data/V300091300_L04_read_1.fq.gz --out combine_read.fq.gz --mismatch 2 --thread 12 --thread2 24 --usePigz --pigzThread 12