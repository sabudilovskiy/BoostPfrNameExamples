cmake_minimum_required(VERSION 3.27)
project(BoostPfrNameExamples)

set(CMAKE_CXX_STANDARD 20)

find_package(BOOST 1.83 REQUIRED COMPONENTS pfr)
find_package(fmt REQUIRED CONFIG)

find_package(nlohmann_json CONFIG REQUIRED)
add_executable(BoostPfrNameExamples main.cpp)

target_link_libraries(BoostPfrNameExamples INTERFACE Boost::pfr)
target_link_libraries(BoostPfrNameExamples PRIVATE fmt::fmt)
target_link_libraries(BoostPfrNameExamples PRIVATE nlohmann_json::nlohmann_json)