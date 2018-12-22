class CPU {
  public:
    // CPU registers; reset should initialize to zero unless set
    uint8_t accumulator;
    uint8_t X;
    uint8_t Y;
    uint16_t PC;
    // only holds lower byte of the real SP
    uint8_t SP;

    // cycle count
    uint64_t local_clock;

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
    Memory* memory;

    // running stuff
    void reset_game();
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
    void compare(uint8_t, uint8_t*);
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
    void decrement_compare(uint8_t*);
    void load_accumulator_x(uint8_t*);
    void store_register(uint8_t*, uint8_t*);
    void load_register(uint8_t*, uint8_t*);
    void set_flag(bool*, bool);
    void bit(uint8_t*);
    void transfer_register(uint8_t*, uint8_t, bool);
    void decrement(uint8_t*);
    void increment(uint8_t*);
    void boolean_or(uint8_t*);
    void boolean_and(uint8_t*);
    void boolean_xor(uint8_t*);

    // addressing modes
    uint8_t* immediate(uint8_t*);
    uint8_t* zero_page(uint8_t);
    uint8_t* memory_absolute(uint16_t);
    uint8_t* zero_page_indexed_X(uint8_t);
    uint8_t* zero_page_indexed_Y(uint8_t);
    uint8_t* absolute_indexed_X(uint16_t, bool);
    uint8_t* absolute_indexed_Y(uint16_t, bool);
    uint8_t* indexed_indirect(uint8_t);
    uint8_t* indirect_indexed(uint8_t, bool);
    uint16_t indirect(uint8_t, uint8_t);

    // addressing mode enum
    void set_memory(Memory*);
};

void CPU::set_memory(Memory* mem_pointer) {
  memory = mem_pointer;
}

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
  memory->set_item(stack_offset + SP, value);
  // simulate underflow
  SP -= 1;
}

uint8_t CPU::stack_pop() {
  uint16_t stack_offset = 0x100;
  uint16_t stack_address = stack_offset + SP + 1;
  uint8_t value = memory->get_item(stack_address);
  SP += 1;
  return value;
}

void CPU::print_register_values() {
  printf("PC:%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%d\n",
          PC,
          accumulator,
          X,
          Y,
          get_flags_as_byte(),
          SP,
          (local_clock * 3) % 341);
}

// basically one giant switch case
// how can i make this better?
void CPU::execute_cycle() {
  uint8_t opcode = memory->get_item(PC);
   // these may lead to errors if we're at the end of PRG
  uint8_t arg1 = memory->get_item(PC + 1);
  uint8_t arg2 = memory->get_item(PC + 2);
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
      local_clock += 6;
      break;

    // RTS
    case 0x60:
      PC = address_stack_pop() + 1; // add one to stored address
      local_clock += 6;
      break;

    // NOP
    case 0xea:
      local_clock += 2;
      PC += 1;
      break;

    // CMP: i
    case 0xc9:
      compare(accumulator, immediate(&arg1));
      break;

    // CMP: zero page
    case 0xc5:
      compare(accumulator, zero_page(arg1));
      break;

    // CMP: zero page, X
    case 0xd5:
      compare(accumulator, zero_page_indexed_X(arg1));
      break;

    // CMP: absolute
    case 0xcd:
      compare(accumulator, memory_absolute(address));
      break;

    // CMP: absolute, X
    case 0xdd:
      compare(accumulator, absolute_indexed_X(address, false));
      break;

    // CMP: absolute, Y
    case 0xd9:
      compare(accumulator, absolute_indexed_Y(address, false));
      break;

    // CMP: indirect, X
    case 0xc1:
      compare(accumulator, indexed_indirect(arg1));
      break;

    // CMP: indirect, Y
    case 0xd1:
      compare(accumulator, indirect_indexed(arg1, false));
      break;

    // CPX: immediate
    case 0xe0:
      compare(X, immediate(&arg1));
      break;

    // CPX: zero page
    case 0xe4:
      compare(X, zero_page(arg1));
      break;

    // CPX: absolute
    case 0xec:
      compare(X, memory_absolute(address));
      break;

    // CPY: immediate
    case 0xc0:
      compare(Y, immediate(&arg1));
      break;

    // CPY: zero page
    case 0xc4:
      compare(Y, zero_page(arg1));
      break;

    // CPY: absolute
    case 0xcc:
      compare(Y, memory_absolute(address));
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
      load_register(&accumulator, immediate(&arg1));
      break;

    // LDA zero page
    case 0xa5:
      load_register(&accumulator, zero_page(arg1));
      break;

    // LDA zero page X
    case 0xb5:
      load_register(&accumulator, zero_page_indexed_X(arg1));
      break;

    // LDA absolute
    case 0xad:
      load_register(&accumulator, memory_absolute(address));
      break;

    // LDA Absolute X
    case 0xbd:
      load_register(&accumulator, absolute_indexed_X(address, false));
      break;

    // LDA absolute Y
    case 0xb9:
      load_register(&accumulator, absolute_indexed_Y(address, false));
      break;

    // LDA indirect X
    case 0xa1:
      load_register(&accumulator, indexed_indirect(arg1));
      break;

    // LDA indirect Y
    case 0xb1:
      load_register(&accumulator, indirect_indexed(arg1, false));
      break;

    // LDX immediate
    case 0xa2:
      load_register(&X, immediate(&arg1));
      break;

    // LDX zero page
    case 0xa6:
      load_register(&X, zero_page(arg1));
      break;

    // LDX zero page Y
    case 0xb6:
      load_register(&X, zero_page_indexed_Y(arg1));
      break;

    // LDX absolute
    case 0xae:
      load_register(&X, memory_absolute(address));
      break;

    // LDX absolute Y
    case 0xbe:
      load_register(&X, absolute_indexed_Y(address, false));
      break;

    // LDY immediate
    case 0xa0:
      load_register(&Y, immediate(&arg1));
      break;

    // LDY zero page
    case 0xa4:
      load_register(&Y, zero_page(arg1));
      break;

    // LDY zero page X
    case 0xb4:
      load_register(&Y, zero_page_indexed_X(arg1));
      break;

    // LDY absolute
    case 0xac:
      load_register(&Y, memory_absolute(address));
      break;

    // LDY absolute X
    case 0xbc:
      load_register(&Y, absolute_indexed_X(address, false));
      break;

    // STA zero page
    case 0x85:
      store_register(&accumulator, zero_page(arg1));
      break;

    // STA zero page X
    case 0x95:
      store_register(&accumulator, zero_page_indexed_X(arg1));
      break;

    // STA absolute
    case 0x8d:
      store_register(&accumulator, memory_absolute(address));
      break;

    // STA absolute X
    case 0x9d:
      store_register(&accumulator, absolute_indexed_X(address, true));
      break;

    // STA absolute Y
    case 0x99:
      store_register(&accumulator, absolute_indexed_Y(address, true));
      break;

    // STA indirect X
    case 0x81:
      store_register(&accumulator, indexed_indirect(arg1));
      break;

    // STA indirect Y
    case 0x91:
      store_register(&accumulator, indirect_indexed(arg1, true));
      break;

    // STX zero page
    case 0x86:
      store_register(&X, zero_page(arg1));
      break;

    // STX zero page Y
    case 0x96:
      store_register(&X, zero_page_indexed_Y(arg1));
      break;

    // STX absolute
    case 0x8E:
      store_register(&X, memory_absolute(address));
      break;

    // STY zero page
    case 0x84:
      store_register(&Y, zero_page(arg1));
      break;

    // STY zero page X
    case 0x94:
      store_register(&Y, zero_page_indexed_X(arg1));
      break;

    // STY absolute
    case 0x8c:
      store_register(&Y, memory_absolute(address));
      break;

    // JMP absolute
    case 0x4c:
      PC = address;
      local_clock += 3;
      break;

    // JMP indirect
    case 0x6c:
      PC = indirect(arg1, arg2);
      break;

    // CLC (clear carry)
    case 0x18:
      set_flag(&carry, false);
      break;

    // SEC (set carry)
    case 0x38:
      set_flag(&carry, true);
      break;

    // CLI (clear interrupt)
    case 0x58:
      set_flag(&interrupt_disable, false);
      break;

    // SEI (set interrupt)
    case 0x78:
      set_flag(&interrupt_disable, true);
      break;

    // CLV (clear overflow)
    case 0xb8:
      set_flag(&overflow, false);
      break;

    // CLD (clear decimal)
    case 0xd8:
      set_flag(&decimal, false);
      break;

    // SED (set decimal)
    case 0xf8:
      set_flag(&decimal, true);
      break;

    // BIT zero page
    case 0x24:
      bit(zero_page(arg1));
      break;

    // BIT absolute
    case 0x2c:
      bit(memory_absolute(address));
      break;

    // TYA
    case 0x98:
      transfer_register(&accumulator, Y, true);
      break;

    // TXS
    case 0x9a:
      transfer_register(&SP, X, false);
      break;

    // TXA
    case 0x8a:
      transfer_register(&accumulator, X, true);
      break;

    // TSX
    case 0xba:
      transfer_register(&X, SP, true);
      break;

    // TAY
    case 0xa8:
      transfer_register(&Y, accumulator, true);
      break;

    // TAX
    case 0xaa:
      transfer_register(&X, accumulator, true);
      break;

    // ROR accumulator
    case 0x6a:
      rotate_right(&accumulator);
      PC += 1;
      break;

    // ROR zero page
    case 0x66:
      rotate_right(zero_page(arg1));
      break;

    // ROR zero page X
    case 0x76:
      rotate_right(zero_page_indexed_X(arg1));
      break;

    // ROR absolute
    case 0x6e:
      rotate_right(memory_absolute(address));
      break;

    // ROR absolute X
    case 0x7e:
      rotate_right(absolute_indexed_X(address, true));
      break;

    // ROL accumulator
    case 0x2a:
      rotate_left(&accumulator);
      PC += 1;
      break;

    // ROL zero page
    case 0x26:
      rotate_left(zero_page(arg1));
      break;

    // ROL zero page X
    case 0x36:
      rotate_left(zero_page_indexed_X(arg1));
      break;

    // ROL absolute
    case 0x2e:
      rotate_left(memory_absolute(address));
      break;

    // ROL absolute X
    case 0x3e:
      rotate_left(absolute_indexed_X(address, true));
      break;

    // LSR accumulator
    case 0x4a:
      shift_right(&accumulator);
      PC += 1;
      break;

    // LSR zero page
    case 0x46:
      shift_right(zero_page(arg1));
      break;

    // LSR zero page X
    case 0x56:
      shift_right(zero_page_indexed_X(arg1));
      break;

    // LSR absolute
    case 0x4e:
      shift_right(memory_absolute(address));
      break;

    // LSR absolute X
    case 0x5e:
      shift_right(absolute_indexed_X(address, true));
      break;

    // PLA
    case 0x68:
      accumulator = stack_pop();
      sign_zero_flags(accumulator);
      local_clock += 4;
      PC += 1;
      break;

    // PHA
    case 0x48:
      stack_push(accumulator);
      local_clock += 3;
      PC += 1;
      break;

    // INY
    case 0xc8:
      increment(&Y);
      PC += 1;
      break;

    // INX
    case 0xe8:
      increment(&X);
      PC += 1;
      break;

    // INC zero page
    case 0xe6:
      increment(zero_page(arg1));
      break;

    // INC zero page X
    case 0xf6:
      increment(zero_page_indexed_X(arg1));
      break;

    // INC absolute
    case 0xee:
      increment(memory_absolute(address));
      break;

    // INC absolute X
    case 0xfe:
      increment(absolute_indexed_X(address, true));
      break;

    // EOR immediate
    case 0x49:
      boolean_xor(immediate(&arg1));
      break;

    // EOR zero page
    case 0x45:
      boolean_xor(zero_page(arg1));
      break;

    // EOR zero page X
    case 0x55:
      boolean_xor(zero_page_indexed_X(arg1));
      break;

    // EOR absolute
    case 0x4d:
      boolean_xor(memory_absolute(address));
      break;

    // EOR absolute X
    case 0x5d:
      boolean_xor(absolute_indexed_X(address, false));
      break;

    // EOR absolute Y
    case 0x59:
      boolean_xor(absolute_indexed_Y(address, false));
      break;

    // EOR indirect X
    case 0x41:
      boolean_xor(indexed_indirect(arg1));
      break;

    // EOR indirect Y
    case 0x51:
      boolean_xor(indirect_indexed(arg1, false));
      break;

    // DEY
    case 0x88:
      decrement(&Y);
      PC += 1;
      break;

    // DEX
    case 0xca:
      decrement(&X);
      PC += 1;
      break;

    // DEC zero page
    case 0xc6:
      decrement(zero_page(arg1));
      break;

    // DEC zero page X
    case 0xd6:
      decrement(zero_page_indexed_X(arg1));
      break;

    // DEC absolute
    case 0xce:
      decrement(memory_absolute(address));
      break;

    // DEC absolute X
    case 0xde:
      decrement(absolute_indexed_X(address, true));
      break;

    // AND immediate
    case 0x29:
      boolean_and(immediate(&arg1));
      break;

    // AND zero page
    case 0x25:
      boolean_and(zero_page(arg1));
      break;

    // AND zero page X
    case 0x35:
      boolean_and(zero_page_indexed_X(arg1));
      break;

    // AND absolute
    case 0x2d:
      boolean_and(memory_absolute(address));
      break;

    // AND absolute X
    case 0x3d:
      boolean_and(absolute_indexed_X(address, false));
      break;

    // AND absolute Y
    case 0x39:
      boolean_and(absolute_indexed_Y(address, false));
      break;

    // AND indirect X
    case 0x21:
      boolean_and(indexed_indirect(arg1));
      break;

    // AND indirect Y
    case 0x31:
      boolean_and(indirect_indexed(arg1, false));
      break;

    // ORA immediate
    case 0x09:
      boolean_or(immediate(&arg1));
      break;

    // ORA zero page
    case 0x05:
      boolean_or(zero_page(arg1));
      break;

    // ORA zero page X
    case 0x15:
      boolean_or(zero_page_indexed_X(arg1));
      break;

    // ORA absolute
    case 0x0d:
      boolean_or(memory_absolute(address));
      break;

    // ORA absolute X
    case 0x1d:
      boolean_or(absolute_indexed_X(address, false));
      break;

    // ORA absolute Y
    case 0x19:
      boolean_or(absolute_indexed_Y(address, false));
      break;

    // ORA indirect X
    case 0x01:
      boolean_or(indexed_indirect(arg1));
      break;

    // ORA indirect Y
    case 0x11:
      boolean_or(indirect_indexed(arg1, false));
      break;

    // ADC immediate
    case 0x69:
      add_with_carry(*immediate(&arg1));
      break;

    // ADC zero page
    case 0x65:
      add_with_carry(*zero_page(arg1));
      break;

    // ADC zero page X
    case 0x75:
      add_with_carry(*zero_page_indexed_X(arg1));
      break;

    // ADC absolute
    case 0x6d:
      add_with_carry(*memory_absolute(address));
      break;

    // ADC absolute X
    case 0x7d:
      add_with_carry(*absolute_indexed_X(address, false));
      break;

    // ADC absolute Y
    case 0x79:
      add_with_carry(*absolute_indexed_Y(address, false));
      break;

    // ADC indirect X
    case 0x61:
      add_with_carry(*indexed_indirect(arg1));
      break;

    // ADC indirect Y
    case 0x71:
      add_with_carry(*indirect_indexed(arg1, false));
      break;

    // PHP
    case 0x08:
      b_lower = true;
      stack_push(get_flags_as_byte());
      b_lower = false;
      local_clock += 3;
      PC += 1;
      break;

    // PLP
    case 0x28:
      set_flags_from_byte(stack_pop());
      local_clock += 4;
      PC += 1;
      break;

    // ASL accumulator
    case 0x0a:
      arithmetic_shift_left(&accumulator);
      PC += 1;
      break;

    // ASL zero page
    case 0x06:
      arithmetic_shift_left(zero_page(arg1));
      break;

    // ASL zero page X
    case 0x16:
      arithmetic_shift_left(zero_page_indexed_X(arg1));
      break;

    // ASL absolute
    case 0x0e:
      arithmetic_shift_left(memory_absolute(address));
      break;

    // ASL absolute X
    case 0x1e:
      arithmetic_shift_left(absolute_indexed_X(address, true));
      break;

    // SBC immediate
    case 0xe9:
      add_with_carry(~*immediate(&arg1));
      break;

    // SBC zero page
    case 0xe5:
      add_with_carry(~*zero_page(arg1));
      break;

    // SBC zero page X
    case 0xf5:
      add_with_carry(~*zero_page_indexed_X(arg1));
      break;

    // SBC absolute
    case 0xed:
      add_with_carry(~*memory_absolute(address));
      break;

    // SBC absolute X
    case 0xfd:
      add_with_carry(~*absolute_indexed_X(address, false));
      break;

    // SBC absolute Y
    case 0xf9:
      add_with_carry(~*absolute_indexed_Y(address, false));
      break;

    // SBC indirect X
    case 0xe1:
      add_with_carry(~*indexed_indirect(arg1));
      break;

    // SBC indirect Y
    case 0xf1:
      add_with_carry(~*indirect_indexed(arg1, false));
      break;

    // BRK
    case 0x00:
      address_stack_push(PC + 2);
      b_lower = true;
      stack_push(get_flags_as_byte());
      b_lower = false;
      interrupt_disable = true;
      PC = memory->brk_vector();
      local_clock += 7;
      break;

    // RTI
    case 0x40:
      set_flags_from_byte(stack_pop());
      PC = address_stack_pop(); // set PC from stack
      local_clock += 6;
      break;

    // NOP immediate
    case 0x80:
      immediate(&arg1);
      break;

    // NOP zero page
    case 0x04:
      zero_page(arg1);
      break;

    // NOP zero page
    case 0x44:
      zero_page(arg1);
      break;

    // NOP zero page
    case 0x64:
      zero_page(arg1);
      break;

    // NOP absolute
    case 0x0c:
      memory_absolute(address);
      break;

    // NOP zero page X
    case 0x14:
      zero_page_indexed_X(arg1);
      break;

    // NOP zero page X
    case 0x34:
      zero_page_indexed_X(arg1);
      break;

    // NOP zero page X
    case 0x54:
      zero_page_indexed_X(arg1);
      break;

    // NOP zero page X
    case 0x74:
      zero_page_indexed_X(arg1);
      break;

    // NOP zero page X
    case 0xd4:
      zero_page_indexed_X(arg1);
      break;

    // NOP zero page X
    case 0xf4:
      zero_page_indexed_X(arg1);
      break;

    // NOP absolute indexed X
    case 0x1c:
      absolute_indexed_X(address, false);
      break;

    // NOP absolute indexed X
    case 0x3c:
      absolute_indexed_X(address, false);
      break;

    // NOP absolute indexed X
    case 0x5c:
      absolute_indexed_X(address, false);
      break;

    // NOP absolute indexed X
    case 0x7c:
      absolute_indexed_X(address, false);
      break;

    // NOP absolute indexed X
    case 0xdc:
      absolute_indexed_X(address, false);
      break;

    // NOP absolute indexed X
    case 0xfc:
      absolute_indexed_X(address, false);
      break;

    // NOP immediate
    case 0x89:
      immediate(&arg1);
      break;

    // NOP immediate
    case 0x82:
      immediate(&arg1);
      break;

    // NOP immediate
    case 0xc2:
      immediate(&arg1);
      break;

    // NOP immediate
    case 0xe2:
      immediate(&arg1);
      break;

    // NOP
    case 0x1a:
      PC += 1;
      local_clock += 2;
      break;

    // NOP
    case 0x3a:
      PC += 1;
      local_clock += 2;
      break;

    // NOP
    case 0x5a:
      PC += 1;
      local_clock += 2;
      break;

    // NOP
    case 0x7a:
      PC += 1;
      local_clock += 2;
      break;

    // NOP
    case 0xda:
      PC += 1;
      local_clock += 2;
      break;

    // NOP
    case 0xfa:
      PC += 1;
      local_clock += 2;
      break;

    // LAX zero page
    case 0xa7:
      load_accumulator_x(zero_page(arg1));
      break;

    // LAX indexed indirect
    case 0xa3:
      load_accumulator_x(indexed_indirect(arg1));
      break;

    // LAX immediate
    case 0xab:
      load_accumulator_x(immediate(&arg1));
      break;

    // LAX absolute
    case 0xaf:
      load_accumulator_x(memory_absolute(address));
      break;

    // LAX indirect indexed
    case 0xb3:
      load_accumulator_x(indirect_indexed(arg1, false));
      break;

    // LAX zero page indexed Y
    case 0xb7:
      load_accumulator_x(zero_page_indexed_Y(arg1));
      break;

    // LAX absolute indexed Y
    case 0xbf:
      load_accumulator_x(absolute_indexed_Y(address, false));
      break;

    // SAX zero page
    case 0x87:
      *(zero_page(arg1)) = X & accumulator;
      break;

    // SAX zero page Y
    case 0x97:
      *(zero_page_indexed_Y(arg1)) = X & accumulator;
      break;

    // SAX indirect X
    case 0x83:
      *(indexed_indirect(arg1)) = X & accumulator;
      break;

    // SAX absolute
    case 0x8f:
      *(memory_absolute(address)) = X & accumulator;
      break;

    // SBC immediate [the undocumented one]
    case 0xeb:
      add_with_carry(~*immediate(&arg1));
      break;

    // DCP zero page
    case 0xc7:
      decrement_compare(zero_page(arg1));
      break;

    // DCP zero page X
    case 0xd7:
      decrement_compare(zero_page_indexed_X(arg1));
      break;

    // DCP absolute
    case 0xcf:
      decrement_compare(memory_absolute(address));
      break;

    // DCP absolute X
    case 0xdf:
      decrement_compare(absolute_indexed_X(address, true));
      break;

    // DCP absolute Y
    case 0xdb:
      decrement_compare(absolute_indexed_Y(address, true));
      break;

    // DCP indirect X
    case 0xc3:
      decrement_compare(indexed_indirect(arg1));
      break;

    // DCP indrect Y
    case 0xd3:
      decrement_compare(indirect_indexed(arg1, true));
      break;

    // ISB zero page
    case 0xe7:
      increment_subtract(zero_page(arg1));
      break;

    // ISB zero page X
    case 0xf7:
      increment_subtract(zero_page_indexed_X(arg1));
      break;

    // ISB absolute
    case 0xef:
      increment_subtract(memory_absolute(address));
      break;

    // ISB absolute X
    case 0xff:
      increment_subtract(absolute_indexed_X(address, true));
      break;

    // ISB absolute Y
    case 0xfb:
      increment_subtract(absolute_indexed_Y(address, true));
      break;

    // ISB indirect X
    case 0xe3:
      increment_subtract(indexed_indirect(arg1));
      break;

    // ISB indirect Y
    case 0xf3:
      increment_subtract(indirect_indexed(arg1, true));
      break;

    // SLO zero page
    case 0x07:
      arithmetic_shift_left_or(zero_page(arg1));
      break;

    // SLO zero page X
    case 0x17:
      arithmetic_shift_left_or(zero_page_indexed_X(arg1));
      break;

    // SLO absolute
    case 0x0f:
      arithmetic_shift_left_or(memory_absolute(address));
      break;

    // SLO absolute X
    case 0x1f:
      arithmetic_shift_left_or(absolute_indexed_X(address, false));
      break;

    // SLO absolute Y
    case 0x1b:
      arithmetic_shift_left_or(absolute_indexed_Y(address, false));
      break;

    // SLO indexed indirect
    case 0x03:
      arithmetic_shift_left_or(indexed_indirect(arg1));
      break;

    // SLO indirect indexed
    case 0x13:
      arithmetic_shift_left_or(indirect_indexed(arg1, false));
      break;

    // RLA zero page
    case 0x27:
      rotate_left_and(zero_page(arg1));
      break;

    // RLA zero page X
    case 0x37:
      rotate_left_and(zero_page_indexed_X(arg1));
      break;

    // RLA absolute
    case 0x2f:
      rotate_left_and(memory_absolute(address));
      break;

    // RLA absolute X
    case 0x3f:
      rotate_left_and(absolute_indexed_X(address, false));
      break;

    // RLA absolute Y
    case 0x3b:
      rotate_left_and(absolute_indexed_Y(address, false));
      break;

    // RLA indexed indirect
    case 0x23:
      rotate_left_and(indexed_indirect(arg1));
      break;

    // RLA indirect indexed
    case 0x33:
      rotate_left_and(indirect_indexed(arg1, false));
      break;

    // SRE zero page
    case 0x47:
      shift_right_xor(zero_page(arg1));
      break;

    // SRE zero paeg X
    case 0x57:
      shift_right_xor(zero_page_indexed_X(arg1));
      break;

    // SRE absolute
    case 0x4f:
      shift_right_xor(memory_absolute(address));
      break;

    // SRE absolute X
    case 0x5f:
      shift_right_xor(absolute_indexed_X(address, false));
      break;

    // SRE absolute Y
    case 0x5b:
      shift_right_xor(absolute_indexed_Y(address, false));
      break;

    // SRE indexed indirect
    case 0x43:
      shift_right_xor(indexed_indirect(arg1));
      break;

    // SRE indirect indexed
    case 0x53:
      shift_right_xor(indirect_indexed(arg1, false));
      break;

    // RRA zero page
    case 0x67:
      rotate_right_add(zero_page(arg1));
      break;

    // RRA zero page X
    case 0x77:
      rotate_right_add(zero_page_indexed_X(arg1));
      break;

    // RRA absolute
    case 0x6f:
      rotate_right_add(memory_absolute(address));
      break;

    // RRA absolute X
    case 0x7f:
      rotate_right_add(absolute_indexed_X(address, false));
      break;

    // RRA absolute Y
    case 0x7b:
      rotate_right_add(absolute_indexed_Y(address, false));
      break;

    // RRA indexed indirect
    case 0x63:
      rotate_right_add(indexed_indirect(arg1));
      break;

    // RRA indirect indexed
    case 0x73:
      rotate_right_add(indirect_indexed(arg1, false));
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
    if (PC < 0x10) {
      return;
    }
  }
}

void CPU::boolean_xor(uint8_t* operand) {
  accumulator ^= *operand;
  sign_zero_flags(accumulator);
}

void CPU::boolean_and(uint8_t* operand) {
  accumulator &= *operand;
  sign_zero_flags(accumulator);
}

void CPU::boolean_or(uint8_t* operand) {
  accumulator |= *operand;
  sign_zero_flags(accumulator);
}

void CPU::increment(uint8_t* operand) {
  *operand += 1;
  sign_zero_flags(*operand);
  local_clock += 2;
}

void CPU::decrement(uint8_t* operand) {
  *operand -= 1;
  sign_zero_flags(*operand);
  local_clock += 2;
}

void CPU::transfer_register(uint8_t* to, uint8_t from, bool flags) {
  *to = from;
  if (flags) {
    sign_zero_flags(from);
  }
  local_clock += 2;
  PC += 1;
}

void CPU::bit(uint8_t* operand) {
  uint8_t value = *operand;
  zero = ((value & accumulator) == 0);
  overflow = (value >> 6) & 1;
  sign = value >= 0x80;
}

void CPU::set_flag(bool* flag, bool value) {
  *flag = value;
  local_clock += 2;
  PC += 1;
}

void CPU::load_register(uint8_t* reg, uint8_t* operand) {
  *reg = *operand;
  sign_zero_flags(*reg);
}

void CPU::store_register(uint8_t* reg, uint8_t* operand) {
  *operand = *reg;
}

void CPU::load_accumulator_x(uint8_t* operand) {
  accumulator = X = *operand;
  sign_zero_flags(accumulator);
}

void CPU::decrement_compare(uint8_t* operand) {
  *operand -= 1;
  sign_zero_flags((accumulator - *operand) & 0xff);
  local_clock += 2;
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
  local_clock += 2;
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

void CPU::rotate_right(uint8_t* operand) {
  uint8_t new_value = (carry << 7) | (*operand >> 1);
  carry = *operand & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80);
  *operand = new_value;
  local_clock += 2;
}

void CPU::rotate_left(uint8_t* operand) {
  uint8_t new_value = (*operand << 1) | carry;
  carry = (*operand >> 7) & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80);
  *operand = new_value;
  local_clock += 2;
}

void CPU::shift_right(uint8_t* operand) {
  uint8_t new_value = *operand >> 1;
  carry = *operand & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80); // this is basically impossible i assume
  *operand = new_value;
  local_clock += 2;
}

void CPU::arithmetic_shift_left(uint8_t* operand) {
  uint8_t new_value = *operand << 1;
  carry = (*operand >> 7) & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80);
  *operand = new_value;
  local_clock += 2;
}

void CPU::compare(uint8_t register_value, uint8_t* operand) {
  uint8_t argument_value = *operand;
  carry = (register_value >= argument_value);
  zero = (register_value == argument_value);
  sign = ((uint8_t) (register_value - argument_value)) >= 0x80;
}

void CPU::sign_zero_flags(uint8_t val) {
  sign = (val >= 0x80);
  zero = (val == 0);
}

void CPU::branch_on_bool(bool arg, int8_t offset) {
  if (arg) {
    uint16_t original_pc_upper = (PC + 2) >> 8; // upper nibble
    PC += ((int8_t) offset) + 2;
    uint16_t new_pc_upper = PC >> 8;
    // extra cycle if crossed page boundary
    if (original_pc_upper != new_pc_upper) {
      local_clock += 1;
    }
    local_clock += 1;
  } else {
    PC += 2;
  }
  local_clock += 2;
}

/* ADDRESSING MODES POINTER */

uint8_t* CPU::immediate(uint8_t* arg) {
  PC += 2;
  local_clock += 2;
  return arg;
}

uint8_t* CPU::zero_page(uint8_t arg) {
  local_clock += 3;
  PC += 2;
  return memory->get_pointer(arg);
}

uint8_t* CPU::memory_absolute(uint16_t address) {
  local_clock += 4;
  PC += 3;
  return memory->get_pointer(address);
}

uint8_t* CPU::zero_page_indexed_X(uint8_t arg) {
  local_clock += 4;
  PC += 2;
  return memory->get_pointer((arg + X) & 0xff);
}

uint8_t* CPU::zero_page_indexed_Y(uint8_t arg) {
  local_clock += 4;
  PC += 2;
  return memory->get_pointer((arg + Y) & 0xff);
}

uint8_t* CPU::absolute_indexed_X(uint16_t arg, bool override_page_boundary) {
  local_clock += 4;
  uint8_t summed_low_byte = arg + X;
  if (summed_low_byte < X || override_page_boundary) {
    local_clock += 1; // page overflow CPU cycle
  }
  PC += 3;
  return memory->get_pointer(arg + X);
}

uint8_t* CPU::absolute_indexed_Y(uint16_t arg, bool override_page_boundary) {
  local_clock += 4;
  uint8_t summed_low_byte = arg + Y;
  if (summed_low_byte < Y || override_page_boundary) {
    local_clock += 1; // page overflow CPU cycle
  }
  PC += 3;
  return memory->get_pointer(arg + Y);
}

uint8_t* CPU::indexed_indirect(uint8_t arg) {
  local_clock += 6;
  PC += 2;
  uint16_t low_byte = (uint16_t) memory->get_item((arg + X) & 0xff);
  uint16_t high_byte = (uint16_t) memory->get_item((arg + X + 1) & 0xff) << 8;
  return memory->get_pointer(high_byte | low_byte);
}

uint8_t* CPU::indirect_indexed(uint8_t arg, bool override_page_boundary) {
  local_clock += 5;
  uint16_t low_byte = memory->get_item(arg);
  uint16_t high_byte = (uint16_t) memory->get_item((arg + 1) & 0xff) << 8;
  uint8_t summed_low_byte = low_byte + Y;
  if (summed_low_byte < Y || override_page_boundary) {
    local_clock += 1; // page overflow CPU cycle
  }
  PC += 2;
  return memory->get_pointer((high_byte | low_byte) + Y);
}

uint16_t CPU::indirect(uint8_t arg1, uint8_t arg2) {
  uint16_t lower_byte_address = arg2 << 8 | arg1;
  uint16_t upper_byte_address = arg2 << 8 | ((arg1 + 1) & 0xff);
  uint8_t lower_byte = memory->get_item(lower_byte_address);
  uint8_t upper_byte = memory->get_item(upper_byte_address);
  uint16_t new_pc_address = (upper_byte << 8) | lower_byte;
  PC += 3;
  local_clock += 5;
  return new_pc_address;
}

// TODO: only works with mapper 0!
void CPU::reset_game() {
  PC = memory->reset_vector();
  PC = 0xc000;
  SP = 0xfd;
  valid = true;
  interrupt_disable = true;
  b_upper = true;
}
