PKGS=freetype2
CXXFLAGS=-Wall -Wextra -Wunused-function -Wconversion -pedantic -ggdb -std=c++20 `pkg-config --cflags $(PKGS)`
LIBS=`pkg-config --libs $(PKGS)` -lm

vodus: main.cpp
	g++ $(CXXFLAGS) -o vodus main.cpp $(LIBS)