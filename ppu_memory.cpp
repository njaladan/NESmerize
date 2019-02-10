#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>


using namespace std;

class PPUMemory {
public:
  uint8_t* pattern_tables;
  uint8_t name_tables[0x1000];
  uint8_t palettes[0x20];
  uint8_t spr_ram[0x100];
  uint8_t oam[0x100];

  // memory operations
  uint8_t* get_pointer(uint16_t);
  uint8_t* get_spr_pointer(uint8_t);
  uint8_t read_oam(uint8_t);
  void set_pattern_tables(uint8_t*);
  void dma_write_oam(uint8_t*);
  void write_oam(uint8_t, uint8_t);
  uint8_t read(uint16_t);
  void write(uint16_t, uint8_t);
};


uint8_t* PPUMemory::get_pointer(uint16_t addr) {
  if (addr < 0x2000) {
    return pattern_tables + addr;
  } else if (addr < 0x3f00) {
    return name_tables + ((addr - 0x2000) & 0xfff);
  } else if (addr < 0x4000) {
    return palettes + ((addr - 0x3f00) & 0x1f);
  } else {
    return get_pointer(addr & 0x3fff);
  }
}

uint8_t PPUMemory::read(uint16_t ind) {
  return *(get_pointer(ind));
}

void PPUMemory::write(uint16_t ind, uint8_t val) {
  *(get_pointer(ind)) = val;
}

uint8_t* PPUMemory::get_spr_pointer(uint8_t addr) {
  return spr_ram + addr;
}

void PPUMemory::set_pattern_tables(uint8_t* pt_pointer) {
  pattern_tables = pt_pointer;
}

void PPUMemory::dma_write_oam(uint8_t* values) {
  for(int i = 0; i < 0x100; ++i) {
    oam[i] = values[i];
  }
}

uint8_t PPUMemory::read_oam(uint8_t ind) {
  return oam[ind];
}

void PPUMemory::write_oam(uint8_t ind, uint8_t val) {
  oam[ind] = val;
}
