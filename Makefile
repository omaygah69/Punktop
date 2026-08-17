CC = clang++ 
SRC = src/libraylib.a src/Imgui/*.cpp src/*.cpp src/implot/*.cpp
OUT = raytest
LIBS = -lX11 -lXrandr -lXi -lXcursor -lXinerama -lGL -lm -ldl -lpthread
INCLUDE = -Isrc/include -Isrc/extras -Isrc/Imgui -Isrc/implot 

.PHONY: all clean

all:
	$(CC) $(SRC) $(INCLUDE) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)
