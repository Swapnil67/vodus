PKGS=freetype2 libpng
CXXFLAGS=-Wall -Wextra -Wunused-function -Wconversion -pedantic -ggdb -std=c++20 -I/opt/homebrew/Cellar/giflib/5.2.2/include `pkg-config --cflags $(PKGS)` 
GIFLIBS=-L/opt/homebrew/Cellar/giflib/5.2.2/lib -lgif
LIBS=`pkg-config --libs $(PKGS)` $(GIFLIBS) -lm 

vodus: main.cpp
	g++ $(CXXFLAGS) -o vodus main.cpp $(LIBS)

.PHONY: render
render: output.mp4

output.mp4: vodus
	rm -rf output/
	mkdir -p output/
	./vodus "zoro" cat-swag.gif gasm.png > /dev/null
	ffmpeg -y -framerate 100 -i 'output/frame-%05d.png' output.mp4