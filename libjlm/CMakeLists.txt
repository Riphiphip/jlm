cmake_minimum_required(VERSION 3.16)
include(../IncludeLLVM.cmake)
include(../IncludeCIRCT.cmake)


add_library(jlm STATIC
        src/common.cpp

        src/backend/hls/rvsdg2rhls/add-triggers.cpp
        src/backend/hls/rvsdg2rhls/gamma-conv.cpp
        src/backend/hls/rvsdg2rhls/theta-conv.cpp
        src/backend/hls/rvsdg2rhls/add-sinks.cpp
        src/backend/hls/rvsdg2rhls/add-forks.cpp
        src/backend/hls/rvsdg2rhls/check-rhls.cpp
        src/backend/hls/rvsdg2rhls/rhls-dne.cpp
        src/backend/hls/rvsdg2rhls/remove-unused-state.cpp
        src/backend/hls/rvsdg2rhls/rvsdg2rhls.cpp
        src/backend/hls/rvsdg2rhls/add-prints.cpp
        src/backend/hls/rvsdg2rhls/add-buffers.cpp

        src/backend/hls/rhls2firrtl/base-hls.cpp
        src/backend/hls/rhls2firrtl/dot-hls.cpp
        src/backend/hls/rhls2firrtl/verilator-harness-hls.cpp
        src/backend/hls/rhls2firrtl/firrtl-hls.cpp
        src/backend/hls/rhls2firrtl/mlirgen.cpp

        src/backend/llvm/jlm2llvm/instruction.cpp
        src/backend/llvm/jlm2llvm/jlm2llvm.cpp
        src/backend/llvm/jlm2llvm/type.cpp
        src/backend/llvm/rvsdg2jlm/rvsdg2jlm.cpp

        src/frontend/llvm/ControlFlowRestructuring.cpp
        src/frontend/llvm/InterProceduralGraphConversion.cpp
        src/frontend/llvm/LlvmInstructionConversion.cpp
        src/frontend/llvm/LlvmModuleConversion.cpp
        src/frontend/llvm/LlvmTypeConversion.cpp

        src/ir/aggregation.cpp
        src/ir/Annotation.cpp
        src/ir/attribute.cpp
        src/ir/basic-block.cpp
        src/ir/cfg.cpp
        src/ir/cfg-structure.cpp
        src/ir/cfg-node.cpp
        src/ir/domtree.cpp
        src/ir/ipgraph.cpp
        src/ir/ipgraph-module.cpp
        src/ir/operators/alloca.cpp
        src/ir/operators/call.cpp
        src/ir/operators/delta.cpp
        src/ir/operators/getelementptr.cpp
        src/ir/operators/lambda.cpp
        src/ir/operators/load.cpp
        src/ir/operators/operators.cpp
        src/ir/operators/Phi.cpp
        src/ir/operators/sext.cpp
        src/ir/operators/store.cpp
        src/ir/print.cpp
        src/ir/RvsdgModule.cpp
        src/ir/ssa.cpp
        src/ir/tac.cpp
        src/ir/types.cpp
        src/ir/variable.cpp
        src/ir/hls/hls.cpp

        src/opt/alias-analyses/AgnosticMemoryNodeProvider.cpp
        src/opt/alias-analyses/MemoryStateEncoder.cpp
        src/opt/alias-analyses/MemoryNodeProvider.cpp
        src/opt/alias-analyses/Operators.cpp
        src/opt/alias-analyses/Optimization.cpp
        src/opt/alias-analyses/PointsToGraph.cpp
        src/opt/alias-analyses/RegionAwareMemoryNodeProvider.cpp
        src/opt/alias-analyses/Steensgaard.cpp
        src/opt/cne.cpp
        src/opt/DeadNodeElimination.cpp
        src/opt/inlining.cpp
        src/opt/InvariantValueRedirection.cpp
        src/opt/inversion.cpp
        src/opt/optimization.cpp
        src/opt/pull.cpp
        src/opt/push.cpp
        src/opt/reduction.cpp
        src/opt/unroll.cpp

        src/tooling/Command.cpp
        src/tooling/CommandGraph.cpp
        src/tooling/CommandGraphGenerator.cpp
        src/tooling/CommandLine.cpp

        src/util/Statistics.cpp
        )

target_link_libraries(jlm jive)
target_include_directories(jlm PUBLIC include)

# Link LLVM, MLIR and CIRCT
#set(llvm_libs ${LLVM_AVAILABLE_LIBS})
#target_link_libraries(jlm ${llvm_libs})
#message(STATUS "LLVM libs: ${llvm_libs}")
# we have to link LLVM here instead of individual libraries, because otherwise we get the issue described here:
# https://github.com/jrprice/Oclgrind/issues/164
target_link_libraries(jlm LLVM)
get_property(mlir_libs GLOBAL PROPERTY MLIR_ALL_LIBS)
target_link_libraries(jlm ${mlir_libs})
message(STATUS "MLIR libs: ${mlir_libs}")
get_property(circt_libs GLOBAL PROPERTY CIRCT_ALL_LIBS)
target_link_libraries(jlm ${circt_libs})
message(STATUS "CIRCT libs: ${circt_libs}")