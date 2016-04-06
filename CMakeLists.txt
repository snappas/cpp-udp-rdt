cmake_minimum_required(VERSION 3.4)
project(udp-rdt)

add_subdirectory(rdt-test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_library(RDT RDT.cpp packet.h SlidingWindow.h)
target_link_libraries(RDT jsoncpp)
target_include_directories(RDT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(Sender sender.cpp)
target_link_libraries(Sender LINK_PUBLIC RDT)

add_executable(Receiver receiver.cpp)
target_link_libraries(Receiver LINK_PUBLIC RDT)

add_executable(Simulator simulator.cpp)