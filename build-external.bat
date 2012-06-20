clang external.c -ccc-host-triple i686-pc-win32 -Wall -emit-llvm -c -o external.bc
llvm-dis external.bc