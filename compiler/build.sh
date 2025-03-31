#!/bin/bash

asm="rionc"
files=$(find src -type f -name "*.c")

cc=clang

echo "Building $asm"

set -xue

$cc $files -o ../build/$asm -Isrc -g -Wall -Wextra -Werror -Wno-missing-braces -Wno-sizeof-array-div -Wno-sizeof-pointer-div -pedantic -std=c11
