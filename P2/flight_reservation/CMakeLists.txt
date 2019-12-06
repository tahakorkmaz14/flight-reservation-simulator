cmake_minimum_required(VERSION 3.10)
project(flight_reservation)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lpthread")
SET(CMAKE_CXX_FLAGS -pthread)
add_executable(flight_reservation main.cpp)