cmake_minimum_required(VERSION 4.3)
project(YearReporter)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)

add_executable(YearReporter main.cpp)
target_link_libraries(YearReporter PRIVATE Drogon::Drogon)
