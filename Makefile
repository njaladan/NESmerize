all: *.cpp *.h
	g++ nes.cpp -w -lSDL2 -lSDL2_image -o nes.out


make debug: *.cpp *.h
	g++ nes.cpp -w -lSDL2 -lSDL2_image -g -o nes.out
