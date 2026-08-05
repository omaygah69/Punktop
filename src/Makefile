CC = clang++
SRC = test.cpp libraylib.a
OUT = raytest
LIBS = -lX11 -lXrandr -lXi -lXcursor -lXinerama -lGL -lm -ldl -lpthread
INCLUDE = -Iinclude

.PHONY: all clean

all:
	$(CC) $(SRC) $(INCLUDE) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)
