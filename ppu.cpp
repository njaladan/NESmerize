

void PPU::set_memory(Memory* mem_pointer) {
  memory = mem_pointer;
}

void PPU::set_ppu_memory(PPUMemory* mem_pointer) {
  ppu_memory = mem_pointer;
}

void PPU::set_cpu(CPU* cpu_pointer) {
  cpu = cpu_pointer;
}

void PPU::set_gui(GUI* gui_ptr) {
  gui = gui_ptr;
}

void PPU::initialize() {
  previous_scanline = 241;
  previous_tick = 0;
  local_clock = 0;
  current_tick = 0;
  current_scanline = 0;
  reg2000.value = 0;
  reg2001.value = 0;
  reg2002.value = 0;
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
      total_ppuaddr = (ppuaddr << 8) | val;
      ppudata = ppu_memory->read(total_ppuaddr);
      ppuaddr = val;
      break;

    case 7:
      ppu_memory->write(total_ppuaddr, val);
      if (reg2000.reg_data.vram_address_increment) {
        total_ppuaddr += 32;
      } else {
        total_ppuaddr += 1;
      }
      break;
  }
}

// TODO: odd frame skip tick
uint16_t PPU::get_current_cycle() {
  return local_clock % 341;
}

uint16_t PPU::get_current_scanline() {
  return (241 + local_clock / 341) % 262;
}

void PPU::run_cycle() {
  current_scanline = get_current_scanline();
  current_tick = get_current_cycle();



}

void PPU::step_to(uint64_t cycle) {
  local_clock = cycle;
  current_scanline = get_current_scanline();
  current_tick = get_current_cycle();
  // new scanline
  if (current_tick < previous_tick) {
    // vblank has started
    if (current_scanline == 241) {
      if (reg2000.reg_data.generate_nmi) {
        cpu->generate_nmi();
      }
      reg2002.reg_data.vblank = true;
    }

    // TODO: have cycle-accurate memory accesses & render during "VBlank" LOL
    if (current_scanline == 0) {
      // TODO: separate bg rendering into another function
      if (reg2001.reg_data.background) {
        struct Color palette[4];
        // render background
        for (int i = 0; i < 30; ++i) {
          for (int j = 0; j < 32; ++j) {
            uint8_t chr_ind = ppu_memory->read(0x2000 + i * 0x20 + j);
            int memory_ind = chr_ind * 16 + 0x1000 * reg2000.reg_data.bg_pattern_table_address;
            write_background_tile_palette(palette, j << 3, i << 3);
            // each line of tile
            for (int k = 0; k < 8; ++k) {
              uint8_t lower_data = ppu_memory->read(memory_ind + k);
              uint8_t upper_data = ppu_memory->read(memory_ind + 8 + k);
              // each pixel of each line
              for (int l = 7; l >= 0; l--) {
                uint8_t palette_ind = ((upper_data & 0x1) << 1) | (lower_data & 0x1);
                write_to_framebuffer(framebuffer, 8 * j + l, 8 * i + k, palette[palette_ind]);
                upper_data >>= 1;
                lower_data >>= 1;
              }
            }
          }
        }
      }
      // TODO: separate sprite rendering into another function
      // TODO: 8x16 sprite size
      if (reg2001.reg_data.sprites) {
        // render sprites
        // TODO: use a bitfield and / or structs for this
        uint8_t* oam = ppu_memory->oam;
        for (int sprite = 63; sprite >= 0; --sprite) {
          uint8_t y_pos = oam[4 * sprite];
          uint8_t tile_index = oam[4 * sprite + 1];
          // TODO: fix this ugly with union
          struct SpriteAttributes attributes = *(struct SpriteAttributes *) (oam + 4 * sprite + 2);
          uint8_t x_pos = oam[4 * sprite + 3];
          if (y_pos > 0xEF || x_pos > 0xF9) {
            continue; // overflow sprites not shown
          }
          int memory_ind = tile_index * 16 + 0x1000 * reg2000.reg_data.sprite_pattern_table_address;
          struct Color palette[4];
          write_sprite_tile_palette(palette, attributes.palette);
          // each line of each tile
          for (int k = 0; k < 8; ++k) {
            uint8_t lower_data = ppu_memory->read(memory_ind + k);
            uint8_t upper_data = ppu_memory->read(memory_ind + 8 + k);
            // each pixel of each line
            for (int l = 7; l >= 0; l--) {
              int x, y;
              if (attributes.flip_horizontal) {
                x = x_pos + (8 - l);
              } else {
                x = x_pos + l;
              }
              if (attributes.flip_vertical) {
                y = y_pos + (8 - k);
              } else {
                y = y_pos + k;
              }
              uint8_t palette_ind = ((upper_data & 0x1) << 1) | (lower_data & 0x1);
              if (palette_ind != 0) {
                write_to_framebuffer(framebuffer, x, y, palette[palette_ind]);
              }
              upper_data >>= 1;
              lower_data >>= 1;
            }
          }
        }
      }
      frames++;
      gui->render_frame(framebuffer);
    }
  }
  previous_scanline = current_scanline;
  previous_tick = current_tick;
}

void PPU::write_sprite_tile_palette(struct Color* palette, uint8_t palette_ind) {
  uint8_t color_ind;
  // 0 color is transparent
  for (int i = 1; i < 4; ++i) {
    color_ind = ppu_memory->read(0x3f10 | (palette_ind << 2) | i);
    palette[i] = PALETTE[color_ind];
  }
}

void PPU::write_background_tile_palette(struct Color* palette, uint8_t x, uint8_t y) {
  uint8_t color_ind;
  for (int i = 0; i < 4; ++i) {
    if (i == 0) {
      color_ind = ppu_memory->read(0x3f00);
      palette[0] = PALETTE[color_ind];
      continue;
    }
    uint8_t attribute_offset = ((y >> 5) << 3) + (x >> 5);
    uint8_t attribute = ppu_memory->read(0x23c0 + attribute_offset);
    uint8_t x_offset = (x >> 4) & 0x1;
    uint8_t y_offset = (y >> 4) & 0x1;
    uint8_t total_offset = (y_offset << 2) | (x_offset << 1);
    uint8_t palette_ind = (attribute >> total_offset) & 0x3;
    color_ind = ppu_memory->read(0x3f00 | (palette_ind << 2) | i);
    palette[i] = PALETTE[color_ind];
  }
}

void PPU::write_to_framebuffer(uint8_t* framebuffer, uint8_t x, uint8_t y, struct Color color) {
  int index = 4 * (y * 256 + x); // 4 bytes per pixel
  framebuffer[index] = color.blue;
  framebuffer[index + 1] = color.green;
  framebuffer[index + 2] = color.red;
  framebuffer[index + 3] = 0xff; // no opacity
}
