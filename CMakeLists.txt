cmake_minimum_required(VERSION 3.16)
project(osex4_new3 C)

set(CMAKE_C_STANDARD 11)

add_executable(osex4_new3 main.c osqueue.c osqueue.h threadPool.c threadPool.h test.c)