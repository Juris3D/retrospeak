#!/bin/sh
# RetroSpeak - build: the Windows app (MinGW-w64, cross from Linux or in MSYS2) + a Linux command line build
set -e
cd "$(dirname "$0")"
mkdir -p build
SRC="src/rules.cpp src/synth.cpp src/cli.cpp"
x86_64-w64-mingw32-windres res/retrospeak.rc -O coff -o build/res.o
x86_64-w64-mingw32-g++ -O2 -std=c++17 -static -mwindows -o build/RetroSpeak.exe src/main_win.cpp $SRC build/res.o -lcomctl32 -lcomdlg32 -lwinmm
g++ -O2 -std=c++17 -o build/retrospeak src/main_cli.cpp $SRC
echo "built: build/RetroSpeak.exe build/retrospeak"
