cmake_minimum_required(VERSION 3.13)
project(libsmoke)

set(CMAKE_CXX_STANDARD 14)

include_directories(libs)

#Include CMakeLists of the Libraries
add_subdirectory(libs/tiny-AES-c-master)
add_subdirectory(libs/sknx)


add_executable(server
        tests/server_test.cpp)

add_executable(client1
        tests/client_test.cpp)

add_executable(client2
        tests/client_test.cpp)

target_link_libraries(server SknxLib)
target_link_libraries(client1 SknxLib)
target_link_libraries(client1 tiny-aes)
target_link_libraries(client2 SknxLib)
target_link_libraries(client2 tiny-aes)