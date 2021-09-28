cmake_minimum_required(VERSION 3.16)
project(PAC2022)

set(CMAKE_CXX_STANDARD 14)

aux_source_directory(./src/ DIR_HELLO_SRCS)
add_executable(PAC2022 ${DIR_HELLO_SRCS})