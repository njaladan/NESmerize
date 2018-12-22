class PPU {
public:
  // accessible objects
  Memory* memory;
  CPU* cpu;
  PPUMemory* ppu_memory;

  uint64_t local_clock;

  // $2000
  bool x_scroll_offset;
  bool y_scroll_offset;
  bool vram_address_increment;
  bool sprite_pattern_table_address;
  bool bg_pattern_table_address;
  bool sprite_size;
  bool ppu_master_slave;
  bool generate_nmi;

  // $2001
  bool grayscale;
  bool leftmost_background;
  bool leftmost_sprites;
  bool background;
  bool sprites;
  bool red;
  bool green;
  bool blue;

  // $2002
  uint8_t lsb_ppu_write;
  bool sprite_overflow;
  bool sprite_0_hit;
  bool vblank;

  // $2003
  uint8_t oamaddr;

  // $2004
  uint8_t oamdata;

  // $2005
  uint8_t scroll_offset;

  // $2006
  uint8_t ppuaddr;

  // $2007
  uint8_t ppudata;

  // $4014
  uint8_t oamdma;



  void set_memory(Memory*);
  void set_ppu_memory(PPUMemory*);
  void step_to(uint64_t);


};

void PPU::set_memory(Memory* mem_pointer) {
  memory = mem_pointer;
}

void PPU::set_ppu_memory(PPUMemory* mem_pointer) {
  ppu_memory = mem_pointer;
}

void PPU::step_to(uint64_t cycle) {

}
