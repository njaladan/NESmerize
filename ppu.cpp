#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>


#include "cpu.cpp"

using namespace std;

class PPU {

  // accessible objects
  Memory memory;
  CPU cpu;




  PPU(Memory, CPU);


}

// check paradigm for this?
PPU::PPU(Memory mem, CPU daddy) {
  this.memory = mem;
  this.cpu = daddy;
}
