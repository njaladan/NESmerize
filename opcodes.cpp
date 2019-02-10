
/*
 ok this actually should maybe all  be deleted because the jump
 table isn't created in memory which is pretty bad for optimizations
 this is created at runtime which is sloww w w w   w

 although i think this is a cute implementation </3
*/


enum CPUFunction {
  ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK,
  BVC, BVS, CLC, CLD, CLI, CLV, CMP, CPX, CPY, DEC, DEX,
  DEY, EOR, INC, INX, INY, JMP, JSR, LDA, LDX, LDY, LSR,
  NOP, ORA, PHA, PHP, PLA, PLP, ROL, ROR, RTI, RTS, SBC,
  SEC, SED, SEI, STA, STX, STY, TAX, TAY, TSX, TXA, TXS,
  TYA, ISC, SLO, RLA, SRE, RRA, DCP, LAX, AHX, STP, SAX,
  TAS, SHY, SHX, LAS
};

enum AddressingMode {
  IMPLIED, IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X,
  ZERO_PAGE_Y, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y,
  INDIRECT, INDIRECT_X, INDIRECT_Y, RELATIVE,
  ACCUMULATOR
};

uint8_t BYTES_PER_MODE[] = {
  1, 2, 2, 2, 2, 3, 3,
  3, 2, 2, 2, 2, 1
};

CPUFunction ALL_FUNCTIONS[] = {
  BRK, ORA, STP, SLO, NOP, ORA, ASL, SLO, PHP, ORA,
  ASL, NOP, NOP, ORA, ASL, SLO, BPL, ORA, STP, SLO,
  NOP, ORA, ASL, SLO, CLC, ORA, NOP, SLO, NOP, ORA,
  ASL, SLO, JSR, AND, STP, RLA, BIT, AND, ROL, RLA,
  PLP, AND, ROL, NOP, BIT, AND, ROL, RLA, BMI, AND,
  STP, RLA, NOP, AND, ROL, RLA, SEC, AND, NOP, RLA,
  NOP, AND, ROL, RLA, RTI, EOR, STP, SRE, NOP, EOR,
  LSR, SRE, PHA, EOR, LSR, NOP, JMP, EOR, LSR, SRE,
  BVC, EOR, STP, SRE, NOP, EOR, LSR, SRE, CLI, EOR,
  NOP, SRE, NOP, EOR, LSR, SRE, RTS, ADC, STP, RRA,
  NOP, ADC, ROR, RRA, PLA, ADC, ROR, NOP, JMP, ADC,
  ROR, RRA, BVS, ADC, STP, RRA, NOP, ADC, ROR, RRA,
  SEI, ADC, NOP, RRA, NOP, ADC, ROR, RRA, NOP, STA,
  NOP, SAX, STY, STA, STX, SAX, DEY, NOP, TXA, NOP,
  STY, STA, STX, SAX, BCC, STA, STP, AHX, STY, STA,
  STX, SAX, TYA, STA, TXS, TAS, SHY, STA, SHX, AHX,
  LDY, LDA, LDX, LAX, LDY, LDA, LDX, LAX, TAY, LDA,
  TAX, NOP, LDY, LDA, LDX, LAX, BCS, LDA, STP, LAX,
  LDY, LDA, LDX, LAX, CLV, LDA, TSX, LAS, LDY, LDA,
  LDX, LAX, CPY, CMP, NOP, DCP, CPY, CMP, DEC, DCP,
  INY, CMP, DEX, NOP, CPY, CMP, DEC, DCP, BNE, CMP,
  STP, DCP, NOP, CMP, DEC, DCP, CLD, CMP, NOP, DCP,
  NOP, CMP, DEC, DCP, CPX, SBC, NOP, ISC, CPX, SBC,
  INC, ISC, INX, SBC, NOP, SBC, CPX, SBC, INC, ISC,
  BEQ, SBC, STP, ISC, NOP, SBC, INC, ISC, SED, SBC,
  NOP, ISC, NOP, SBC, INC, ISC
};

uint8_t ALL_CYCLES[] = {
  7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 0, 4, 6, 6, 2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 0, 4, 6, 6, 2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, 2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 7, 6, 2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 6, 7,
  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, 2, 6, 0, 5, 4, 4, 4, 4, 2, 5, 2, 4, 4, 5, 6, 7,
  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, 2, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 7,
  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, 2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, 2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};

AddressingMode ALL_ADDRESSES[] = {
  IMMEDIATE, INDIRECT_X, IMMEDIATE, INDIRECT_X, ZERO_PAGE,
  ZERO_PAGE, ZERO_PAGE, ZERO_PAGE, IMPLIED, IMMEDIATE,
  ACCUMULATOR, IMMEDIATE, ABSOLUTE, ABSOLUTE, ABSOLUTE,
  ABSOLUTE, RELATIVE, INDIRECT_Y, INDIRECT_Y, INDIRECT_Y,
  ZERO_PAGE_X, ZERO_PAGE_X, ZERO_PAGE_X, ZERO_PAGE_X,
  IMPLIED, ABSOLUTE_Y, IMPLIED, ABSOLUTE_Y, ABSOLUTE_X,
  ABSOLUTE_X, ABSOLUTE_X, ABSOLUTE_X
};

// TODO: consider using std::array
CPUFunction no_page_boundary[] = {ASL, DEC, INC, LSR, ROL, ROR, STA};

class Opcode {
  public:
    uint8_t opcode;
    uint8_t cycles;
    uint8_t instruction_length;
    AddressingMode addressing_mode;
    bool page_boundary_cycle_allowed;
    CPUFunction instruction;
};

class OpcodeGenerator {
  public:
    Opcode* generate_all_opcodes();
    void set_addressing_mode(Opcode& op);
    void set_length_cycles(Opcode& op);
    void set_instruction(Opcode& op);
};

void OpcodeGenerator::set_addressing_mode(Opcode& op) {
  uint8_t opcode = op.opcode;
  uint8_t address_mask = opcode & 0x1f;
  uint8_t upper_mask = opcode & 0xe0;
  AddressingMode opcode_admode;

  opcode_admode = ALL_ADDRESSES[address_mask];

  // exceptions
  if (opcode == 0x20) {
    opcode_admode = ABSOLUTE;
  } else if (opcode == 0x6c) {
    opcode_admode = INDIRECT;
  } else if (
    upper_mask >= 0x80 &&
    upper_mask <= 0xa0 &&
    address_mask >= 0x16 &&
    address_mask <= 0x17
  ) {
    opcode_admode = ZERO_PAGE_Y;
  } else if (
    upper_mask >= 0x80 &&
    upper_mask <= 0xa0 &&
    address_mask >= 0x1e &&
    address_mask <= 0x1f
  ) {
    opcode_admode = ABSOLUTE_Y;
  }
  op.addressing_mode = opcode_admode;
}

void OpcodeGenerator::set_length_cycles(Opcode& op) {
  AddressingMode addressing_mode = op.addressing_mode;
  op.instruction_length = BYTES_PER_MODE[addressing_mode];
  op.cycles = ALL_CYCLES[op.opcode];
}

void OpcodeGenerator::set_instruction(Opcode& op) {
  op.instruction = ALL_FUNCTIONS[op.opcode];
  if (
    op.addressing_mode == ABSOLUTE_X ||
    op.addressing_mode == ABSOLUTE_Y ||
    op.addressing_mode == INDIRECT_Y
  ) {
    bool set_page_boundary = true;
    for (int i; i < 7; ++i) {
      if (op.instruction == no_page_boundary[i]) {
        set_page_boundary = false;
        break;
      }
    }
  }
}

Opcode* OpcodeGenerator::generate_all_opcodes() {
  static Opcode all_opcodes[256];
  for (int i = 0; i <= 0xff; ++i) {
    Opcode current = all_opcodes[i];
    current.opcode = i;
    all_opcodes[i] = current;
    set_addressing_mode(current);
    set_length_cycles(current);
    set_instruction(current);
    all_opcodes[i] = current;
  }
  return all_opcodes;
}
