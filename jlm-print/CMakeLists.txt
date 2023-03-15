cmake_minimum_required(VERSION 3.16)

include(../IncludeLLVM.cmake)

add_executable(jlm-print jlm-print.cpp)

target_link_libraries(jlm-print jlm)
install(TARGETS jlm-print)