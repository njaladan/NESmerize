class Memory;
class PPU;

class CPU {
  public:
    void set_memory(Memory*);
    void set_ppu(PPU*);
    uint64_t local_clock;

    // handle state
    void initialize();
    void execute_instruction();
    void generate_nmi();

    // for debugging only
    bool valid;

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
    Interrupt interrupt_type;

    // keep state during execution
    Opcode current_opcode;
    bool override_pc_increment;
    bool extra_cycle_taken;

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
    void jump();
    void load_into_register(uint8_t*);

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
    void increment_pc_cycles();
    void run_instruction();
    uint64_t get_current_cycle();
    uint64_t get_current_scanline();
    Opcode* opcodes;
    PPU* ppu;

};
