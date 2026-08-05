CC = clang++ 
SRC = exp.cpp libraylib.a imgui/*.cpp rlImGui.cpp
OUT = raytest
LIBS = -lX11 -lXrandr -lXi -lXcursor -lXinerama -lGL -lm -ldl -lpthread
INCLUDE = -Iinclude -Iinclude/imgui -Iextras

.PHONY: all clean

all:
	$(CC) $(SRC) $(INCLUDE) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)
