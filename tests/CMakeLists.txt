cmake_minimum_required(VERSION 3.16)

include(../IncludeLLVM.cmake)

add_executable(jlm-test
        test-operation.cpp
        test-registry.cpp
        test-runner.cpp
        test-types.cpp
        # manually include test here for now
)

include_directories(jlm-test .)
target_link_libraries(jlm-test jlm)
#install(TARGETS jlm-opt)