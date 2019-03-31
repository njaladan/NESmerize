uint16_t STACK_OFFSET = 0x100;

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
  printf("PC:%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3lu SL:%lu\n",
          PC,
          accumulator,
          X,
          Y,
          get_flags_as_byte(),
          SP,
          get_current_cycle(),
          get_current_scanline());
}

// TODO: set B flag correctly, check if this even works
void CPU::check_interrupt() {
  if (interrupt_type &&
    (interrupt_type == NMI || interrupt_disable == false)) {
    address_stack_push(PC);
    stack_push(get_flags_as_byte());
    interrupt_disable = true;
    if (interrupt_type == NMI) {
      PC = memory->nmi_vector();
    } else {
      PC = memory->irq_vector();
    }
    local_clock += 7;
    interrupt_type = NONE; // reset interrupt line
  }
}

uint64_t CPU::get_current_scanline() {
  // don't ask me
  return (241 + (local_clock * 3) / 341) % 262;
}

uint64_t CPU::get_current_cycle() {
  return (local_clock * 3) % 341;
}

void CPU::generate_nmi() {
  interrupt_type = NMI;
}

void CPU::execute_instruction() {
  override_pc_increment = false;
  extra_cycle_taken = false;
  check_interrupt();
  // print_register_values();
  run_instruction();
  increment_pc_cycles();
}

void CPU::increment_pc_cycles() {
  if (!override_pc_increment) {
    PC += current_opcode.instruction_length;
  }
  local_clock += current_opcode.cycles + extra_cycle_taken;
}

void CPU::set_ppu(PPU* ppu_pointer) {
  ppu = ppu_pointer;
  local_clock = 0;
}

void CPU::run_instruction() {
  uint8_t opcode = memory->read(PC);
  current_opcode = opcodes[opcode];
  CPUFunction current_instruction = current_opcode.instruction;

  // vblank catch up
  if (get_current_scanline() == 240 &&
      (get_current_cycle() + current_opcode.cycles * 3 > 340))
     {
       ppu->step_to((local_clock + current_opcode.cycles) * 3);
  }

  switch(current_instruction) {

    case ADC:
      return add_with_carry();

    case AND:
      return boolean_and();

    case ASL:
      return arithmetic_shift_left();

    case BCC:
      return branch_on_bool(!carry);

    case BCS:
      return branch_on_bool(carry);

    case BEQ:
      return branch_on_bool(zero);

    case BIT:
      return bit();

    case BMI:
      return branch_on_bool(sign);

    case BNE:
      return branch_on_bool(!zero);

    case BPL:
      return branch_on_bool(!sign);

    case BRK:
      // probably corrupted heap :(
      exit(1);
      address_stack_push(PC + 2);
      b_lower = true;
      stack_push(get_flags_as_byte());
      b_lower = false;
      interrupt_disable = true;
      PC = memory->irq_vector();
      override_pc_increment = true;
      return;

    case BVC:
      return branch_on_bool(!overflow);

    case BVS:
      return branch_on_bool(overflow);

    case CLC:
      carry = false;
      return;

    case CLD:
      decimal = false;
      return;

    case CLI:
      interrupt_disable = false;
      return;

    case CLV:
      overflow = false;
      return;

    case CMP:
      return compare(accumulator);

    case CPX:
      return compare(X);

    case CPY:
      return compare(Y);

    case DEC:
      return decrement();

    case DEX:
      X = X - 1;
      sign_zero_flags(X);
      return;

    case DEY:
      Y = Y - 1;
      sign_zero_flags(Y);
      return;

    case EOR:
      return boolean_xor();

    case INC:
      return increment();

    case INX:
      X = X + 1;
      sign_zero_flags(X);
      return;

    case INY:
      Y = Y + 1;
      sign_zero_flags(Y);
      return;

    case JMP:
      return jump(); // fill this in

    case JSR:
      address_stack_push(PC + 2);
      return jump(); // fill in

    case LDA:
      return load_into_register(&accumulator);

    case LDX:
      return load_into_register(&X);

    case LDY:
      return load_into_register(&Y);

    case LSR:
      return shift_right();

    case NOP:
      get_operand();
      return;

    case ORA:
      return boolean_or();

    case PHA:
      return stack_push(accumulator);

    case PHP:
      return stack_push(get_flags_as_byte());

    case PLA:
      accumulator = stack_pop();
      return sign_zero_flags(accumulator);

    case PLP:
      return set_flags_from_byte(stack_pop());

    case ROL:
      return rotate_left();

    case ROR:
      return rotate_right();

    case RTI:
      set_flags_from_byte(stack_pop());
      PC = address_stack_pop();
      override_pc_increment = true;
      return;

    case RTS:
      PC = address_stack_pop() + 1; // add one to stored address
      override_pc_increment = true;
      return;

    case SBC:
      return subtract_with_carry();

    case SEC:
      carry = true;
      return;

    case SED:
      decimal = true;
      return;

    case SEI:
      interrupt_disable = true;
      return;

    case STA:
      return store(accumulator);

    case STX:
      return store(X);

    case STY:
      return store(Y);

    case TAX:
      X = accumulator;
      return sign_zero_flags(X);

    case TAY:
      Y = accumulator;
      return sign_zero_flags(Y);

    case TSX:
      X = SP;
      return sign_zero_flags(X);

    case TXA:
      accumulator = X;
      return sign_zero_flags(accumulator);

    case TXS:
      SP = X;
      return;

    case TYA:
      accumulator = Y;
      return sign_zero_flags(accumulator);

    case ISC:
      return increment_subtract();

    case SLO:
      return arithmetic_shift_left_or();

    case RLA:
      return rotate_left_and();

    case SRE:
      return shift_right_xor();

    case RRA:
      return rotate_right_add();

    case DCP:
      return decrement_compare();

    case LAX:
      return load_accumulator_x();

    case AHX:
      return;

    case STP:
      valid = false;
      return;

    case SAX:
      return store_x_and_accumulator();

    case TAS:
      return;

    case SHY:
      return;

    case SHX:
      return;

    case LAS:
      return;

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
  store(get_operand() + 1);
  sign_zero_flags(get_operand());
}

void CPU::decrement() {
  store(get_operand() - 1);
  sign_zero_flags(get_operand());
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
  sign = (new_value >= 0x80);
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

void CPU::jump() {
  uint8_t arg1 = memory->read(PC + 1);
  uint8_t arg2 = memory->read(PC + 2);
  uint16_t address = arg2 << 8 | arg1;
  override_pc_increment = true;
  if (current_opcode.addressing_mode == ABSOLUTE) {
    PC = address;
  } else {
    PC = get_operand();
  }
}

void CPU::sign_zero_flags(uint8_t val) {
  sign = (val >= 0x80);
  zero = (val == 0);
}

void CPU::load_into_register(uint8_t* reg) {
  *reg = get_operand();
  sign_zero_flags(*reg);
}

void CPU::store(uint8_t value) {
  uint8_t arg1 = memory->read(PC + 1);
  switch(current_opcode.addressing_mode) {
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
  switch(current_opcode.addressing_mode) {

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
  switch(current_opcode.addressing_mode) {
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
  SP = 0xfd;
  valid = true;
  interrupt_disable = true;
  b_upper = true;
  OpcodeGenerator gen;
  opcodes = gen.generate_all_opcodes();
}
