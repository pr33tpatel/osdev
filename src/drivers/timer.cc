#include <drivers/timer.h>

using namespace os;
using namespace os::common;
using namespace os::drivers;
using namespace os::hardwarecommunication;
using namespace os::utils;

ProgrammableIntervalTimer::ProgrammableIntervalTimer(InterruptManager* interrupts, uint32_t frequency)
: InterruptHandler(interrupts, interrupts->HardwareInterruptOffset() + 0),
  dataPort(0x40),
  commandPort(0x43)
{
  ticks = 0; // initalize ticks to be 0 at boot
  uint32_t internalOscillator = 1193182; 
  uint32_t divisor = internalOscillator / frequency;

  commandPort.Write(0x36); // send the command Byte

  // send divisor, first the lowByte, then the highByte
  uint8_t divisor_lowByte = (uint8_t)(divisor & 0xFF);
  uint8_t divisor_highByte = (uint8_t)((divisor >> 8) & 0xFF);

  dataPort.Write(divisor_lowByte);
  dataPort.Write(divisor_highByte);

  printf(RED_COLOR, BLACK_COLOR, "PIT initalized at %dHz", frequency);
}

uint32_t ProgrammableIntervalTimer::HandleInterrupt(uint32_t esp) {
  ticks++;
  return esp;
}

void ProgrammableIntervalTimer::Wait(uint32_t milliseconds) {


  uint32_t ticksToWait = milliseconds;
  uint64_t endTicks = ticks + ticksToWait;

  // [busy wait]
  while (ticks < endTicks) {
    asm volatile("nop"); 
  }
}
