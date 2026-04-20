# Compiler & Flags
CXX := g++
CXXFLAGS := -std=c++17  # C++ version
CXXFLAGS += -Wall -Wextra -pedantic  # generate all the warnings
CXXFLAGS += -g  # add GDB instrumentation
CXXFLAGS += -I .. 

BIN_DIR := bin
OBJ_DIR := obj

TARGET := $(BIN_DIR)/mmap_util
OBJS := $(OBJ_DIR)/main.o $(OBJ_DIR)/mmap.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@

$(OBJ_DIR)/main.o: src/main.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/mmap.o: lib/src/mmap.cc lib/include/mmap.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(BIN_DIR)/* $(OBJ_DIR)/*

.PHONY: all clean
