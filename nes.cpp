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
  }
}

// alternative is to run CPU until PPU latch is
// 'filled' and then step PPU to that point
void NES::run_game() {
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  while(cpu.valid) {
    cpu.execute_instruction();
    ppu.step_to(cpu.local_clock * 3); // PPU clock is 3x
  }
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  auto d = duration_cast<microseconds>(t2 - t1).count();
  cout << d;

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
