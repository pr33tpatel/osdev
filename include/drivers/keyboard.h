
#ifndef __OS__DRIVERS__KEYBOARD_H
#define __OS__DRIVERS__KEYBOARD_H

#include <common/types.h>
#include <drivers/driver.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/port.h>
#include <utils/print.h>

#define ARROW_UP 0x91
#define ARROW_RIGHT 0x92
#define ARROW_DOWN 0x93
#define ARROW_LEFT 0x94

#define SHIFT_ARROW_UP 0x95
#define SHIFT_ARROW_RIGHT 0x96
#define SHIFT_ARROW_DOWN 0x97
#define SHIFT_ARROW_LEFT 0x98

namespace os {
namespace drivers {


class KeyboardEventHandler {
 public:
  KeyboardEventHandler();

  virtual void OnKeyDown(char);
  virtual void OnKeyUp(char);
};

class KeyboardDriver : public os::hardwarecommunication::InterruptHandler, public Driver {
  os::hardwarecommunication::Port8Bit dataport;
  os::hardwarecommunication::Port8Bit commandport;

  KeyboardEventHandler* handler;

  bool Shift;

 public:
  KeyboardDriver(os::hardwarecommunication::InterruptManager* manager, KeyboardEventHandler* handler);
  ~KeyboardDriver();
  virtual os::common::uint32_t HandleInterrupt(os::common::uint32_t esp);
  virtual void Activate();
};
}  // namespace drivers
}  // namespace os

#endif
