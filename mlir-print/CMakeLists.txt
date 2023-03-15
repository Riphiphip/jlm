cmake_minimum_required(VERSION 3.16)

include(../IncludeLLVM.cmake)

add_executable(mlir-print mlir-print.cpp)

target_link_libraries(mlir-print jlm)
install(TARGETS mlir-print)