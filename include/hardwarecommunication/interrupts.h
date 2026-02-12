#ifndef __OS__HARDWARECOMMUNICATION__INTERRUPTMANAGER_H
#define __OS__HARDWARECOMMUNICATION__INTERRUPTMANAGER_H


#include <common/types.h>
#include <gdt.h>
#include <hardwarecommunication/port.h>
#include <multitasking.h>
#include <utils/print.h>


namespace os {
namespace hardwarecommunication {

class InterruptManager;

class InterruptHandler {
 protected:
  InterruptHandler(InterruptManager* interruptManager, os::common::uint8_t InterruptNumber);
  ~InterruptHandler();

  os::common::uint8_t InterruptNumber;
  InterruptManager* interruptManager;

 public:
  virtual os::common::uint32_t HandleInterrupt(os::common::uint32_t esp);
};


class InterruptManager {
  friend class InterruptHandler;

 protected:
  static InterruptManager* ActiveInterruptManager;
  InterruptHandler* handlers[256];
  TaskManager* taskManager;

  struct GateDescriptor {
    os::common::uint16_t handlerAddressLowBits;
    os::common::uint16_t gdt_codeSegmentSelector;
    os::common::uint8_t reserved;
    os::common::uint8_t access;
    os::common::uint16_t handlerAddressHighBits;
  } __attribute__((packed));

  static GateDescriptor interruptDescriptorTable[256];

  struct InterruptDescriptorTablePointer {
    os::common::uint16_t size;
    os::common::uint32_t base;
  } __attribute__((packed));

  os::common::uint16_t hardwareInterruptOffset;
  static void SetInterruptDescriptorTableEntry(
      os::common::uint8_t interrupt,
      os::common::uint16_t codeSegmentSelectorOffset,
      void (*handler)(),
      os::common::uint8_t DescriptorPrivilegeLevel,
      os::common::uint8_t DescriptorType
  );


  static void InterruptIgnore();

  static void HandleInterruptRequest0x00();
  static void HandleInterruptRequest0x01();
  static void HandleInterruptRequest0x02();
  static void HandleInterruptRequest0x03();
  static void HandleInterruptRequest0x04();
  static void HandleInterruptRequest0x05();
  static void HandleInterruptRequest0x06();
  static void HandleInterruptRequest0x07();
  static void HandleInterruptRequest0x08();
  static void HandleInterruptRequest0x09();
  static void HandleInterruptRequest0x0A();
  static void HandleInterruptRequest0x0B();
  static void HandleInterruptRequest0x0C();
  static void HandleInterruptRequest0x0D();
  static void HandleInterruptRequest0x0E();
  static void HandleInterruptRequest0x0F();
  static void HandleInterruptRequest0x31();

  static void HandleInterruptRequest0x80();

  static void HandleException0x00();
  static void HandleException0x01();
  static void HandleException0x02();
  static void HandleException0x03();
  static void HandleException0x04();
  static void HandleException0x05();
  static void HandleException0x06();
  static void HandleException0x07();
  static void HandleException0x08();
  static void HandleException0x09();
  static void HandleException0x0A();
  static void HandleException0x0B();
  static void HandleException0x0C();
  static void HandleException0x0D();
  static void HandleException0x0E();
  static void HandleException0x0F();
  static void HandleException0x10();
  static void HandleException0x11();
  static void HandleException0x12();
  static void HandleException0x13();


  static os::common::uint32_t HandleInterrupt(os::common::uint8_t interrupt, os::common::uint32_t esp);
  os::common::uint32_t DoHandleInterrupt(os::common::uint8_t interrupt, os::common::uint32_t esp);

  Port8BitSlow programmableInterruptControllerMasterCommandPort;
  Port8BitSlow programmableInterruptControllerMasterDataPort;
  Port8BitSlow programmableInterruptControllerSlaveCommandPort;
  Port8BitSlow programmableInterruptControllerSlaveDataPort;

 public:
  InterruptManager(
      os::common::uint16_t hardwareInterruptOffset,
      os::GlobalDescriptorTable* globalDescriptorTable,
      os::TaskManager* taskManager
  );
  ~InterruptManager();
  os::common::uint16_t HardwareInterruptOffset();
  void Activate();
  void Deactivate();
};
}  // namespace hardwarecommunication
}  // namespace os
#endif
