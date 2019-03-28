#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;

enum Interrupt {NONE, IRQ, NMI};

#include "opcodes.cpp"
#include "cpu.h"
#include "memory.h"

#include "ppu_memory.cpp"
#include "ppu.cpp"
#include "memory.cpp"
#include "cpu.cpp"

using namespace std::chrono;

class NES {
  public:
  CPU cpu;
  Memory memory;
  PPU ppu;
  PPUMemory ppu_memory;

  void create_system();
  void load_program(char*);
  void play_game(char*);
  void run_game();

  // debugging
  void print_pattern_tables();
};


void NES::create_system() {
  cpu.set_memory(&memory);
  cpu.set_ppu(&ppu);
  ppu.set_memory(&memory);
  ppu.set_ppu_memory(&ppu_memory);
  ppu.set_cpu(&cpu);
  ppu.initialize();
  memory.set_cpu(&cpu);
  memory.set_ppu_memory(&ppu_memory);
  memory.set_ppu(&ppu);
}

void NES::load_program(char* filename) {
  ifstream rom(filename, ios::binary | ios::ate);
  streamsize size = rom.tellg();
  rom.seekg(0, ios::beg);
  char buffer[size];

  if (rom.read(buffer, size)) {
    int prg_size = buffer[4] * 0x4000;
    int mapper = (buffer[6] >> 4) | (buffer[7] & 0xf0);
    if (mapper != 0) {
      cout << "not mapper 0! exiting...";
      return;
    }
    // set PRG memory pointers
    char* program = (buffer + 0x10);
    memory.set_prg_nrom_top((uint8_t*) program);
    if (prg_size == 0x4000) {
      memory.set_prg_nrom_bottom((uint8_t*) program);
    } else {
      memory.set_prg_nrom_bottom((uint8_t*) program + 0x4000);
    }
    // set PPU CHR memory pointers
    int chr_size = buffer[5] * 8192;
    uint8_t* chr_data = (uint8_t*) program + prg_size;
    if (chr_size > 0) {
      ppu_memory.set_pattern_tables(chr_data);
    }
    cpu.initialize();

    // print_pattern_tables();

  }
}

void NES::print_pattern_tables() {
  // for each tile
  for(int i = 0; i < 512; ++i) {
    int offset = 16 * i; // 16 bytes per tile

    // for each line of each tile
    for(int j = 0; j < 8; ++j) {
      uint8_t data1 = ppu_memory.read(offset + j);
      uint8_t data2 = ppu_memory.read(offset + 8 + j);

      // for each pixel (bit) for each line [this is flipped]
      for(int k = 0; k < 8; ++k) {
        printf("%u", (data1 & 0x1) + ((data2 & 0x1) << 1));
        data1 >>= 1;
        data2 >>= 1;
      }
      printf("\n");
    }
    printf("--------\n");
  }
}

// alternative is to run CPU until PPU latch is
// 'filled' and then step PPU to that point
void NES::run_game() {
  while(cpu.valid) {
    cpu.execute_instruction();
    ppu.step_to(cpu.local_clock * 3); // PPU clock is 3x
  }
}

void NES::play_game(char* filename) {
  create_system();
  load_program(filename);
  run_game();
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << "Specify a filename\n";
    return 1;
  }
  NES nes;
  nes.play_game(argv[1]);
}
