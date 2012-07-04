@echo off
clang++ external.cpp d3.cpp heap.cpp ui.cpp shared.cpp utils.cpp -std=gnu++11 -ffreestanding -ccc-host-triple i686-pc-win32 -D_X86_ "-IC:\MinGW64\x86_64-w64-mingw32\include" -Os -Wall -fno-exceptions -fno-inline -emit-llvm -c
llvm-link external.o d3.o heap.o ui.o shared.o utils.o  -o=../external.bc
llvm-dis ../external.bc