message(STATUS "CIRCT_PATH ${CIRCT_PATH}")
find_package(LLVM 15 REQUIRED CONFIG PATHS ${CIRCT_PATH}/llvm/build/lib/cmake)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS_LIST})

