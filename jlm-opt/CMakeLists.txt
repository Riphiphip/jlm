cmake_minimum_required(VERSION 3.16)

include(../IncludeLLVM.cmake)

add_executable(jlm-opt jlm-opt.cpp)

include_directories(jlm-opt include)
target_link_libraries(jlm-opt jlm)
install(TARGETS jlm-opt)