# nes-emu

An NES emulator written in C++.


## To-do

A to-do list, (roughly) in order of priority.

- [x] All 6502 CPU opcodes implemented and working
- [ ] PPU implementation
- [x] iNES memory mapper 0 [NROM]
- [ ] SDL gui
- [ ] PPU scrolling
- [ ] APU support [stretch goal?]
- [ ] memory mappers 1, 2
- [ ] save support



### Code clarity ideas
The code's pretty messy, since I've been prioritizing quantity of quality (at the moment). Here are a few things that I think can make it better.
- [x] enums for addressing modes - especially since they're formulaic
- [x] wrapper functions for things like EOR, AND, ORA, etc. since the only difference is addressing modes
- [x] object organization -> master class for NES object
- [ ] consider passing object / struct as a parameter? instead of function signature changes
- [x] refactor the opcode switch completely [low priority]
- [x] have a instance variable for "extra cycle" flag
- [ ] use a function for detecting upper byte overflow?
- [ ] once running nes roms, remove unofficial opcode implementations
- [ ] move enums and initializer lists to separate file


### Current to-dos

Smaller things to work on incrementally - more for me to not forget what I'm working on
- [x] STA always increments cycle in favor of page turn - how to force this?
- [x] unofficial opcodes, starting at LAX
- [x] keep in mind that mapper 0 is hardcoded into current logic (but that's still 250 games)


## Benchmarks
### cpu.cpp

case-switch for all opcodes: ~400musec
logical choose for each instruction: ~1500musec
prebuilt opcode table: ~1000musec

possible to combine both? pre-generate jump table with opcode objects at initialization using logical choice [or just hardcode, either way]


## Resources

[NES Development Guide](http://nesdev.com/NESDoc.pdf)

[NESTest](http://www.qmtpro.com/~nes/misc/nestest.txt)
