cmake_minimum_required(VERSION 3.22.1)
project(project2)

set(CMAKE_CXX_STANDARD 17)

add_executable(project2 main.c
        common.h
        receiverUtil.c
        receiverUtil.h
        communicate.c
        communicate.h
        input.c
        input.h
        receiver.c
        receiver.h
        sender.c
        sender.h
        util.c
        util.h
        crc.c
        crc.h)
