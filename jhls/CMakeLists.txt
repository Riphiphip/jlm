cmake_minimum_required(VERSION 3.16)

include(../IncludeLLVM.cmake)
include(../IncludeCIRCT.cmake)

add_executable(jhls jhls.cpp)

include_directories(jhls include)
target_link_libraries(jhls jlm)

install(TARGETS jhls)