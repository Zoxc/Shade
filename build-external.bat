clang++ external.cpp -ccc-host-triple i686-pc-win32 -D_X86_ "-IC:\MinGW64\x86_64-w64-mingw32\include" -Os -Wall -fno-inline -emit-llvm -c -o external.bc
llvm-dis external.bc