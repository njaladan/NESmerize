#include <stdio.h>
#include <stdint.h>

class Memory {
  public:
    uint8_t internal_ram[0x800];
    uint8_t ppu_reg[0x8];
    uint8_t apu_io_registers[0x20];
    uint8_t blank[0x3fe0];
    uint8_t* prg_nrom_top;
    uint8_t* prg_nrom_bottom;

    uint8_t read(uint16_t);
    void write(uint16_t, uint8_t);
    void print_memory();

    uint8_t* get_pointer(uint16_t);

    // vectors
    uint16_t reset_vector();
    uint16_t irq_vector();
    uint16_t nmi_vector();

    // mapper 0 specific functioanlity
    void set_prg_nrom_bottom(uint8_t*);
    void set_prg_nrom_top(uint8_t*);

    // hardware connections
    CPU* cpu;
    PPUMemory* ppumem;
    PPU* ppu;
    void set_cpu(CPU*);
    void set_ppu_memory(PPUMemory*);
    void set_ppu(PPU*);
};


void Memory::set_cpu(CPU* cpu_pointer) {
  cpu = cpu_pointer;
}

void Memory::set_ppu_memory(PPUMemory* ppu_mem_pointer) {
  ppumem = ppu_mem_pointer;
}

void Memory::set_ppu(PPU* ppu_pointer) {
  ppu = ppu_pointer;
}

// TODO: evaluate if this can be private? or delet this
uint8_t* Memory::get_pointer(uint16_t ind) {
  if (ind < 0x2000) {
    return (internal_ram + ind % 0x7ff);
  } else if (ind < 0x4000) {
    return (ppu_reg + (ind & 0x7));
  } else if (ind < 0x4020) {
    return (apu_io_registers + (ind & 0xff));
  } else if (ind < 0x8000) {
    return blank;
  } else if (ind < 0xc000) {
    return prg_nrom_top + (ind - 0x8000);
  } else {
    return prg_nrom_bottom + (ind - 0xc000);
  }
}

void Memory::set_prg_nrom_top(uint8_t* rom_pointer) {
  prg_nrom_top = rom_pointer;
}

void Memory::set_prg_nrom_bottom(uint8_t* rom_pointer) {
  prg_nrom_bottom = rom_pointer;
}

// set checks here
uint8_t Memory::read(uint16_t ind) {
  if (ind >= 0x2000 && ind < 0x4000) {
    return ppu->read_register(ind & 0x7);
  }
  return *get_pointer(ind);
}

void Memory::write(uint16_t ind, uint8_t val) {
  if (ind == 0x4014) {
    ppumem->dma_write_oam(get_pointer(val * 0x100));
    cpu->local_clock += 513;
  } else if (ind >= 0x2000 && ind < 0x4000) {
    ppu->write_register(ind & 0x7, val);
    return;
  }
  *(get_pointer(ind)) = val;
}

uint16_t Memory::reset_vector() {
  return (read(0xfffd) << 8) | read(0xfffc);
}

uint16_t Memory::nmi_vector() {
  return (read(0xfffb) << 8) | read(0xfffa);
}

uint16_t Memory::irq_vector() {
  return (read(0xffff) << 8) | read(0xfffe);
}
