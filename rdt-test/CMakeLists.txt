cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(basicTest basic_test.cpp)
target_link_libraries(basicTest gtest gtest_main pthread)

add_executable(packetTests packet_tests.cpp slidingwindow_tests.cpp)
target_link_libraries(packetTests gtest gtest_main pthread RDT)