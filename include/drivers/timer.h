#ifndef __OS__DRIVERS__TIMER_H
#define __OS__DRIVERS__TIMER_H

#include <common/types.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/port.h>
#include <utils/print.h>

namespace os {
  namespace drivers {

    class ProgrammableIntervalTimer : public hardwarecommunication::InterruptHandler {
    private:
      hardwarecommunication::Port8Bit dataPort;
      hardwarecommunication::Port8Bit commandPort;

    public:

      ProgrammableIntervalTimer(hardwarecommunication::InterruptManager* interrupts, common::uint32_t frequency);
      ~ProgrammableIntervalTimer();

      common::uint64_t ticks; // [tracks how many "ticks" have passed since boot]

      common::uint32_t HandleInterrupt(common::uint32_t esp) override;
      void Wait(common::uint32_t milliseconds);
    };

  }
}

#endif
