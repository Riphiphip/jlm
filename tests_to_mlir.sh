#!/usr/bin/env bash

for file in ./tests/c-tests/*; do
    NAME=$(basename -- "$file" .c);
    clang-14 -S -emit-llvm -Xclang -disable-O0-optnone "$file" -o "./mlir-print-test-files/$NAME.ll";
    opt-14 -mem2reg -S "./mlir-print-test-files/$NAME.ll" -o "./mlir-print-test-files/$NAME.opt.ll";
    ./bin/mlir-print --file "./mlir-print-test-files/$NAME.opt.ll" > "./mlir-print-test-files/$NAME.mlir";
done;