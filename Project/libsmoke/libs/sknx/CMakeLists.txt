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

add_executable(client2
        config/arch/arm7_lpc2000/lpc2138_miosix_board/board_settings.h
        config/arch/cortexM3_efm32gg/efm32gg332f1024_wandstem/board_settings.h