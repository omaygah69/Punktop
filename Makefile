CXX := clang++

SRC_DIR := src
BUILD_DIR := build
TARGET := $(BUILD_DIR)/punktop

CXXFLAGS := -Wall -Wextra \
            -Iinclude \
            -Iextras \
            -Isrc/imgui \
            -Isrc/implot

LDLIBS := -Llib -lraylib \
          -lX11 -lXrandr -lXi -lXcursor -lXinerama \
          -lGL -lm -ldl -lpthread

SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS := $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) system.json
