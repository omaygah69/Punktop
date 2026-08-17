CC = clang++ 
SRC = main.cpp libraylib.a Imgui/*.cpp implot/*.cpp rlImGui.cpp
BRUH = api.cpp cpu.cpp cpuplot.cpp disk.cpp dockspace.cpp memoryplot.cpp net.cpp proc.cpp system.cpp systemfetch.cpp
OUT = punktop
LIBS = -lX11 -lXrandr -lXi -lXcursor -lXinerama -lGL -lm -ldl -lpthread
INCLUDE = -Iinclude -Iextras -IImgui -Iimplot 

.PHONY: all clean

all:
	$(CC) $(SRC) $(INCLUDE) $(BRUH) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)



