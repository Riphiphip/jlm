cmake_minimum_required(VERSION 3.16)

project(libjive)

add_library(jive STATIC
        src/common.cpp
        src/rvsdg/binary.cpp
        src/rvsdg/control.cpp
        src/rvsdg/gamma.cpp
        src/rvsdg/graph.cpp
        src/rvsdg/node-normal-form.cpp
        src/rvsdg/node.cpp
        src/rvsdg/notifiers.cpp
        src/rvsdg/nullary.cpp
        src/rvsdg/operation.cpp
        src/rvsdg/region.cpp
        src/rvsdg/simple-normal-form.cpp
        src/rvsdg/simple-node.cpp
        src/rvsdg/statemux.cpp
        src/rvsdg/structural-normal-form.cpp
        src/rvsdg/structural-node.cpp
        src/rvsdg/theta.cpp
        src/rvsdg/tracker.cpp
        src/rvsdg/traverser.cpp
        src/rvsdg/type.cpp
        src/rvsdg/unary.cpp
        # visualization
        src/util/callbacks.cpp
        src/view.cpp
        # bitstrings
        src/types/bitstring/arithmetic.cpp
        src/types/bitstring/bitoperation-classes.cpp
        src/types/bitstring/comparison.cpp
        src/types/bitstring/concat.cpp
        src/types/bitstring/constant.cpp
        src/types/bitstring/slice.cpp
        src/types/bitstring/type.cpp
        src/types/bitstring/value-representation.cpp
        # records
        src/types/record.cpp
        )

target_include_directories(jive PUBLIC include)