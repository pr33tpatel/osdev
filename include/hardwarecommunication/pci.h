#ifndef __OS__HARDWARECOMMUNICATION__PCI_H
#define __OS__HARDWARECOMMUNICATION__PCI_H

#include <common/types.h>
#include <drivers/driver.h>
#include <hardwarecommunication/port.h>
#include <hardwarecommunication/interrupts.h>
#include <memorymanagement.h>

namespace os {
  namespace hardwarecommunication {

    enum BaseAddressRegisterType {
      MemoryMapping = 0,
      InputOutput = 1
    };


    class BaseAddressRegister {
      
      public:
        bool prefetchable;
        os::common::uint8_t* address;
        os::common::uint32_t size;
        BaseAddressRegisterType type;
    };


    class PeripheralComponentInterconnectDeviceDescriptor {
      public:
        os::common::uint32_t portBase;
        os::common::uint32_t interrupt;

        os::common::uint16_t bus ;
        os::common::uint16_t device;
        os::common::uint16_t function;

        os::common::uint16_t vendor_id;
        os::common::uint16_t device_id;

        os::common::uint8_t class_id;
        os::common::uint8_t subclass_id;
        os::common::uint8_t interface_id;

        os::common::uint16_t revision;
        
        PeripheralComponentInterconnectDeviceDescriptor();
        ~PeripheralComponentInterconnectDeviceDescriptor();

    };


    class PeripheralComponentInterconnectController {

      Port32Bit dataPort;
      Port32Bit commandPort;

      public:
        PeripheralComponentInterconnectController();
        ~PeripheralComponentInterconnectController();

        os::common::uint32_t Read(os::common::uint16_t bus, os::common::uint16_t device, os::common::uint16_t function, os::common::uint32_t registeroffest);
        void Write(os::common::uint16_t bus, os::common::uint16_t device, os::common::uint16_t function, os::common::uint32_t registeroffest, os::common::uint32_t value);
        bool DeviceHasFunctions(os::common::uint16_t bus, os::common::uint16_t device);

        void SelectDrivers(os::drivers::DriverManager* driverManager, os::hardwarecommunication::InterruptManager* interrupts);
        os::drivers::Driver* GetDriver(PeripheralComponentInterconnectDeviceDescriptor dev, os::hardwarecommunication::InterruptManager* interrupts);
        PeripheralComponentInterconnectDeviceDescriptor GetDeviceDescriptor(os::common::uint16_t bus, os::common::uint16_t device, os::common::uint16_t function);
        BaseAddressRegister GetBaseAddressRegister(os::common::uint16_t bus, os::common::uint16_t device, os::common::uint16_t function, os::common::uint16_t bar); // bar stands for Base Address Register

        void EnableBusMastering(uint16_t bus, uint16_t device, uint16_t function);
        void EnableBusMastering(PeripheralComponentInterconnectDeviceDescriptor *dev);
    };
  }

};

#endif
