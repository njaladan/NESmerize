class Memory;
class CPU;

struct Reg2000Data {
  bool x_scroll_offset : 1;
  bool y_scroll_offset : 1;
  bool vram_address_increment : 1;
  bool sprite_pattern_table_address : 1;
  bool bg_pattern_table_address : 1;
  bool sprite_size : 1;
  bool ppu_master_slave : 1;
  bool generate_nmi : 1;
};

union Reg2000 {
  uint8_t value;
  Reg2000Data reg_data;
};

struct Reg2001Data {
  bool grayscale : 1;
  bool leftmost_background : 1;
  bool leftmost_sprites : 1;
  bool background : 1;
  bool sprites : 1;
  bool red : 1;
  bool green : 1;
  bool blue : 1;
};

union Reg2001 {
  uint8_t value;
  Reg2001Data reg_data;
};

struct Reg2002Data {
  uint8_t lsb_ppu_write : 5;
  bool sprite_overflow : 1;
  bool sprite_0 : 1;
  bool vblank : 1;
};

union Reg2002 {
  uint8_t value;
  Reg2002Data reg_data;
};


class PPU {
  public:
  // hardware connections
  Memory* memory;
  CPU* cpu;
  PPUMemory* ppu_memory;

  uint16_t previous_tick;
  uint16_t previous_scanline;

  // registers
  union Reg2000 reg2000;
  union Reg2001 reg2001;
  union Reg2002 reg2002;
  uint8_t oamaddr;
  uint8_t oamdata;
  uint8_t scroll_offset;
  uint8_t ppuaddr;
  uint8_t ppudata;

  // latches
  int8_t address_latch; // -1 means not set?

  void set_memory(Memory*);
  void set_ppu_memory(PPUMemory*);
  void set_cpu(CPU*);
  void step_to(uint64_t);
  void initialize();

  uint8_t read_register(uint8_t);
  void write_register(uint8_t, uint8_t);
};

void PPU::set_memory(Memory* mem_pointer) {
  memory = mem_pointer;
}

void PPU::set_ppu_memory(PPUMemory* mem_pointer) {
  ppu_memory = mem_pointer;
}

void PPU::set_cpu(CPU* cpu_pointer) {
  cpu = cpu_pointer;
}

void PPU::initialize() {
  previous_scanline = 241;
  previous_tick = 0;
}

uint8_t PPU::read_register(uint8_t reg) {
  switch (reg) {
    case 0:
      return reg2000.value;

    case 1:
      return reg2001.value;

    case 2: {
      uint8_t value = reg2002.value;
      reg2002.reg_data.vblank = false;
      address_latch = -1;
      return value;
    };

    case 3:
      return oamaddr;

    case 4:
      return oamdata;

    case 5:
      return scroll_offset;

    case 6:
      return ppuaddr;

    case 7:
      return ppudata;
  }
}

void PPU::write_register(uint8_t reg, uint8_t val) {
  reg2002.reg_data.lsb_ppu_write = val & 0x1f;
  switch (reg) {
    case 0:
      reg2000.value = val;
      break;

    case 1:
      reg2001.value = val;
      break;

    case 2:
      break;

    case 3:
      oamdata = ppu_memory->read_oam(val);
      oamaddr = val;
      break;

    case 4:
      ppu_memory->write_oam(oamaddr, val);
      oamaddr += 1;
      break;

    case 5:
      break;

    case 6:
      break;

    case 7:
      break;
  }
}


void PPU::step_to(uint64_t cycle) {
  uint16_t current_scanline = previous_scanline;
  uint16_t current_tick = cycle % 341;
  // new scanline
  if (current_tick < previous_tick) {
    current_scanline = (previous_scanline + 1) % 262;
    if (current_scanline == 241) {
      if (reg2000.reg_data.generate_nmi) {
        cpu->generate_nmi();
      }

      reg2002.reg_data.vblank = true;
    }
  }
  previous_scanline = current_scanline;
  previous_tick = current_tick;
  // TODO: do the frame skip thing
  if (current_scanline >= 240) {
    return;
  }
  // do render stuff here
}
