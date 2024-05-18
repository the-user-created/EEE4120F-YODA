# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -O3

# Build settings
SRC_DIR = src
HEAD_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
HEADERS = $(wildcard $(HEAD_DIR)/*.h)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
EXECUTABLE = $(BIN_DIR)/md6_cpp

# Default target
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(HEADERS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean, build, and run
re: clean all run

# Run target
run: $(EXECUTABLE)
	$(EXECUTABLE)

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean run re
