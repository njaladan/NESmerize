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

struct SpriteAttributes {
  uint8_t palette : 2;
  uint8_t unimplemented : 3;
  bool priority : 1;
  bool flip_horizontal : 1;
  bool flip_vertical : 1;
};

class PPU {
  public:
  // hardware connections
  Memory* memory;
  CPU* cpu;
  PPUMemory* ppu_memory;
  GUI* gui;

  uint16_t previous_tick;
  uint16_t previous_scanline;
  uint16_t current_scanline;
  uint16_t current_tick;
  uint64_t local_clock; // this is 3x the cpu one

  // registers
  union Reg2000 reg2000;
  union Reg2001 reg2001;
  union Reg2002 reg2002;
  uint8_t oamaddr;
  uint8_t oamdata;
  uint8_t scroll_offset;
  uint8_t ppuaddr;
  uint8_t ppudata;

  // visual
  uint8_t framebuffer[256 * 240 * 4];
  int frames;

  // latches
  uint16_t total_ppuaddr; // 0 means not set?

  void set_memory(Memory*);
  void set_ppu_memory(PPUMemory*);
  void set_cpu(CPU*);
  void set_gui(GUI*);
  void step_to(uint64_t);
  void run_cycle();
  void initialize();
  uint16_t get_current_cycle();
  uint16_t get_current_scanline();


  uint8_t read_register(uint8_t);
  void write_register(uint8_t, uint8_t);
  void write_background_tile_palette(Color*, uint8_t, uint8_t);
  void write_sprite_tile_palette(Color*, uint8_t);
  void write_to_framebuffer(uint8_t*, uint8_t, uint8_t, Color);
};
