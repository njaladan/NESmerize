enum AddressingMode {
  IMPLIED,
  IMMEDIATE,
  ZERO_PAGE,
  ZERO_PAGE_X,
  ZERO_PAGE_Y,
  ABSOLUTE,
  ABSOLUTE_X,
  ABSOLUTE_Y,
  INDIRECT,
  INDIRECT_X,
  INDIRECT_Y,
  RELATIVE,
  ACCUMULATOR
};

uint8_t BYTES_PER_MODE[] = {
  1, 2, 2, 2, 2, 3, 3,
  3, 2, 2, 2, 2, 1
};

uint8_t CYCLES_PER_MODE[] = {
  2, 2, 3, 4, 4, 4, 4,
  4, 5, 6, 5, 2, 2
};

uint16_t STACK_OFFSET = 0x100;


class CPU {
  public:
    void set_memory(Memory*);
    uint64_t local_clock;

    // handle state
    void initialize();
    void execute_instruction();

    // for debugging only
    bool valid;

    // external devices can interrupt
    Interrupt interrupt_type;

  private:
    uint8_t accumulator;
    uint8_t X;
    uint8_t Y;
    uint16_t PC;
    // only holds lower byte of the real SP
    uint8_t SP;

    // CPU flags
    bool carry;
    bool zero;
    bool interrupt_disable;
    bool overflow;
    bool sign;
    bool decimal;
    bool b_upper;
    bool b_lower;

    // "hardware" connections
    Memory* memory;

    // keep state during execution
    AddressingMode current_addressing_mode;
    bool extra_cycle_taken;
    bool override_pc_increment;
    bool override_cycle_increment;

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
    void compare(uint8_t);
    void sign_zero_flags(uint8_t);
    void rotate_left();
    void rotate_right();
    void shift_right();
    void arithmetic_shift_left();
    void add_with_carry();
    void subtract_with_carry();
    void add_with_carry_helper(uint8_t);
    void branch_on_bool(bool);
    void increment_subtract();
    void arithmetic_shift_left_or();
    void rotate_left_and();
    void shift_right_xor();
    void rotate_right_add();
    void decrement_compare();
    void load_accumulator_x();
    void bit();
    void decrement();
    void increment();
    void boolean_or();
    void boolean_and();
    void boolean_xor();
    void store_x_and_accumulator();
    void store_x();
    void load_x();

    // addressing modes
    uint16_t absolute_indexed_X(uint16_t);
    uint16_t absolute_indexed_Y(uint16_t);
    uint16_t indexed_indirect(uint8_t);
    uint16_t indirect_indexed(uint8_t);
    uint16_t indirect(uint8_t, uint8_t);

    // memory access syntactic sugar
    uint16_t get_operand();
    uint16_t get_memory_index();
    void store(uint8_t);

    // other
    void check_interrupt();
    void set_addressing_mode();
    void increment_pc_cycles();
    void run_instruction();

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

uint16_t CPU::address_stack_pop() {
  uint8_t lower_byte = stack_pop();
  uint16_t upper_byte = (uint16_t) stack_pop();
  uint16_t val = ((uint16_t) upper_byte << 8) | lower_byte;
  return val;
}

// TODO: make stack_offset a global variable or something?
void CPU::stack_push(uint8_t value) {
  memory->write(STACK_OFFSET + SP, value);
  SP -= 1;
}

uint8_t CPU::stack_pop() {
  uint8_t value = memory->read(STACK_OFFSET + SP + 1);
  SP += 1;
  return value;
}

void CPU::print_register_values() {
  printf("PC:%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%lu\n",
          PC,
          accumulator,
          X,
          Y,
          get_flags_as_byte(),
          SP,
          (local_clock * 3) % 341);
}

// TODO: set B flag correctly
// TODO: check if interrupt reset works correctly
void CPU::check_interrupt() {
  if (interrupt_type &&
    (interrupt_type == NMI || interrupt_disable == false)) {
    address_stack_push(PC);
    stack_push(get_flags_as_byte());
    interrupt_disable = true;
    if (interrupt_type == NMI) {
      PC = memory->nmi_vector();
    } else {
      PC = memory-> irq_vector();
    }
    local_clock += 7;
    interrupt_type = NONE;
  }
}

void CPU::set_addressing_mode() {
  uint8_t opcode = memory->read(PC);
  uint8_t address_mask = opcode & 0x1f;
  uint8_t upper_mask = opcode & 0xe0;
  switch(address_mask) {

    case 0x0:
      current_addressing_mode = IMMEDIATE;
      break;

    case 0x1:
      current_addressing_mode = INDIRECT_X;
      break;

    case 0x2:
      current_addressing_mode = IMMEDIATE;
      break;

    case 0x3:
      current_addressing_mode = INDIRECT_X;
      break;

    case 0x4 ... 0x7:
      current_addressing_mode = ZERO_PAGE;
      break;

    case 0x8:
      current_addressing_mode = IMPLIED;
      break;

    case 0x9:
      current_addressing_mode = IMMEDIATE;
      break;

    case 0xa:
      current_addressing_mode = ACCUMULATOR;
      break;

    case 0xb:
      current_addressing_mode = IMMEDIATE;
      break;

  	case 0xc ... 0xf:
      current_addressing_mode = ABSOLUTE;
      break;

  	case 0x10:
      current_addressing_mode = RELATIVE;
      break;

  	case 0x11 ... 0x13:
      current_addressing_mode = INDIRECT_Y;
      break;

  	case 0x14 ... 0x17:
      current_addressing_mode = ZERO_PAGE_X;
      break;

    case 0x18:
      current_addressing_mode = IMPLIED;
      break;

  	case 0x19:
      current_addressing_mode = ABSOLUTE_Y;
      break;

    case 0x1a:
      current_addressing_mode = IMPLIED;
      break;

    case 0x1b:
      current_addressing_mode = ABSOLUTE_Y;
      break;

  	case 0x1c ... 0x1f:
      current_addressing_mode = ABSOLUTE_X;
      break;
  }

  // exceptions
  if (opcode == 0x20) {
    current_addressing_mode = ABSOLUTE;
  } else if (opcode == 0x6c) {
    current_addressing_mode = INDIRECT;
  } else if (
    upper_mask >= 0x80 &&
    upper_mask <= 0xa0 &&
    address_mask >= 0x16 &&
    address_mask <= 0x17
  ) {
    current_addressing_mode = ZERO_PAGE_Y;
  } else if (
    upper_mask >= 0x80 &&
    upper_mask <= 0xa0 &&
    address_mask >= 0x1e &&
    address_mask <= 0x1f
  ) {
    current_addressing_mode = ABSOLUTE_Y;
  }
}

void CPU::execute_instruction() {
  override_pc_increment = false;
  override_cycle_increment = false;
  extra_cycle_taken = false;
  check_interrupt();
  set_addressing_mode();
  // print_register_values();
  run_instruction();
  increment_pc_cycles();
}

void CPU::increment_pc_cycles() {
  if (!override_pc_increment) {
    PC += BYTES_PER_MODE[current_addressing_mode];
  }
  if (!override_cycle_increment) {
    local_clock += CYCLES_PER_MODE[current_addressing_mode];
    local_clock += extra_cycle_taken;
  }
}

void CPU::run_instruction() {
  uint8_t opcode = memory->read(PC);
  uint8_t arg1 = memory->read(PC + 1);
  uint8_t arg2 = memory->read(PC + 2);
  uint16_t address = arg2 << 8 | arg1;

  uint8_t opcode_block = opcode & 0x3;
  uint8_t address_mask = opcode & 0x1f;
  uint8_t upper_mask = opcode & 0xe0;

  // branches
  if (address_mask == 0x10) {
    bool flags[] = {!sign, sign, !overflow, overflow,
                    !carry, carry, !zero, zero};
    branch_on_bool(flags[opcode >> 5]);
    return;
  }

  // STP - stop execution
  if (address_mask == 0x12) {
    valid = false;
    return;
  }

  if (address_mask == 0x18) {

    // TYA
    if (upper_mask == 0x80) {
      accumulator = Y;
      sign_zero_flags(accumulator);
      return;
    }

    // set / clear flags
    bool* register_pointer[] = {
      &carry,
      &interrupt_disable,
      &overflow,
      &decimal
    };
    uint8_t register_select = opcode >> 6;
    uint8_t value_select = (opcode >> 5) & 0x1;
    // CLV is special
    if (register_select == 2) {
      *(register_pointer[register_select]) = 0;
    } else {
      *(register_pointer[register_select]) = value_select;
    }
    return;
  }

  if (address_mask == 0x08) {
    switch(upper_mask) {

      // PHP
      case 0x00:
        local_clock += 1;
        return stack_push(get_flags_as_byte());

      // PLP
      case 0x20:
        local_clock += 2;
        return set_flags_from_byte(stack_pop());

      // PHA
      case 0x40:
        local_clock += 1;
        return stack_push(accumulator);

      // PLA
      case 0x60:
        local_clock += 2;
        accumulator = stack_pop();
        return sign_zero_flags(accumulator);

      // DEY
      case 0x80:
        Y = Y - 1;
        sign_zero_flags(Y);
        return;

      // TAY
      case 0xa0:
        Y = accumulator;
        sign_zero_flags(Y);
        return;

      // INY
      case 0xc0:
        Y += 1;
        sign_zero_flags(Y);
        return;

      // INX
      case 0xe0:
        X += 1;
        sign_zero_flags(X);
        return;
    }
  }

  // undocumented opcodes
  if (opcode_block == 3) {
    if (upper_mask <= 0x60) {
      local_clock += 2;
    }
    switch(upper_mask) {

      // SLO
      case 0x00:
        return arithmetic_shift_left_or();

      // RLA
      case 0x20:
        return rotate_left_and();

      // SRE
      case 0x40:
        return shift_right_xor();

      // RRA
      case 0x60:
        return rotate_right_add();

      // SAX
      case 0x80:
        return store_x_and_accumulator();

      // LAX
      case 0xa0:
        return load_accumulator_x();

      // DCP
      case 0xc0:
        return decrement_compare();

      // ISC
      case 0xe0:
        return increment_subtract();
    }
  }

  // RMW operations
  if (opcode_block == 2) {
    if (current_addressing_mode == IMPLIED) {
      switch(opcode) {
        case 0x9a:
          return store_x();

        case 0xba:
          return load_x();

        default:
          return;
      }
    }
    if (upper_mask <= 0x60) {
      if (current_addressing_mode != ACCUMULATOR) {
        local_clock += 2; // shift extra cycles
      }
      if (current_addressing_mode == ABSOLUTE_X) {
        extra_cycle_taken = true;
      }
    }

    switch(upper_mask) {
      // ASL
      case 0x00:
        return arithmetic_shift_left();

      // ROL
      case 0x20:
        return rotate_left();

      // LSR
      case 0x40:
        return shift_right();

      // ROR
      case 0x60:
        return rotate_right();

      // store X related
      case 0x80:
        return store_x();

      // LDX and misc.
      case 0xa0:
        return load_x();

      // DEC / DEX
      case 0xc0:
        return decrement();

      // INC
      case 0xe0:
        return increment();
    }
  }

  // ALU operations
  if (opcode_block == 1) {
    switch(upper_mask) {

      // ORA
      case 0x00:
        return boolean_or();

      // AND
      case 0x20:
        return boolean_and();

      // EOR
      case 0x40:
        return boolean_xor();

      // ADC
      case 0x60:
        return add_with_carry();

      // STA
      case 0x80:
        if (
          current_addressing_mode == ABSOLUTE_X ||
          current_addressing_mode == ABSOLUTE_Y ||
          current_addressing_mode == INDIRECT_Y
        ) {
          extra_cycle_taken = true;
        }
        return store(accumulator);

      // LDA
      case 0xa0:
        accumulator = get_operand();
        sign_zero_flags(accumulator);
        return;

      // CMP
      case 0xc0:
        return compare(accumulator);

      // SBC
      case 0xe0:
        return subtract_with_carry();
    }
  }

  // control instructions [must be last]
  if (opcode_block == 0) {
    // NOP
    if (
      address_mask >= 0x14 &&
      (upper_mask >= 0xc0 ||
      upper_mask <= 0x60)
    ) {
      get_operand();
      return;
    }

    switch(upper_mask) {

      // CPX
      case 0xe0:
        return compare(X);

      // CPY
      case 0xc0:
        return compare(Y);

      // LDY
      case 0xa0:
        Y = get_operand();
        sign_zero_flags(Y);
        return;

      // STY
      case 0x80:
        return store(Y);
    }
  }

  // outliers
  switch(opcode) {

    // BRK
    case 0x00:
      address_stack_push(PC + 2);
      b_lower = true;
      stack_push(get_flags_as_byte());
      b_lower = false;
      interrupt_disable = true;
      PC = memory->irq_vector();
      override_pc_increment = true;
      local_clock += 7;
      break;

    // JSR
    case 0x20:
      address_stack_push(PC + 2);
      PC = address;
      override_pc_increment = true;
      override_cycle_increment = true;
      local_clock += 6;
      break;

    // BIT zero page
    case 0x24:
      return bit();

    // BIT absolute
    case 0x2c:
      return bit();

    // RTI
    case 0x40:
      set_flags_from_byte(stack_pop());
      PC = address_stack_pop();
      override_pc_increment = true;
      override_cycle_increment = true;
      local_clock += 6;
      break;

    // JMP absolute
    case 0x4c:
      PC = address;
      override_pc_increment = true;
      override_cycle_increment = true;
      local_clock += 3; // special case
      break;

    // RTS
    case 0x60:
      PC = address_stack_pop() + 1; // add one to stored address
      override_pc_increment = true;
      override_cycle_increment = true;
      local_clock += 6;
      break;

    // JMP indirect
    case 0x6c:
      PC = get_operand();
      override_pc_increment = true;
      break;
  }
}

void CPU::boolean_xor() {
  accumulator ^= get_operand();
  sign_zero_flags(accumulator);
}

void CPU::boolean_and() {
  accumulator &= get_operand();
  sign_zero_flags(accumulator);
}

void CPU::boolean_or() {
  accumulator |= get_operand();
  sign_zero_flags(accumulator);
}

void CPU::increment() {
  // NOP
  if (current_addressing_mode == ACCUMULATOR) {
    return;
  } else {
    store(get_operand() + 1);
    sign_zero_flags(get_operand());
    extra_cycle_taken = (current_addressing_mode == ABSOLUTE_X);
    local_clock += 2;
  }
}

void CPU::decrement() {
  // DEX
  if (current_addressing_mode == ACCUMULATOR) {
    X = X - 1;
    sign_zero_flags(X);
  } else {
    store(get_operand() - 1);
    sign_zero_flags(get_operand());
    extra_cycle_taken = (current_addressing_mode == ABSOLUTE_X);
    local_clock += 2;
  }
}

void CPU::store_x() {
  if (current_addressing_mode == IMPLIED) {
    SP = X;
  } else {
    store(X);
  }
  if (current_addressing_mode == ACCUMULATOR) {
    sign_zero_flags(X);
  }
}

void CPU::load_x() {
  if (current_addressing_mode == IMPLIED) {
    X = SP;
  } else {
    X = get_operand();
  }
  sign_zero_flags(X);
}

void CPU::bit() {
  uint8_t value = get_operand();
  zero = ((value & accumulator) == 0);
  overflow = (value >> 6) & 1;
  sign = value >= 0x80;
}

void CPU::load_accumulator_x() {
  accumulator = X = get_operand();
  sign_zero_flags(accumulator);
}

void CPU::store_x_and_accumulator() {
  store(X & accumulator);
}

void CPU::decrement_compare() {
  store(get_operand() - 1);
  sign_zero_flags((accumulator - get_operand()) & 0xff);
  local_clock += 2;
}

void CPU::rotate_right_add() {
  rotate_right();
  add_with_carry();
}

void CPU::shift_right_xor() {
  shift_right();
  accumulator ^= get_operand();
  sign_zero_flags(accumulator);
}

void CPU::rotate_left_and() {
  rotate_left();
  accumulator &= get_operand();
  sign_zero_flags(accumulator);
}

void CPU::arithmetic_shift_left_or() {
  arithmetic_shift_left();
  accumulator |= get_operand();
  sign_zero_flags(accumulator);
}

void CPU::increment_subtract() {
  store(get_operand() + 1);
  subtract_with_carry();
  if (current_addressing_mode != IMMEDIATE) {
    local_clock += 2;
  }
}

void CPU::add_with_carry() {
  add_with_carry_helper(get_operand());
}

void CPU::subtract_with_carry() {
  add_with_carry_helper(~get_operand());
}

void CPU::add_with_carry_helper(uint8_t operand) {
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

void CPU::rotate_right() {
  uint8_t value = get_operand();
  uint8_t new_value = (carry << 7) | (value >> 1);
  carry = value & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80);
  store(new_value);
}

void CPU::rotate_left() {
  uint8_t value = get_operand();
  uint8_t new_value = (value << 1) | carry;
  carry = (value >> 7) & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80);
  store(new_value);
}

void CPU::shift_right() {
  uint8_t value = get_operand();
  uint8_t new_value = value >> 1;
  carry = value & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80); // this isq basically impossible i assume
  store(new_value);
}

void CPU::arithmetic_shift_left() {
  uint8_t value = get_operand();
  uint8_t new_value = value << 1;
  carry = (value >> 7) & 0x1;
  zero = (new_value == 0);
  sign = (new_value >= 0x80);
  store(new_value);
}

void CPU::compare(uint8_t register_value) {
  uint8_t argument_value = get_operand();
  carry = (register_value >= argument_value);
  zero = (register_value == argument_value);
  sign = ((uint8_t) (register_value - argument_value)) >= 0x80;
}

void CPU::sign_zero_flags(uint8_t val) {
  sign = (val >= 0x80);
  zero = (val == 0);
}

void CPU::store(uint8_t value) {
  uint8_t arg1 = memory->read(PC + 1);
  switch(current_addressing_mode) {
    case IMPLIED:
      return;

    case IMMEDIATE:
      return;

    case RELATIVE:
      return;

    case ACCUMULATOR:
      accumulator = value;
      return;

    default:
      return memory->write(get_memory_index(), value);
  }
}

uint16_t CPU::get_memory_index() {
  uint8_t arg1 = memory->read(PC + 1);
  uint8_t arg2 = memory->read(PC + 2);
  uint16_t address = arg2 << 8 | arg1;
  switch(current_addressing_mode) {

    case ZERO_PAGE:
      return arg1;

    case ZERO_PAGE_Y:
      return (arg1 + Y) & 0xff;

    case ZERO_PAGE_X:
      return (arg1 + X) & 0xff;

    case ABSOLUTE:
      return address;

    case ABSOLUTE_X:
      return absolute_indexed_X(address);

    case ABSOLUTE_Y:
      return absolute_indexed_Y(address);

    case INDIRECT_X:
      return indexed_indirect(arg1);

    case INDIRECT_Y:
      return indirect_indexed(arg1);
  }
}

uint16_t CPU::get_operand() {
  uint8_t arg1 = memory->read(PC + 1);
  uint8_t arg2 = memory->read(PC + 2);
  switch(current_addressing_mode) {
    case IMPLIED:
      return 0;

    case IMMEDIATE:
      return arg1;

    case ACCUMULATOR:
      return accumulator;

    case INDIRECT:
      return indirect(arg1, arg2);

    default:
      return memory->read(get_memory_index());
  }
}

void CPU::branch_on_bool(bool arg) {
  if (arg) {
    uint8_t arg1 = memory->read(PC + 1);
    uint16_t new_pc = PC + ((int8_t) arg1) + 2;
    uint16_t original_pc_upper = (PC + 2) >> 8;
    uint16_t new_pc_upper = new_pc >> 8;
    if (original_pc_upper != new_pc_upper) {
      extra_cycle_taken = true; // page boundary crossed
    }
    local_clock += 1;
    override_pc_increment = true;
    PC = new_pc;
  }
}

uint16_t CPU::absolute_indexed_X(uint16_t arg) {
  uint8_t summed_low_byte = arg + X;
  if (summed_low_byte < X) {
    extra_cycle_taken = true; // page overflow CPU cycle
  }
  return arg + X;
}

uint16_t CPU::absolute_indexed_Y(uint16_t arg) {
  uint8_t summed_low_byte = arg + Y;
  if (summed_low_byte < Y) {
    extra_cycle_taken = true;
  }
  return arg + Y;
}

uint16_t CPU::indexed_indirect(uint8_t arg) {
  uint8_t low_byte_address = (arg + X) & 0xff;
  uint8_t high_byte_address = (arg + X + 1) & 0xff;
  uint16_t low_byte = memory->read(low_byte_address);
  uint16_t high_byte = memory->read(high_byte_address) << 8;
  return high_byte | low_byte;
}

uint16_t CPU::indirect_indexed(uint8_t arg) {
  uint8_t high_byte_address = (arg + 1) & 0xff;
  uint16_t low_byte = memory->read(arg);
  uint16_t high_byte = memory->read(high_byte_address) << 8;
  uint8_t summed_low_byte = low_byte + Y;
  if (summed_low_byte < Y) {
    extra_cycle_taken = true; // page overflow CPU cycle
  }
  return (high_byte | low_byte) + Y;
}

uint16_t CPU::indirect(uint8_t arg1, uint8_t arg2) {
  uint16_t lower_byte_address = arg2 << 8 | arg1;
  uint16_t upper_byte_address = arg2 << 8 | ((arg1 + 1) & 0xff);
  uint8_t lower_byte = memory->read(lower_byte_address);
  uint8_t upper_byte = memory->read(upper_byte_address);
  uint16_t address = (upper_byte << 8) | lower_byte;
  return address;
}

void CPU::initialize() {
  accumulator = X = Y = 0;
  PC = memory->reset_vector();
  PC = 0xc000; // just for nestest
  SP = 0xfd;
  valid = true;
  interrupt_disable = true;
  b_upper = true;
}
