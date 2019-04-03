cmake_minimum_required(VERSION 3.13)
project(sknx)

set(CMAKE_CXX_STANDARD 14)

include_directories(libs
        src/shared)


add_library(SknxLib
        libs/polarssl/src/aes.c
        libs/polarssl/src/aesni.c
        libs/polarssl/src/bignum.c
        libs/polarssl/src/ctr_drbg.c
        libs/polarssl/src/ecp.c
        libs/polarssl/src/ecp_curves.c
        libs/polarssl/src/entropy.c
        libs/polarssl/src/entropy_poll.c
        libs/polarssl/src/miosix_poll.cc
        libs/polarssl/src/net.c
        libs/polarssl/src/platform.c
        libs/polarssl/src/sha512.c
        libs/polarssl/src/timing.c
        libs/sban/src/bd1.c
        libs/sban/src/bd2.c
        libs/sban/src/gdh2.c
        libs/sban/src/mka.c
        libs/sban/src/util.c)

target_include_directories(SknxLib PUBLIC
        libs
        src/shared)