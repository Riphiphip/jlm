cmake_minimum_required(VERSION 3.16)

include(../IncludeLLVM.cmake)
include(../IncludeCIRCT.cmake)

add_executable(jlm-hls jlm-hls.cpp)

target_link_libraries(jlm-hls jlm)

install(TARGETS jlm-hls)