PKGS=freetype2
CXXFLAGS=-Wall -Wextra -Wunused-function -Wconversion -pedantic -ggdb -std=c++20 -I/opt/homebrew/Cellar/giflib/5.2.2/include `pkg-config --cflags $(PKGS)` 
GIFLIBS=-L/opt/homebrew/Cellar/giflib/5.2.2/lib -lgif
LIBS=`pkg-config --libs $(PKGS)` $(GIFLIBS) -lm 

vodus: main.cpp
	g++ $(CXXFLAGS) -o vodus main.cpp $(LIBS)