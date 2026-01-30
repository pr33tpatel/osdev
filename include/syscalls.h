#ifndef __OS__SYSCALLS_H 
#define __OS__SYSCALLS_H

#include <common/types.h>
#include <hardwarecommunication/interrupts.h>
#include <multitasking.h>

namespace os {
  class SyscallHandler : public hardwarecommunication::InterruptHandler {

    public:
      SyscallHandler(hardwarecommunication::InterruptManager* interruptManager, os::common::uint8_t InterruptNumber);
      ~SyscallHandler();

      virtual os::common::uint32_t HandleInterrupt(os::common::uint32_t esp);
  };

  /* DRACOS SYSTEM CALLS */
  void sysprintf(const char* str);
}

#endif
