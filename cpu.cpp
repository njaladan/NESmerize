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
    // only holds lower byte of the real SP
    uint8_t SP;

    // cycle count
    uint8_t cycles;

    // CPU flags
    bool carry;
    bool zero;
    bool interrupt_disable;
    bool overflow;
    bool sign;
    bool decimal; // has no effect on NES 6502 bc no decimal mode included
    bool b_upper;
    bool b_lower;

    // interrupt handler
    bool interrupt;

    // for debugging only
    bool valid;

    // "hardware" connections
    Memory memory;

    // running stuff
    void load_program(char*, int);
    void execute_cycle();
    void run();

    // flag operations
    uint8_t get_flags_as_byte();
    void set_flags_from_byte(uint8_t);


    // memory functions
    void stack_push(uint8_t);
    uint8_t stack_pop();

    void address_stack_push(uint16_t);
    uint16_t address_stack_pop();

    // debugging
    void print_register_values();


    // opcode implementation
    void compare(uint8_t, uint8_t);
    void sign_zero_flags(uint8_t);
    void rotate_left(uint8_t*);
    void rotate_right(uint8_t*);
    void shift_right(uint8_t*);
    void arithmetic_shift_left(uint8_t*);
    void add_with_carry(uint8_t);
    void branch_on_bool(bool, int8_t);
    void increment_subtract(uint8_t*);
    void arithmetic_shift_left_or(uint8_t*);
    void rotate_left_and(uint8_t*);
    void shift_right_xor(uint8_t*);
    void rotate_right_add(uint8_t*);

    // addressing modes
    uint8_t* zero_page(uint8_t);
    uint8_t* memory_absolute(uint16_t);
    uint8_t* zero_page_indexed_X(uint8_t);
    uint8_t* zero_page_indexed_Y(uint8_t);
    uint8_t* absolute_indexed_X(uint16_t);
    uint8_t* absolute_indexed_Y(uint16_t);
    uint8_t* indexed_indirect(uint8_t);
    uint8_t* indirect_indexed(uint8_t);
    uint16_t indirect(uint8_t, uint8_t);

    // addressing mode enum
    enum AddressingMode {
      ZERO_PAGE_INDEXED_X,
      ZERO_PAGE_INDEXED_Y,
      ABSOLUTE_INDEXED_X,
      ABSOLUTE_INDEXED_Y,
      INDEXED_INDIRECT,
      INDIRECT_INDEXED,
      ACCUMULATOR,
      IMMEDIATE,
      ZERO_PAGE,
      ABSOLUTE,
      RELATIVE,
      INDIRECT
    };

};

void CPU::address_stack_push(uint16_t addr) {
  uint8_t lower_byte = (uint8_t) addr; // does this cast work?
  uint8_t upper_byte = ((uint16_t) addr) >> 8;
  stack_push(upper_byte);
  stack_push(lower_byte);
}

// check if little / big endian makes a difference?
uint16_t CPU::address_stack_pop() {
  uint8_t lower_byte = stack_pop();
  uint16_t upper_byte = (uint16_t) stack_pop();
  uint16_t val = ((uint16_t) upper_byte << 8) | lower_byte;
  return val;
}

// TODO: make stack_offset a global variable or something?
void CPU::stack_push(uint8_t value) {
  uint16_t stack_offset = 0x100;
  memory.set_item(stack_offset + SP, value);
  // simulate underflow
  SP -= 1;
}

uint8_t CPU::stack_pop() {
  uint16_t stack_offset = 0x100;
  uint16_t stack_address = stack_offset + SP + 1;
  uint8_t value = memory.get_item(stack_address);
  SP += 1;
  return value;
}

void CPU::print_register_values() {
  printf("PC:%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", PC, accumulator, X, Y, get_flags_as_byte(), SP);
}

// basically one giant switch case
// how can i make this better?
void CPU::execute_cycle() {
  uint8_t opcode = memory.get_item(PC);
   // these may lead to errors if we're at the end of PRG
  uint8_t arg1 = memory.get_item(PC + 1);
  uint8_t arg2 = memory.get_item(PC + 2);
  uint16_t address = arg2 << 8 | arg1;
  print_register_values();

  // used to avoid scope change in switch - is there a better way to do this?
  uint8_t temp;
  uint8_t* temp_pointer;

  switch(opcode) {

    // JSR: a
    case 0x20:
      address_stack_push(PC + 2); // push next address - 1 to stack
      PC = address;
      cycles += 6;
      break;

    // RTS
    case 0x60:
      PC = address_stack_pop() + 1; // add one to stored address
      cycles += 6;
      break;

    // NOP
    case 0xea:
      PC += 1;
      cycles += 2;
      break;

    // CMP: i
    case 0xc9:
      compare(accumulator, arg1);
      PC += 2;
      break;

    // CMP: zero page
    case 0xc5:
      compare(accumulator, *(zero_page(arg1)));
      PC += 2;
      break;

    // CMP: zero page, X
    case 0xd5:
      compare(accumulator, *(zero_page_indexed_X(arg1)));
      PC += 2;
      break;

    // CMP: absolute
    case 0xcd:
      compare(accumulator, *(memory_absolute(address)));
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
      compare(X, *(zero_page(arg1)));
      PC += 2;
      break;

    // CPX: absolute
    case 0xec:
      compare(X, *(memory_absolute(address)));
      PC += 3;
      break;

    // CPY: immediate
    case 0xc0:
      compare(Y, arg1);
      PC += 2;
      break;

    // CPY: zero page
    case 0xc4:
      compare(Y, *(zero_page(arg1)));
      PC += 2;
      break;

    // CPY: absolute
    case 0xcc:
      compare(Y, *(memory_absolute(address)));
      PC += 3;
      break;

    // BPL
    case 0x10:
      branch_on_bool(!sign, arg1);
      break;

    // BMI
    case 0x30:
      branch_on_bool(sign, arg1);
      break;

    // BVC
    case 0x50:
      branch_on_bool(!overflow, arg1);
      break;

    // BVS
    case 0x70:
      branch_on_bool(overflow, arg1);
      break;

    // BCC
    case 0x90:
      branch_on_bool(!carry, arg1);
      break;

    // BCS
    case 0xB0:
      branch_on_bool(carry, arg1);
      break;

    // BNE
    case 0xD0:
      branch_on_bool(!zero, arg1);
      break;

    // BEQ
    case 0xF0:
      branch_on_bool(zero, arg1);
      break;

    // LDA immediate
    case 0xa9:
      accumulator = arg1;
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // LDA zero page
    case 0xa5:
      accumulator = *(zero_page(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // LDA zero page X
    case 0xb5:
      accumulator = *(zero_page_indexed_X(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // LDA absolute
    case 0xad:
      accumulator = *(memory_absolute(address));
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // LDA Absolute X
    case 0xbd:
      accumulator = *(absolute_indexed_X(address));
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // LDA absolute Y
    case 0xb9:
      accumulator = *(absolute_indexed_Y(address));
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // LDA indirect X
    case 0xa1:
      accumulator = *(indexed_indirect(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // LDA indirect Y
    case 0xb1:
      accumulator = *(indirect_indexed(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // LDX immediate
    case 0xa2:
      X = arg1;
      sign_zero_flags(X);
      PC += 2;
      break;

    // LDX zero page
    case 0xa6:
      X = *(zero_page(arg1));
      sign_zero_flags(X);
      PC += 2;
      break;

    // LDX zero page Y
    case 0xb6:
      X = *(zero_page_indexed_Y(arg1));
      sign_zero_flags(X);
      PC += 2;
      break;

    // LDX absolute
    case 0xae:
      X = *(memory_absolute(address));
      sign_zero_flags(X);
      PC += 3;
      break;

    // LDX absolute Y
    case 0xbe:
      X = *(absolute_indexed_Y(address));
      sign_zero_flags(X);
      PC += 3;
      break;

    // LDY immediate
    case 0xa0:
      Y = arg1;
      sign_zero_flags(Y);
      PC += 2;
      break;

    // LDY zero page
    case 0xa4:
      Y = *(zero_page(arg1));
      sign_zero_flags(Y);
      PC += 2;
      break;

    // LDY zero page X
    case 0xb4:
      Y = *(zero_page_indexed_X(arg1));
      sign_zero_flags(Y);
      PC += 2;
      break;

    // LDY absolute
    case 0xac:
      Y = *(memory_absolute(address));
      sign_zero_flags(Y);
      PC += 3;
      break;

    // LDY absolute X
    case 0xbc:
      Y = *(absolute_indexed_X(address));
      sign_zero_flags(Y);
      PC += 3;
      break;

    // STA zero page
    case 0x85:
      *(zero_page(arg1)) = accumulator;
      PC += 2;
      break;

    // STA zero page X
    case 0x95:
      *(zero_page_indexed_X(arg1)) = accumulator;
      PC += 2;
      break;

    // STA absolute
    case 0x8d:
      *(memory_absolute(address)) = accumulator;
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
      *(zero_page(arg1)) = X;
      PC += 2;
      break;

    // STX zero page Y
    case 0x96:
      *(zero_page_indexed_Y(arg1)) = X;
      PC += 2;
      break;

    // STX absolute
    case 0x8E:
      *(memory_absolute(address)) = X;
      PC += 3;
      break;

    // STY zero page
    case 0x84:
      *(zero_page(arg1)) = Y;
      PC += 2;
      break;

    // STY zero page X
    case 0x94:
      *(zero_page_indexed_X(arg1)) =  Y;
      PC += 2;
      break;

    // STY absolute
    case 0x8c:
      *(memory_absolute(address)) = Y;
      PC += 3;
      break;

    // JMP absolute
    case 0x4c:
      PC = address;
      cycles += 3;
      break;

    // JMP indirect
    case 0x6c:
      PC = indirect(arg1, arg2);
      cycles += 5;
      break;

    // CLC (clear carry)
    case 0x18:
      carry = false;
      PC += 1;
      cycles += 2;
      break;

    // SEC (set carry)
    case 0x38:
      carry = true;
      PC += 1;
      cycles += 2;
      break;

    // CLI (clear interrupt)
    case 0x58:
      interrupt_disable = false;
      PC += 1;
      cycles += 2;
      break;

    // SEI (set interrupt)
    case 0x78:
      interrupt_disable = true;
      PC += 1;
      cycles += 2;
      break;

    // CLV (clear overflow)
    case 0xb8:
      overflow = false;
      PC += 1;
      cycles += 2;
      break;

    // CLD (clear decimal)
    case 0xd8:
      decimal = false;
      PC += 1;
      cycles += 2;
      break;

    // SED (set decimal)
    case 0xf8:
      decimal = true;
      PC += 1;
      cycles += 2;
      break;

    // BIT zero page
    case 0x24:
      zero = ((memory.get_item(arg1) & accumulator) == 0);
      overflow = (memory.get_item(arg1) >> 6) & 1;
      sign = memory.get_item(arg1) >= 0x80;
      cycles += 3;
      PC += 2;
      break;

    // BIT absolute
    case 0x2c:
      zero = ((memory.get_item(address) & accumulator) == 0);
      overflow = (memory.get_item(address) >> 6) & 1;
      sign = memory.get_item(address) >= 0x80;
      cycles += 4;
      PC += 3;
      break;

    // TYA
    case 0x98:
      accumulator = Y;
      sign_zero_flags(accumulator);
      PC += 1;
      cycles += 2;
      break;

    // TXS
    case 0x9a:
      SP = X;
      PC += 1;
      cycles += 2;
      break;

    // TXA
    case 0x8a:
      accumulator = X;
      sign_zero_flags(accumulator);
      PC += 1;
      cycles += 2;
      break;

    // TSX
    case 0xba:
      X = SP;
      sign_zero_flags(X);
      PC += 1;
      cycles += 2;
      break;

    // TAY
    case 0xa8:
      Y = accumulator;
      sign_zero_flags(Y);
      PC += 1;
      cycles += 2;
      break;

    // TAX
    case 0xaa:
      X = accumulator;
      sign_zero_flags(X);
      PC += 1;
      cycles += 2;
      break;

    // ROR accumulator
    case 0x6a:
      rotate_right(&accumulator);
      PC += 1;
      break;

    // ROR zero page
    case 0x66:
      rotate_right(zero_page(arg1));
      PC += 2;
      break;

    // ROR zero page X
    case 0x76:
      rotate_right(zero_page_indexed_X(arg1));
      PC += 2;
      break;

    // ROR absolute
    case 0x6e:
      rotate_right(memory_absolute(address));
      PC += 3;
      break;

    // ROR absolute X
    case 0x7e:
      rotate_right(absolute_indexed_X(address));
      PC += 3;
      break;

    // ROL accumulator
    case 0x2a:
      rotate_left(&accumulator);
      PC += 1;
      break;

    // ROL zero page
    case 0x26:
      rotate_left(zero_page(arg1));
      PC += 2;
      break;

    // ROL zero page X
    case 0x36:
      rotate_left(zero_page_indexed_X(arg1));
      PC += 2;
      break;

    // ROL absolute
    case 0x2e:
      rotate_left(memory_absolute(address));
      PC += 3;
      break;

    // ROL absolute X
    case 0x3e:
      rotate_left(absolute_indexed_X(address));
      PC += 3;
      break;

    // LSR accumulator
    case 0x4a:
      shift_right(&accumulator);
      PC += 1;
      break;

    // LSR zero page
    case 0x46:
      shift_right(zero_page(arg1));
      PC += 2;
      break;

    // LSR zero page X
    case 0x56:
      shift_right(zero_page_indexed_X(arg1));
      PC += 2;
      break;

    // LSR absolute
    case 0x4e:
      shift_right(memory_absolute(address));
      PC += 3;
      break;

    // LSR absolute X
    case 0x5e:
      shift_right(absolute_indexed_X(address));
      PC += 3;
      break;

    // PLA
    case 0x68:
      accumulator = stack_pop();
      sign_zero_flags(accumulator);
      PC += 1;
      cycles += 4;
      break;

    // PHA
    case 0x48:
      stack_push(accumulator);
      PC += 1;
      cycles += 3;
      break;

    // INY
    case 0xc8:
      Y += 1;
      sign_zero_flags(Y);
      PC += 1;
      cycles += 2;
      break;

    // INX
    case 0xe8:
      X += 1;
      sign_zero_flags(X);
      PC += 1;
      cycles += 2;
      break;

    // INC zero page
    case 0xe6:
      temp = memory.get_item(arg1) + 1;
      memory.set_item(arg1, temp);
      sign_zero_flags(temp);
      PC += 2;
      cycles += 2;
      break;

    // INC zero page X
    case 0xf6:
      temp_pointer = zero_page_indexed_X(arg1);
      *temp_pointer = *temp_pointer + 1;
      sign_zero_flags(*temp_pointer);
      PC += 2;
      cycles += 2;
      break;

    // INC absolute
    case 0xee:
      temp = memory.get_item(address) + 1;
      memory.set_item(address, temp);
      sign_zero_flags(temp);
      PC += 3;
      cycles += 2;
      break;

    // INC absolute X
    case 0xfe:
      temp_pointer = absolute_indexed_X(address);
      *temp_pointer = *temp_pointer + 1;
      sign_zero_flags(*temp_pointer);
      PC += 3;
      cycles += 2;
      break;

    // EOR immediate
    case 0x49:
      accumulator ^= arg1;
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // EOR zero page
    case 0x45:
      accumulator ^= memory.get_item(arg1);
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // EOR zero page X
    case 0x55:
      accumulator ^= *(zero_page_indexed_X(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // EOR absolute
    case 0x4d:
      accumulator ^= memory.get_item(address);
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // EOR absolute X
    case 0x5d:
      accumulator ^= *(absolute_indexed_X(address));
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // EOR absolute Y
    case 0x59:
      accumulator ^= *(absolute_indexed_Y(address));
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // EOR indirect X
    case 0x41:
      accumulator ^= *(indexed_indirect(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // EOR indirect Y
    case 0x51:
      accumulator ^= *(indirect_indexed(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // DEY
    case 0x88:
      Y = Y - 1;
      sign_zero_flags(Y);
      PC += 1;
      cycles += 2;
      break;

    // DEX
    case 0xca:
      X = X - 1;
      sign_zero_flags(X);
      PC += 1;
      cycles += 2;
      break;

    // DEC zero page
    case 0xc6:
      temp = memory.get_item(arg1) - 1;
      memory.set_item(arg1, temp);
      sign_zero_flags(temp);
      PC += 2;
      cycles += 2;
      break;

    // DEC zero page X
    case 0xd6:
      temp_pointer = zero_page_indexed_X(arg1);
      *temp_pointer = *temp_pointer - 1;
      sign_zero_flags(*temp_pointer);
      PC += 2;
      cycles += 2;
      break;

    // DEC absolute
    case 0xce:
      temp = memory.get_item(address) - 1;
      memory.set_item(address, temp);
      sign_zero_flags(temp);
      PC += 3;
      cycles += 2;
      break;

    // DEC absolute X
    case 0xde:
      temp_pointer = absolute_indexed_X(address);
      *temp_pointer = *temp_pointer - 1;
      sign_zero_flags(*temp_pointer);
      PC += 3;
      cycles += 2;
      break;

    // AND immediate
    case 0x29:
      accumulator &= arg1;
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // AND zero page
    case 0x25:
      accumulator &= memory.get_item(arg1);
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // AND zero page X
    case 0x35:
      accumulator &= *(zero_page_indexed_X(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // AND absolute
    case 0x2d:
      accumulator &= memory.get_item(address);
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // AND absolute X
    case 0x3d:
      accumulator &= *(absolute_indexed_X(address));
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // AND absolute Y
    case 0x39:
      accumulator &= *(absolute_indexed_Y(address));
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // AND indirect X
    case 0x21:
      accumulator &= *(indexed_indirect(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // AND indirect Y
    case 0x31:
      accumulator &= *(indirect_indexed(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // ORA immediate
    case 0x09:
      accumulator |= arg1;
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // ORA zero page
    case 0x05:
      accumulator |= memory.get_item(arg1);
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // ORA zero page X
    case 0x15:
      accumulator |= *(zero_page_indexed_X(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // ORA absolute
    case 0x0d:
      accumulator |= memory.get_item(address);
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // ORA absolute X
    case 0x1d:
      accumulator |= *(absolute_indexed_X(address));
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // ORA absolute Y
    case 0x19:
      accumulator |= *(absolute_indexed_Y(address));
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // ORA indirect X
    case 0x01:
      accumulator |= *(indexed_indirect(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // ORA indirect Y
    case 0x11:
      accumulator |= *(indirect_indexed(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // ADC immediate
    case 0x69:
      add_with_carry(arg1);
      PC += 2;
      break;

    // ADC zero page
    case 0x65:
      add_with_carry(memory.get_item(arg1));
      PC += 2;
      break;

    // ADC zero page X
    case 0x75:
      add_with_carry(*(zero_page_indexed_X(arg1)));
      PC += 2;
      break;

    // ADC absolute
    case 0x6d:
      add_with_carry(memory.get_item(address));
      PC += 3;
      break;

    // ADC absolute X
    case 0x7d:
      add_with_carry(*(absolute_indexed_X(address)));
      PC += 3;
      break;

    // ADC absolute Y
    case 0x79:
      add_with_carry(*(absolute_indexed_Y(address)));
      PC += 3;
      break;

    // ADC indirect X
    case 0x61:
      add_with_carry(*(indexed_indirect(arg1)));
      PC += 2;
      break;

    // ADC indirect Y
    case 0x71:
      add_with_carry(*(indirect_indexed(arg1)));
      PC += 2;
      break;

    // PHP
    case 0x08:
      b_lower = true;
      stack_push(get_flags_as_byte());
      b_lower = false;
      PC += 1;
      cycles += 3;
      break;

    // PLP
    case 0x28:
      set_flags_from_byte(stack_pop());
      PC += 1;
      cycles += 4;
      break;

    // ASL accumulator
    case 0x0a:
      arithmetic_shift_left(&accumulator);
      PC += 1;
      break;

    // ASL zero page
    case 0x06:
      arithmetic_shift_left(memory.get_pointer(arg1));
      PC += 2;
      break;

    // ASL zero page X
    case 0x16:
      arithmetic_shift_left(zero_page_indexed_X(arg1));
      PC += 2;
      break;

    // ASL absolute
    case 0x0e:
      arithmetic_shift_left(memory.get_pointer(address));
      PC += 3;
      break;

    // ASL absolute X
    case 0x1e:
      arithmetic_shift_left(absolute_indexed_X(address));
      PC += 3;
      break;

    // SBC immediate
    case 0xe9:
      add_with_carry(~arg1);
      PC += 2;
      break;

    // SBC zero page
    case 0xe5:
      add_with_carry(~memory.get_item(arg1));
      PC += 2;
      break;

    // SBC zero page X
    case 0xf5:
      add_with_carry(~*(zero_page_indexed_X(arg1)));
      PC += 2;
      break;

    // SBC absolute
    case 0xed:
      add_with_carry(~memory.get_item(address));
      PC += 3;
      break;

    // SBC absolute X
    case 0xfd:
      add_with_carry(~*(absolute_indexed_X(address)));
      PC += 3;
      break;

    // SBC absolute Y
    case 0xf9:
      add_with_carry(~*(absolute_indexed_Y(address)));
      PC += 3;
      break;

    // SBC indirect X
    case 0xe1:
      add_with_carry(~*(indexed_indirect(arg1)));
      PC += 2;
      break;

    // SBC indirect Y
    case 0xf1:
      add_with_carry(~*(indirect_indexed(arg1)));
      PC += 2;
      break;

    // BRK
    case 0x00:
      PC += 2; // something online said this was 2 - includes a padding byte
      break;

    // RTI
    case 0x40:
      set_flags_from_byte(stack_pop());
      PC = address_stack_pop(); // set PC from stack
      break;

    // NOP immediate
    case 0x80:
      PC += 2;
      break;

    // NOP zero page
    case 0x04:
      PC += 2;
      break;

    // NOP zero page
    case 0x44:
      PC += 2;
      break;

    // NOP zero page
    case 0x64:
      PC += 2;
      break;

    // NOP absolute
    case 0x0c:
      PC += 3;
      break;

    // NOP zero page X
    case 0x14:
      PC += 2;
      break;

    // NOP zero page X
    case 0x34:
      PC += 2;
      break;

    // NOP zero page X
    case 0x54:
      PC += 2;
      break;

    // NOP zero page X
    case 0x74:
      PC += 2;
      break;

    // NOP zero page X
    case 0xd4:
      PC += 2;
      break;

    // NOP zero page X
    case 0xf4:
      PC += 2;
      break;

    // NOP absolute indexed X
    case 0x1c:
      PC += 3;
      break;

    // NOP absolute indexed X
    case 0x3c:
      PC += 3;
      break;

    // NOP absolute indexed X
    case 0x5c:
      PC += 3;
      break;

    // NOP absolute indexed X
    case 0x7c:
      PC += 3;
      break;

    // NOP absolute indexed X
    case 0xdc:
      PC += 3;
      break;

    // NOP absolute indexed X
    case 0xfc:
      PC += 3;
      break;

    // NOP immediate
    case 0x89:
      PC += 2;
      break;

    // NOP immediate
    case 0x82:
      PC += 2;
      break;

    // NOP immediate
    case 0xc2:
      PC += 2;
      break;

    // NOP immediate
    case 0xe2:
      PC += 2;
      break;

    // NOP
    case 0x1a:
      PC += 1;
      break;

    // NOP
    case 0x3a:
      PC += 1;
      break;

    // NOP
    case 0x5a:
      PC += 1;
      break;

    // NOP
    case 0x7a:
      PC += 1;
      break;

    // NOP
    case 0xda:
      PC += 1;
      break;

    // NOP
    case 0xfa:
      PC += 1;
      break;

    // LAX zero page
    case 0xa7:
      accumulator = X = *(zero_page(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // LAX indexed indirect
    case 0xa3:
      accumulator = X = *(indexed_indirect(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // LAX immediate
    case 0xab:
      accumulator = X = arg1;
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // LAX absolute
    case 0xaf:
      accumulator = X = *(memory_absolute(address));
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // LAX indirect indexed
    case 0xb3:
      accumulator = X = *(indirect_indexed(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // LAX zero page indexed Y
    case 0xb7:
      accumulator = X = *(zero_page_indexed_Y(arg1));
      sign_zero_flags(accumulator);
      PC += 2;
      break;

    // LAX absolute indexed Y
    case 0xbf:
      accumulator = X = *(absolute_indexed_Y(address));
      sign_zero_flags(accumulator);
      PC += 3;
      break;

    // SAX zero page
    case 0x87:
      *(zero_page(arg1)) = X & accumulator;
      PC += 2;
      break;

    // SAX zero page Y
    case 0x97:
      *(zero_page_indexed_Y(arg1)) = X & accumulator;
      PC += 2;
      break;

    // SAX indirect X
    case 0x83:
      *(indexed_indirect(arg1)) = X & accumulator;
      PC += 2;
      break;

    // SAX absolute
    case 0x8f:
      *(memory_absolute(address)) = X & accumulator;
      PC += 3;
      break;

    // SBC immediate [the undocumented one]
    case 0xeb:
      add_with_carry(~arg1);
      PC += 2;
      break;

    // DCP zero page
    case 0xc7:
      *(zero_page(arg1)) -= 1;
      sign_zero_flags((accumulator - *(zero_page(arg1))) & 0xff);
      PC += 2;
      break;

    // DCP zero page X
    case 0xd7:
      *(zero_page_indexed_X(arg1)) -= 1;
      sign_zero_flags((accumulator - *(zero_page_indexed_X(arg1))) & 0xff);
      PC += 2;
      break;

    // DCP absolute
    case 0xcf:
      *(memory_absolute(address)) -= 1;
      sign_zero_flags((accumulator - *(memory_absolute(address))) & 0xff);
      PC += 3;
      break;

    // DCP absolute X
    case 0xdf:
      *(absolute_indexed_X(address)) -= 1;
      sign_zero_flags((accumulator - *(absolute_indexed_X(address))) & 0xff);
      PC += 3;
      break;

    // DCP absolute Y
    case 0xdb:
      *(absolute_indexed_Y(address)) -= 1;
      sign_zero_flags((accumulator - *(absolute_indexed_Y(address))) & 0xff);
      PC += 3;
      break;

    // DCP indirect X
    case 0xc3:
      *(indexed_indirect(arg1)) -= 1;
      sign_zero_flags((accumulator - *(indexed_indirect(arg1))) & 0xff);
      PC += 2;
      break;

    // DCP indrect Y
    case 0xd3:
      *(indirect_indexed(arg1)) -= 1;
      sign_zero_flags((accumulator - *(indirect_indexed(arg1))) & 0xff);
      PC += 2;
      break;

    // ISB zero page
    case 0xe7:
      increment_subtract(zero_page(arg1));
      PC += 2;
      break;

    // ISB zero page X
    case 0xf7:
      increment_subtract(zero_page_indexed_X(arg1));
      PC += 2;
      break;

    // ISB absolute
    case 0xef:
      increment_subtract(memory_absolute(address));
      PC += 3;
      break;

    // ISB absolte X
    case 0xff:
      increment_subtract(absolute_indexed_X(address));
      PC += 3;
      break;

    // ISB absolute Y
    case 0xfb:
      increment_subtract(absolute_indexed_Y(address));
      PC += 3;
      break;

    // ISB indirect X
    case 0xe3:
      increment_subtract(indexed_indirect(arg1));
      PC += 2;
      break;

    // ISB indirect Y
    case 0xf3:
      increment_subtract(indirect_indexed(arg1));
      PC += 2;
      break;

    // SLO zero page
    case 0x07:
      arithmetic_shift_left_or(zero_page(arg1));
      PC += 2;
      break;

    // SLO zero page X
    case 0x17:
      arithmetic_shift_left_or(zero_page_indexed_X(arg1));
      PC += 2;
      break;

    // SLO absolute
    case 0x0f:
      arithmetic_shift_left_or(memory_absolute(address));
      PC += 3;
      break;

    // SLO absolute X
    case 0x1f:
      arithmetic_shift_left_or(absolute_indexed_X(address));
      PC += 3;
      break;

    // SLO absolute Y
    case 0x1b:
      arithmetic_shift_left_or(absolute_indexed_Y(address));
      PC += 3;
      break;

    // SLO indexed indirect
    case 0x03:
      arithmetic_shift_left_or(indexed_indirect(arg1));
      PC += 2;
      break;

    // SLO indirect indexed
    case 0x13:
      arithmetic_shift_left_or(indirect_indexed(arg1));
      PC += 2;
      break;

    // RLA zero page
    case 0x27:
      rotate_left_and(zero_page(arg1));
      PC += 2;
      break;

    // RLA zero page X
    case 0x37:
      rotate_left_and(zero_page_indexed_X(arg1));
      PC += 2;
      break;

    // RLA absolute
    case 0x2f:
      rotate_left_and(memory_absolute(address));
      PC += 3;
      break;

    // RLA absolute X
    case 0x3f:
      rotate_left_and(absolute_indexed_X(address));
      PC += 3;
      break;

    // RLA absolute Y
    case 0x3b:
      rotate_left_and(absolute_indexed_Y(address));
      PC += 3;
      break;

    // RLA indexed indirect
    case 0x23:
      rotate_left_and(indexed_indirect(arg1));
      PC += 2;
      break;

    // RLA indirect indexed
    case 0x33:
      rotate_left_and(indirect_indexed(arg1));
      PC += 2;
      break;

    // SRE zero page
    case 0x47:
      shift_right_xor(zero_page(arg1));
      PC += 2;
      break;

    // SRE zero paeg X
    case 0x57:
      shift_right_xor(zero_page_indexed_X(arg1));
      PC += 2;
      break;

    // SRE absolute
    case 0x4f:
      shift_right_xor(memory_absolute(address));
      PC += 3;
      break;

    // SRE absolute X
    case 0x5f:
      shift_right_xor(absolute_indexed_X(address));
      PC += 3;
      break;

    // SRE absolute Y
    case 0x5b:
      shift_right_xor(absolute_indexed_Y(address));
      PC += 3;
      break;

    // SRE indexed indirect
    case 0x43:
      shift_right_xor(indexed_indirect(arg1));
      PC += 2;
      break;

    // SRE indirect indexed
    case 0x53:
      shift_right_xor(indirect_indexed(arg1));
      PC += 2;
      break;

    // RRA zero page
    case 0x67:
      rotate_right_add(zero_page(arg1));
      PC += 2;
      break;

    // RRA zero page X
    case 0x77:
      rotate_right_add(zero_page_indexed_X(arg1));
      PC += 2;
      break;

    // RRA absolute
    case 0x6f:
      rotate_right_add(memory_absolute(address));
      PC += 3;
      break;

    // RRA absolute X
    case 0x7f:
      rotate_right_add(absolute_indexed_X(address));
      PC += 3;
      break;

    // RRA absolute Y
    case 0x7b:
      rotate_right_add(absolute_indexed_Y(address));
      PC += 3;
      break;

    // RRA indexed indirect
    case 0x63:
      rotate_right_add(indexed_indirect(arg1));
      PC += 2;
      break;

    // RRA indirect indexed
    case 0x73:
      rotate_right_add(indirect_indexed(arg1));
      PC += 2;
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

void CPU::rotate_right_add(uint8_t* operand) {
  rotate_right(operand);
  add_with_carry(*operand);
}

void CPU::shift_right_xor(uint8_t* operand) {
  shift_right(operand);
  accumulator ^= *operand;
  sign_zero_flags(accumulator);
}

void CPU::rotate_left_and(uint8_t* operand) {
  rotate_left(operand);
  accumulator &= *operand;
  sign_zero_flags(accumulator);
}

void CPU::arithmetic_shift_left_or(uint8_t* operand) {
  arithmetic_shift_left(operand);
  accumulator |= *operand;
  sign_zero_flags(accumulator);
}

void CPU::increment_subtract(uint8_t* operand) {
  *operand += 1;
  add_with_carry(~(*operand));
}

void CPU::add_with_carry(uint8_t operand) {
  uint8_t before = accumulator;
  uint16_t sum = accumulator + operand + carry;
  accumulator += operand + carry;
  zero = (accumulator == 0);
  sign = (accumulator >= 0x80);
  // signed overflow
  overflow = ~(operand ^ before) & (before ^ sum) & 0x80;
  // unsigned overflow
  carry = sum > 0xff;
}

uint8_t CPU::get_flags_as_byte() {
  uint8_t flags = 0;
  flags |= (sign << 7);
  flags |= (overflow << 6);
  flags |= (b_upper << 5);
  flags |= (b_lower << 4);
  flags |= (decimal << 3);
  flags |= (interrupt_disable << 2);
  flags |= (zero << 1);
  flags |= carry;
  return flags;
}

void CPU::set_flags_from_byte(uint8_t flags) {
  carry = flags & 0x1;
  zero = (flags >> 1) & 0x1;
  interrupt_disable = (flags >> 2) & 0x1;
  decimal = (flags >> 3) & 0x1;
  overflow = (flags >> 6) & 0x1;
  sign = (flags >> 7) & 0x1;
}

// POTENTIAL BUG -> zero flag set when A == 0 or new_val == 0 (??)
// going with the second, correct if wrong
void CPU::rotate_right(uint8_t* operand) {
  uint8_t new_value = (carry << 7) | (*operand >> 1);
  carry = *operand & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80);
  *operand = new_value;
  cycles += 2; // ROR has extra 2 cycles
}

void CPU::rotate_left(uint8_t* operand) {
  uint8_t new_value = (*operand << 1) | carry;
  carry = (*operand >> 7) & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80);
  *operand = new_value;
  cycles += 2;
}

void CPU::shift_right(uint8_t* operand) {
  uint8_t new_value = *operand >> 1;
  carry = *operand & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80); // this is basically impossible i assume
  *operand = new_value;
  cycles += 2;
}

void CPU::arithmetic_shift_left(uint8_t* operand) {
  uint8_t new_value = *operand << 1;
  carry = (*operand >> 7) & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80);
  *operand = new_value;
  cycles += 2;
}

void CPU::compare(uint8_t operand, uint8_t argument) {
  carry = (operand >= argument);
  zero = (operand == argument);
  sign = ((uint8_t) (operand - argument)) >= 0x80; // two's cmplt cmp
}

void CPU::sign_zero_flags(uint8_t val) {
  sign = (val >= 0x80);
  zero = (val == 0);
}

void CPU::branch_on_bool(bool arg, int8_t offset) {
  if (arg) {
    uint16_t original_pc_upper = (PC >> 4) & 0xf; // upper nibble
    PC += ((int8_t) offset) + 2;
    uint16_t new_pc_upper = (PC >> 4) & 0xf;
    // extra cycle if crossed page boundary
    if (original_pc_upper != new_pc_upper) {
      cycles += 1;
    }
    cycles += 1;
  } else {
    PC += 2;
  }
  cycles += 2;
}

// ADDRESSING MODES POINTER

uint8_t* CPU::zero_page(uint8_t arg) {
  cycles += 3;
  return memory.get_pointer(arg);
}

uint8_t* CPU::memory_absolute(uint16_t address) {
  cycles += 4;
  return memory.get_pointer(address);
}

uint8_t* CPU::zero_page_indexed_X(uint8_t arg) {
  cycles += 4;
  return memory.get_pointer((arg + X) & 0xff);
}

uint8_t* CPU::zero_page_indexed_Y(uint8_t arg) {
  cycles += 4;
  return memory.get_pointer((arg + Y) & 0xff);
}

uint8_t* CPU::absolute_indexed_X(uint16_t arg) {
  cycles += 4;
  uint8_t summed_low_byte = arg + Y;
  if (summed_low_byte < Y) {
    cycles += 1; // page overflow CPU cycle
  }
  return memory.get_pointer(arg + X);
}

uint8_t* CPU::absolute_indexed_Y(uint16_t arg) {
  cycles += 4;
  uint8_t summed_low_byte = arg + Y;
  if (summed_low_byte < Y) {
    cycles += 1; // page overflow CPU cycle
  }
  return memory.get_pointer(arg + Y);
}

uint8_t* CPU::indexed_indirect(uint8_t arg) {
  cycles += 6;
  uint16_t low_byte = (uint16_t) memory.get_item((arg + X) & 0xff);
  uint16_t high_byte = (uint16_t) memory.get_item((arg + X + 1) & 0xff) << 8;
  return memory.get_pointer(high_byte | low_byte);
}

uint8_t* CPU::indirect_indexed(uint8_t arg) {
  cycles += 5;
  uint16_t low_byte = memory.get_item(arg);
  uint16_t high_byte = (uint16_t) memory.get_item((arg + 1) & 0xff) << 8;
  if (low_byte + Y < Y) {
    cycles += 1; // page overflow CPU cycle
  }
  return memory.get_pointer((high_byte | low_byte) + Y);
}

uint16_t CPU::indirect(uint8_t arg1, uint8_t arg2) {
  uint16_t lower_byte_address = arg2 << 8 | arg1;
  uint16_t upper_byte_address = arg2 << 8 | ((arg1 + 1) & 0xff); // because last byte overflow won't carry over
  uint8_t lower_byte = memory.get_item(lower_byte_address);
  uint8_t upper_byte = memory.get_item(upper_byte_address);
  uint16_t new_pc_address = (upper_byte << 8) | lower_byte;
  return new_pc_address;
}


// ideally loading would mean a pointer but I'll keep this for now, sigh
// not correct b/c bank switching means needless IOs instead of a simple pointer swap LMAOO
void CPU::load_program(char* program, int size) {
  bool mirror = false;
  if (size == 0x4000) {
    mirror = true;
  }
  int program_offset = 0x8000;
  for (int i = 0; i < size; i++) {
    memory.set_item(program_offset + i, *(program + i));
    if (mirror) {
      memory.set_item(program_offset + 0x4000 + i, *(program + i));
    }
  }
  // need to cast to (uint16_t) ?
  PC = (memory.get_item(0xfffd) << 8) | memory.get_item(0xfffc);
  /* NESTEST OVERLOADED PC */

  PC = 0xc000;

  SP = 0xfd;
  valid = true;
  interrupt_disable = true;
  b_upper = true;
}

int main() {
  CPU nes;
  ifstream rom("nestest.nes", ios::binary | ios::ate);
  streamsize size = rom.tellg();
  rom.seekg(0, ios::beg);

  char buffer[size];
  if (rom.read(buffer, size)) {
    int prg_size = buffer[4] * 0x4000; // multiplier
    // techncially doesn't count for possibility of trainer but that's ok bc those are rare at the moment
    char* prg_data = buffer + 16;
    nes.load_program(prg_data, prg_size);
    nes.run();
  }


}
