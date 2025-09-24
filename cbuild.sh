#!/usr/bin/env bash

CXX="g++"
SRC="main.cpp imgui/*.cpp src/*.cpp"
OUT="build/punktop"
CFLAGS="$(sdl2-config --cflags --libs) -Iinclude/imgui -Iinclude/implot -Iinclude -lpthread"

build() {
    mkdir -p build
    echo "[INFO] Starting build..."
    echo "[CMD] $CXX $SRC -o $OUT $CFLAGS"
    $CXX $SRC -o $OUT $CFLAGS
    echo "[INFO] Build completed"
}

run() {
    build
    echo "[INFO] Running $OUT"
    ./$OUT
}

case "$1" in
    build|b)
        build
        ;;
    run|r)
        run
        ;;
    *)
        echo "[ERROR] Invalid command: $1"
        exit 1
        ;;
esac
