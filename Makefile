test: cpu.cpp memory.h memory.cpp
	gcc cpu.cpp -o nes_emu.out
	./nes_emu.out
