#!/usr/bin/bash
# g++ demomain.cpp imgui/* -o build/demo $(sdl2-config --cflags --libs) -Iinclude/imgui && ./build/demo

build(){
    echo "[INFO] Starting build..."
    echo "[CMD] g++ main.cpp imgui/* src/*.cpp -o build/wtop $(sdl2-config --cflags --libs) -Iinclude/imgui -Iinclude -lpthread"
    g++ main.cpp imgui/* src/*.cpp -o build/wtop $(sdl2-config --cflags --libs) -Iinclude/imgui -Iinclude -lpthread
    echo "[INFO] Build completed"
}

run(){
    echo "[INFO] Starting build..."
    echo "[CMD] g++ main.cpp imgui/* src/*.cpp -o build/wtop $(sdl2-config --cflags --libs) -Iinclude/imgui -Iinclude -lpthread"
    g++ main.cpp imgui/* src/*.cpp -o build/wtop $(sdl2-config --cflags --libs) -Iinclude/imgui -Iinclude -lpthread
    echo "[INFO] Build completed"
    ./build/wtop
    rm -rf ./build/wtop
}

case "$1" in
    build|b)
	build
	exit 0
	;;
    run|r)
	run
	exit 0
	;;
esac
echo "[ERROR] Invalid commad: $1"
