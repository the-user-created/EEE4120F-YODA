# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -O3
LDFLAGS = -framework OpenCL

# Build settings
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
HEADERS = $(wildcard $(SRC_DIR)/*.h)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
EXECUTABLE = $(BIN_DIR)/md5_opencl

# Default target
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	mkdir -p $(BIN_DIR)
	cp src/Kernel.cl $(BIN_DIR)/Kernel.cl
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

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

.PHONY: all clean
