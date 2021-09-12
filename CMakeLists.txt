cmake_minimum_required(VERSION 3.16)
project(ST_BarcodeMap-main)

set(CMAKE_CXX_STANDARD 14)

aux_source_directory(./src/ DIR_HELLO_SRCS)
add_executable(ST_BarcodeMap-main ${DIR_HELLO_SRCS})