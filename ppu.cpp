

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
      if (reg2001.reg_data.background) {
        // literally render entire frame as ascii LOL
        for (int i = 0; i < 30; ++i) {
          for (int j = 0; j < 32; ++j) {
            uint8_t chr_ind = ppu_memory->read(0x2000 + i * 0x20 + j);
            int memory_ind = chr_ind * 16 + 0x1000 * reg2000.reg_data.bg_pattern_table_address;
            // each line of tile
            for (int k = 0; k < 8; ++k) {
              uint8_t lower_data = ppu_memory->read(memory_ind + k);
              uint8_t upper_data = ppu_memory->read(memory_ind + 8 + k);
              // each pixel of each line
              for (int l = 7; l >= 0; l--) {
                uint8_t palette_offset = ((upper_data & 0x1) << 1) | (lower_data & 0x1);
                write_to_framebuffer(framebuffer, 8 * j + l, 8 * i + k, palette_offset);
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
  }
  previous_scanline = current_scanline;
  previous_tick = current_tick;
}

void PPU::write_to_framebuffer(uint8_t* framebuffer, uint8_t x, uint8_t y, uint8_t palette_offset) {
  // background color
  uint8_t color_ind;
  if (palette_offset == 0) {
    color_ind = ppu_memory->read(0x3f00);
  } else {
    uint8_t attribute_offset = ((y >> 5) << 3) + (x >> 5);
    uint8_t attribute = ppu_memory->read(0x23c0 + attribute_offset);
    uint8_t x_offset = (x >> 4) & 0x1;
    uint8_t y_offset = (y >> 4) & 0x1;
    uint8_t total_offset = (y_offset << 2) | (x_offset << 1);
    uint8_t palette_ind = (attribute >> total_offset) & 0x3;
    color_ind = ppu_memory->read(0x3f00 | (palette_ind << 2) | palette_offset);
  }

  int index = 4 * (y * 256 + x); // 4 bytes per pixel
  struct Color color = PALETTE[color_ind];

  framebuffer[index] = color.blue;
  framebuffer[index + 1] = color.green;
  framebuffer[index + 2] = color.red;
  framebuffer[index + 3] = 0xff; // no opacity
}
