# nes-emu

An NES emulator written in C++. Currently in a state of progress, but hoping to finish this up before 2018 ends.



## To-do

A to-do list, (roughly) in order of priority.

- [x] All 6502 CPU opcodes implemented and working
- [ ] PPU implementation
- [ ] memory map with pointers instead of writing things manually to a memory array
- [ ] memory mappers 0, 1, 2
- [ ] SDL gui
- [ ] PPU scrolling
- [ ] APU support [stretch goal?]
- [ ] save support



### Code clarity ideas
The code's pretty messy, since I've been prioritizing quantity of quality (at the moment). Here are a few things that I think can make it better.
- [ ] use upper / lower nibbles to determine instruction instead of a big switch? or a jump table (maybe virtual)
- [ ] enums for addressing modes - especially since they're formulaic
- [ ] wrapper functions for things like EOR, AND, ORA, etc. since the only difference is addressing modes
- [x] object organization -> master class for NES object



### Current to-dos

Smaller things to work on incrementally - more for me to not forget what I'm working on
- [ ] STA always increments cycle in favor of page turn - how to force this?
- [ ] unofficial opcodes, starting at LAX
- [ ] keep in mind that mapper 0 is hardcoded into current logic (but that's still 250 games)


## Resources

[NES Development Guide](http://nesdev.com/NESDoc.pdf)

[NESTest](http://www.qmtpro.com/~nes/misc/nestest.txt)
