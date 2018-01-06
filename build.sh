#!/bin/bash
# Build Script

compile_flags='-static -lgdi32 -static-libgcc -static-libstdc++ -lwinmm'
warnings='-Wall -Wno-unused-parameter'
external='-mwindows -D EXTERNAL_BUILD'
performant='-O3 -D FAST_BUILD'
program_path='-o snow.exe win32_snow.cpp'

echo -e "Compiling Program..."
x86_64-w64-mingw32-g++ $program_path $compile_flags $performant $warnings
