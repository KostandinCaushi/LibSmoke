cmake_minimum_required(VERSION 2.8.12)

project(tinyaes C ASM)

add_library(tiny-aes
        aes.c)

target_include_directories(tiny-aes PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})
