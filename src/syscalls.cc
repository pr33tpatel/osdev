#include <syscalls.h>

using namespace os;
using namespace os::common;
using namespace os::utils;
using namespace os::hardwarecommunication;


SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
    : InterruptHandler(interruptManager, InterruptNumber + interruptManager->HardwareInterruptOffset()) {
}

SyscallHandler::~SyscallHandler() {
}

uint32_t SyscallHandler::HandleInterrupt(uint32_t esp) {
  CPUState* cpu = (CPUState*)esp;

  switch (cpu->eax) {
    case 4:
      printf((char*)cpu->ebx);
      break;

    default:
      break;
  }

  return esp;
}

/* DRACOS SYSTEM CALLS */
namespace os {

// sysprintf system call
void sysprintf(const char* str) {
  asm("int $0x80" : : "a"(4), "b"(str));
}

}  // namespace os
