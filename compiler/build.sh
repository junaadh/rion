#!/bin/bash

asm="rionc"
files=$(find src -type f -name "*.c")

cc=clang

echo "Building $asm"

set -xue

$cc $files -o ../build/$asm -g -Wall -Wextra -Werror -pedantic -std=c11
