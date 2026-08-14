#!/bin/bash

if [ ! -d build ]; then
    mkdir -p build;
fi

SourceFile="code/entry.c"
OutputFile="build/windcc"

CompileFlags=" \
    -g \
    -O0 \
    -ffreestanding \
    -fpie \
    -fno-stack-protector \
    -nostdlib \
    -std=c11 \
    -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-variable \
    -Wno-unused-but-set-variable \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -o $OutputFile"

LinkFlags=" \
    -fuse-ld=lld \
    -Wl,-nostdlib \
    -Wl,-entry,EntryPoint"

clang $CompileFlags $LinkFlags $SourceFile

