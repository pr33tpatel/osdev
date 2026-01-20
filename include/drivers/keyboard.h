
#ifndef __OS__DRIVERS__KEYBOARD_H
#define __OS__DRIVERS__KEYBOARD_H

#include <drivers/driver.h>
#include <common/types.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/port.h>

namespace os {
  namespace drivers {


    class KeyboardEventHandler{
      public:
        KeyboardEventHandler();

        virtual void OnKeyDown(char);
        virtual void OnKeyUp(char);
    };

    class KeyboardDriver : public os::hardwarecommunication::InterruptHandler, public Driver
    {
      os::hardwarecommunication::Port8Bit dataport;
      os::hardwarecommunication::Port8Bit commandport;

      KeyboardEventHandler* handler;
      public:
      KeyboardDriver(os::hardwarecommunication::InterruptManager* manager, KeyboardEventHandler *handler);
      ~KeyboardDriver();
      virtual os::common::uint32_t HandleInterrupt(os::common::uint32_t esp);
      virtual void Activate();

    };
  }
}

#endif
