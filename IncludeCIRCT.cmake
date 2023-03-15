message(STATUS "CIRCT_PATH ${CIRCT_PATH}")
find_package(MLIR REQUIRED CONFIG PATHS ${CIRCT_PATH}/llvm/build/lib/cmake)

message(STATUS "Found MLIR ${MLIR_PACKAGE_VERSION}")
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

separate_arguments(MLIR_DEFINITIONS_LIST NATIVE_COMMAND ${MLIR_DEFINITIONS})
include_directories(${MLIR_INCLUDE_DIRS})
add_definitions(${MLIR_DEFINITIONS_LIST})

find_package(CIRCT REQUIRED CONFIG PATHS ${CIRCT_PATH}/build/lib/cmake)

message(STATUS "Found CIRCT ${CIRCT_PACKAGE_VERSION}")
message(STATUS "Using CIRCTConfig.cmake in: ${CIRCT_DIR}")

separate_arguments(CIRCT_DEFINITIONS_LIST NATIVE_COMMAND ${CIRCT_DEFINITIONS})
include_directories(${CIRCT_INCLUDE_DIRS})
add_definitions(${CIRCT_DEFINITIONS_LIST})