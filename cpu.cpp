#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>


#include "memory.cpp"

using namespace std;

class CPU {
  // TODO: move to private after debugging
  public:
    // CPU registers; reset should initialize to zero unless set
    uint8_t accumulator;
    uint8_t X;
    uint8_t Y;
    uint16_t PC;
    uint8_t SP;

    // CPU flags
    bool carry;
    bool zero;
    bool interrupt_disable;
    bool overflow;
    bool sign;
    bool decimal; // has no effect on NES 6502 bc no decimal mode included
    // 2-bit B flag should be here, but I don't think it has an effect on the CPU

    // debugging
    bool valid;

    // "hardware" connections
    Memory memory;

    // running stuff
    void load_program(char*, int);
    void execute_cycle();
    void run();


    // memory functions
    void stack_push(uint16_t);
    uint16_t stack_pop();


    // opcode implementation
    void compare(uint8_t, uint8_t);
    void load_flags(uint8_t);

    // addressing modes
    uint8_t* zero_page_indexed_X(uint8_t);
    uint8_t* zero_page_indexed_Y(uint8_t);
    uint8_t* absolute_indexed_X(uint16_t);
    uint8_t* absolute_indexed_Y(uint16_t);
    uint8_t* indexed_indirect(uint8_t);
    uint8_t* indirect_indexed(uint8_t);
    uint16_t indirect(uint8_t, uint8_t);

};

// ideally loading would mean a pointer but I'll keep this for now, sigh
// not correct b/c bank switching means needless IOs instead of a simple pointer swap LMAOO
void CPU::load_program(char* program, int size) {
  bool mirror = false;
  if (size == 0x4000) {
    mirror = true;
  }
  int program_offset = 0x4000;
  for (int i = 0; i < size; i++) {
    memory.set_item(0x4000 + i, *(program + i));
    if (mirror) {
      memory.set_item(0xC000 + i, *(program + i));
    }
  }
  // need to cast to (uint16_t) ?
  PC = (memory.get_item(0xfffd) << 8) | memory.get_item(0xfffc);
  SP = 0xff;
  valid = true;
  printf("intializing PC with %x\n", PC);
}

void CPU::stack_push(uint16_t addr) {
  uint16_t stack_offset = 0x100;
  uint8_t lower_byte = (uint8_t) addr; // does this cast work?
  uint8_t upper_byte = (uint8_t) addr >> 8;
  memory.set_item(SP, upper_byte);
  memory.set_item(SP - 1, lower_byte);
  SP = SP - 2;
}

// check if little / big endian makes a difference
uint16_t CPU::stack_pop() {
  uint16_t stack_offset = 0x100;
  uint8_t lower_byte = memory.get_item(stack_offset + SP + 1);
  uint8_t upper_byte = memory.get_item(stack_offset + SP + 2);
  uint16_t val = upper_byte << 8 | lower_byte;
  SP = SP + 2;
}

// basically one giant switch case
// how can i make this better?
void CPU::execute_cycle() {
  uint8_t opcode = memory.get_item(PC);
   // these may lead to errors if we're at the end of PRG
  uint8_t arg1 = memory.get_item(PC + 1);
  uint8_t arg2 = memory.get_item(PC + 2);
  uint16_t address = arg2 << 8 | arg1;
  printf("%x: %x %x %x\n", PC, opcode, arg1, arg2);

  switch(opcode) {

    // JSR: a
    case 0x20:
      stack_push(PC + 2); // push next address - 1 to stack
      PC = address;
      break;

    // RTS
    case 0x60:
      PC = stack_pop() + 1; // add one to stored address
      break;

    // NOP
    case 0xea:
      PC += 2;
      break;

    // CMP: i
    case 0xc9:
      compare(accumulator, arg1);
      PC += 2;
      break;

    // CMP: zero page
    case 0xc5:
      compare(accumulator, memory.get_item(arg1));
      PC += 2;
      break;

    // CMP: zero page, X
    case 0xd5:
      compare(accumulator, *(zero_page_indexed_X(arg1)));
      PC += 2;
      break;

    // CMP: absolute
    case 0xcd:
      compare(accumulator, memory.get_item(address));
      PC += 3;
      break;

    // CMP: absolute, X
    case 0xdd:
      compare(accumulator, *(absolute_indexed_X(address)));
      PC += 3;
      break;

    // CMP: absolute, Y
    case 0xd9:
      compare(accumulator, *(absolute_indexed_Y(address)));
      PC += 3;
      break;

    // CMP: indirect, X
    case 0xc1:
      compare(accumulator, *(indexed_indirect(arg1)));
      PC += 2;
      break;

    // CMP: indirect, Y
    case 0xd1:
      compare(accumulator, *(indirect_indexed(arg1)));
      PC += 2;
      break;

    // CPX: immediate
    case 0xe0:
      compare(X, arg1);
      PC += 2;
      break;

    // CPX: zero page
    case 0xe4:
      compare(X, memory.get_item(arg1));
      PC += 2;
      break;

    // CPX: absolute
    case 0xec:
      compare(X, memory.get_item(address));
      PC += 3;
      break;

    // CPY: immediate
    case 0xc0:
      compare(Y, arg1);
      PC += 2;
      break;

    // CPX: zero page
    case 0xc4:
      compare(Y, memory.get_item(arg1));
      PC += 2;
      break;

    // CPX: absolute
    case 0xcc:
      compare(Y, memory.get_item(address));
      PC += 3;
      break;

    // BPL
    case 0x10:
      if (!sign) {
        PC += ((int8_t) arg1) + 2;
      } else {
        PC += 2;
      }
      break;

    // BMI
    case 0x30:
      if (sign) {
        PC += ((int8_t) arg1) + 2;
      } else {
        PC += 2;
      }
      break;

    // BVC
    case 0x50:
      if (!overflow) {
        PC += ((int8_t) arg1) + 2;
      } else {
        PC += 2;
      }
      break;

    // BVS
    case 0x70:
      if (overflow) {
        PC += ((int8_t) arg1) + 2;
      } else {
        PC += 2;
      }
      break;

    // BCC
    case 0x90:
      if (!carry) {
        PC += ((int8_t) arg1) + 2;
      } else {
        PC += 2;
      }
      break;

    // BCS
    case 0xB0:
      if (carry) {
        PC += ((int8_t) arg1) + 2;
      } else {
        PC += 2;
      }
      break;

    // BNE
    case 0xD0:
      if (!zero) {
        PC += ((int8_t) arg1) + 2;
      } else {
        PC += 2;
      }
      break;

    // BEQ
    case 0xF0:
      if (zero) {
        PC += ((int8_t) arg1) + 2;
      } else {
        PC += 2;
      }
      break;

    // LDA immediate
    case 0xa9:
      accumulator = arg1;
      load_flags(accumulator);
      PC += 2;
      break;

    // LDA zero page
    case 0xa5:
      accumulator = memory.get_item(arg1);
      load_flags(accumulator);
      PC += 2;
      break;

    // LDA zero page X
    case 0xb5:
      accumulator = *(zero_page_indexed_X(arg1));
      load_flags(accumulator);
      PC += 2;
      break;

    // LDA absolute
    case 0xad:
      accumulator = memory.get_item(address);
      load_flags(accumulator);
      PC += 3;
      break;

    // LDA Absolute X
    case 0xbd:
      accumulator = *(absolute_indexed_X(address));
      load_flags(accumulator);
      PC += 3;
      break;

    // LDA absolute Y
    case 0xb9:
      accumulator = *(absolute_indexed_Y(address));
      load_flags(accumulator);
      PC += 3;
      break;

    // LDA indirect X
    case 0xa1:
      accumulator = *(indexed_indirect(arg1));
      load_flags(accumulator);
      PC += 2;
      break;

    // LDA indirect Y
    case 0xb1:
      accumulator = *(indirect_indexed(arg1));
      load_flags(accumulator);
      PC += 2;
      break;

    // LDX immediate
    case 0xa2:
      X = arg1;
      load_flags(X);
      PC += 2;
      break;

    // LDX zero page
    case 0xa6:
      X = memory.get_item(arg1);
      load_flags(X);
      PC += 2;
      break;

    // LDX zero page Y
    case 0xb6:
      X = *(zero_page_indexed_Y(arg1));
      load_flags(X);
      PC += 2;
      break;

    // LDX absolute
    case 0xae:
      X = memory.get_item(address);
      load_flags(X);
      PC += 3;
      break;

    // LDX absolute Y
    case 0xbe:
      X = *(absolute_indexed_Y(address));
      load_flags(X);
      PC += 3;
      break;

    // LDY immediate
    case 0xa0:
      Y = arg1;
      load_flags(Y);
      PC += 2;
      break;

    // LDY zero page
    case 0xa4:
      Y = memory.get_item(arg1);
      load_flags(Y);
      PC += 2;
      break;

    // LDY zero page X
    case 0xb4:
      Y = *(zero_page_indexed_X(arg1));
      load_flags(Y);
      PC += 2;
      break;

    // LDY absolute
    case 0xac:
      Y = memory.get_item(address);
      load_flags(Y);
      PC += 3;
      break;

    // LDY absolute X
    case 0xbc:
      Y = *(absolute_indexed_X(address));
      load_flags(Y);
      PC += 3;
      break;

    // STA zero page
    case 0x85:
      memory.set_item(arg1, accumulator);
      PC += 2;
      break;

    // STA zero page X
    case 0x95:
      *(zero_page_indexed_X(arg1)) = accumulator;
      PC += 2;
      break;

    // STA absolute
    case 0x8d:
      memory.set_item(address, accumulator);
      PC += 3;
      break;

    // STA absolute X
    case 0x9d:
      *(absolute_indexed_X(address)) = accumulator;
      PC += 3;
      break;

    // STA absolute Y
    case 0x99:
      *(absolute_indexed_Y(address)) = accumulator;
      PC += 3;
      break;

    // STA indirect X
    case 0x81:
      *(indexed_indirect(arg1)) = accumulator;
      PC += 2;
      break;

    // STA indirect Y
    case 0x91:
      *(indirect_indexed(arg1)) = accumulator;
      PC += 2;
      break;

    // STX zero page
    case 0x86:
      memory.set_item(arg1, X);
      PC += 2;
      break;

    // STX zero page Y
    case 0x96:
      *(zero_page_indexed_Y(arg1)) = X;
      PC += 2;
      break;

    // STX absolute
    case 0x8E:
      memory.set_item(address, X);
      PC += 3;
      break;

    // STY zero page
    case 0x84:
      memory.set_item(arg1, Y);
      PC += 2;
      break;

    // STY zero page X
    case 0x94:
      *(zero_page_indexed_X(arg1)) =  Y;
      PC += 2;
      break;

    // STY absolute
    case 0x8c:
      memory.set_item(address, Y);
      PC += 3;
      break;

    // JMP absolute
    case 0x4c:
      PC = address;
      break;

    // JMP indirect
    case 0x6c:
      PC = indirect(arg1, arg2);
      break;

    // CLC (clear carry)
    case 0x18:
      carry = false;
      PC += 1;
      break;

    // SEC (set carry)
    case 0x38:
      carry = true;
      PC += 1;
      break;

    // CLI (clear interrupt)
    case 0x58:
      interrupt_disable = false;
      PC += 1;
      break;

    // SEI (set interrupt)
    case 0x78:
      interrupt_disable = true;
      PC += 1;
      break;

    // CLV (clear overflow)
    case 0xb8:
      overflow = false;
      PC += 1;
      break;

    // CLD (clear decimal)
    case 0xd8:
      decimal = false;
      PC += 1;
      break;

    // SED (set decimal)
    case 0xf8:
      decimal = true;
      PC += 1;
      break;

    // BIT zero page
    case 0x24:
      zero = ((memory.get_item(arg1) & accumulator) == 0);
      overflow = (memory.get_item(arg1) >> 6) & 1;
      sign = (memory.get_item(arg1) >> 7) & 1;
      PC += 2;
      break;

    // BIT absolute
    case 0x2c:
      zero = ((memory.get_item(address) & accumulator) == 0);
      overflow = (memory.get_item(address) >> 6) & 1;
      sign = (memory.get_item(address) >> 7) & 1;
      PC += 3;
      break;




    default:
      printf("this instruction is not here lmao: %x\n", opcode);
      valid = false;
      break;
  }
}

void CPU::run() {
  while(valid) {
    execute_cycle();
  }
}

void CPU::compare(uint8_t operand, uint8_t argument) {
  carry = (operand >= argument);
  zero = (operand == argument);
  sign = (operand >= 0x80); // positive two's complement
}

void CPU::load_flags(uint8_t val) {
  sign = (val >= 0x80);
  zero = (val == 0);
}

// ADDRESSING MODES POINTER

uint8_t* CPU::zero_page_indexed_X(uint8_t arg) {
  return memory.get_pointer((arg + X) & 0x100);
}

uint8_t* CPU::zero_page_indexed_Y(uint8_t arg) {
  return memory.get_pointer((arg + Y) & 0x100);
}

uint8_t* CPU::absolute_indexed_X(uint16_t arg) {
  return memory.get_pointer(arg + X);
}

uint8_t* CPU::absolute_indexed_Y(uint16_t arg) {
  return memory.get_pointer(arg + Y);
}

uint8_t* CPU::indexed_indirect(uint8_t arg) {
  uint16_t low_byte = (uint16_t) memory.get_item((arg + X) & 0x100);
  uint16_t high_byte = memory.get_item((arg + X + 1) & 0x100) << 8;
  return memory.get_pointer(high_byte | low_byte);
}

uint8_t* CPU::indirect_indexed(uint8_t arg) {
  uint16_t low_byte = (uint16_t) memory.get_item(arg);
  uint16_t high_byte = (uint16_t) memory.get_item((arg + 1) & 0x100);
  return memory.get_pointer((high_byte | low_byte) + Y);
}

uint16_t CPU::indirect(uint8_t arg1, uint8_t arg2) {
  uint16_t lower_byte_address = arg1 << 8 | arg2;
  uint16_t upper_byte_address = arg1 << 8 | ((arg2 + 1) & 0xff); // because last byte overflow won't carry over
  uint8_t lower_byte = memory.get_item(lower_byte_address);
  uint8_t upper_byte = memory.get_item(upper_byte_address);
  uint16_t new_pc_address = (upper_byte << 8) | lower_byte;
  return new_pc_address;
}

int main() {
  CPU nes;
  ifstream rom("read_test.nes", ios::binary | ios::ate);
  streamsize size = rom.tellg();
  rom.seekg(0, std::ios::beg);

  char buffer[size];
  if (rom.read(buffer, size)) {
    printf("Read in ROM data\n");
    int prg_size = buffer[4] * 0x4000; // multiplier
    char* prg_data = buffer + 16; // techncially doesn't count for possibility of trainer but that's ok
    nes.load_program(prg_data, prg_size);
    nes.run();
  }


}
