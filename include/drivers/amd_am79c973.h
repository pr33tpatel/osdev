#ifndef __OS__DRIVERS__AMD_AM79C973_H
#define __OS__DRIVERS__AMD_AM79C973_H

#include <common/types.h>
#include <drivers/driver.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/pci.h>
#include <hardwarecommunication/port.h>

namespace os {
  namespace drivers {

    class amd_am79c973 : public Driver, public hardwarecommunication::InterruptHandler {

      struct InitializationBlock {
        common::uint16_t mode;
        unsigned reserved1 : 4;
        unsigned numSendBuffers : 4;
        unsigned reserved2 : 4;
        unsigned numRecvBuffers : 4;
        common::uint64_t physicalAddress : 48; // NOTE: MAC address is 48 bits but is treated like a 64-bit number later (?)
        common::uint16_t reserved3;
        common::uint64_t logicalAddress; // NOTE: for IP address i think
        common::uint32_t recvBufferDescrAddress;
        common::uint32_t sendBufferDescrAddress;
      } __attribute__((packed));

      struct BufferDescriptor {
        common::uint32_t address;
        common::uint32_t flags;
        common::uint32_t flags2;
        common::uint32_t avail;
      } __attribute__((packed));

      hardwarecommunication::Port16Bit MACAddress0Port;
      hardwarecommunication::Port16Bit MACAddress2Port;
      hardwarecommunication::Port16Bit MACAddress4Port;

      hardwarecommunication::Port16Bit registerDataPort;
      hardwarecommunication::Port16Bit registerAddressPort;

      hardwarecommunication::Port16Bit resetPort;
      hardwarecommunication::Port16Bit busControlRegisterDataPort;

      InitializationBlock initBlock;

      // NOTE: +15 because the Buffers need to be 16-bit aligned, weird device hardware stuff
      BufferDescriptor* sendBufferDescrPtr;
      common::uint8_t sendBufferDescMemory[2*1024+15][8]; 
      common::uint8_t sendBuffers[2*1024+15][8]; 
      common::uint8_t currentSendBuffer;

      BufferDescriptor* recvBufferDescrPtr;
      common::uint8_t recvBufferDescMemory[2*1024+15][8]; 
      common::uint8_t recvBuffers[2*1024+15][8]; 
      common::uint8_t currentRecvBuffer;



    public:
      amd_am79c973(hardwarecommunication::PeripheralComponentInterconnectDeviceDescriptor *dev, hardwarecommunication::InterruptManager* interrupts);
      ~amd_am79c973();

      virtual void Activate();
      virtual int Reset();
      virtual common::uint32_t HandleInterrupt(common::uint32_t esp);
    };
  }
}

#endif
