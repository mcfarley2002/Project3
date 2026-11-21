CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -g

all: scene_editor

scene_editor: main.cpp scene.cpp scene.hpp
	$(CXX) $(CXXFLAGS) main.cpp scene.cpp -o $@

clean:
	rm -f scene_editor 

.PHONY: all clean
