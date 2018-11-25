test: disassembler.c zelda.nes
	gcc disassembler.c -o disassembler
	./disassembler zelda.nes
