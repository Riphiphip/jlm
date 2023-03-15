cmake_minimum_required(VERSION 3.16)

include(../IncludeLLVM.cmake)

add_executable(jlc jlc.cpp)

target_link_libraries(jlc jlm)
install(TARGETS jlc)