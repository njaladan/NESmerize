#include <stdio.h>
#include <stdint.h>


// hardwired for NROM (mapper 0) at the moment
class Memory {
  public:
    uint8_t internal_ram[0x800];
    uint8_t ppu_reg[0x8];
    uint8_t apu_io_registers[0x20];
    uint8_t prg_nrom[0xbfe0];

    uint8_t get_item(uint16_t);
    void set_item(uint16_t, uint8_t);
    void print_memory();

    // get pointer (can potentially overflow if assembly is bad)
    uint8_t* get_pointer(uint16_t);

};

// assumes ind positive, fix this!
uint8_t* Memory::get_pointer(uint16_t ind) {
  if (ind < 0x2000) {
    return (internal_ram + ind % 800);
  } else if (ind < 0x4000) {
    // modulo using mask is faster
    return (ppu_reg + (ind & 0x8));
  } else if (ind < 0x4020) {
    return (apu_io_registers + (ind & 0xff));
  } else {
    // when ind >= 0x4020 -> NROM
    // does NOT take PRG RAM mirroring into consideration (!!)
    return (prg_nrom + (ind - 0x4020));
  }
}

uint8_t Memory::get_item(uint16_t ind) {
  return *get_pointer(ind);
}

void Memory::set_item(uint16_t ind, uint8_t val) {
  *(get_pointer(ind)) = val;
}
