#include "common/types.h"
#include <drivers/amd_am79c973.h>

using namespace os;
using namespace os::common;
using namespace os::drivers;
using namespace os::hardwarecommunication;

void printf(const char*);


amd_am79c973::amd_am79c973(PeripheralComponentInterconnectDeviceDescriptor *dev, InterruptManager* interrupts) 
: Driver(),
  InterruptHandler(interrupts, dev->interrupt + interrupts->HardwareInterruptOffset()),
  MACAddress0Port(dev->portBase),
  MACAddress2Port(dev->portBase + 0x02),
  MACAddress4Port(dev->portBase + 0x04),
  registerDataPort(dev->portBase + 0x10),
  registerAddressPort(dev->portBase + 0x12),
  resetPort(dev->portBase + 0x14),
  busControlRegisterDataPort(dev->portBase + 0x16)
{ 

  currentSendBuffer = 0;
  currentRecvBuffer = 0;

  uint64_t MAC0 = MACAddress0Port.Read() % 256;
  uint64_t MAC1 = MACAddress0Port.Read() / 256;
  uint64_t MAC2 = MACAddress2Port.Read() % 256;
  uint64_t MAC3 = MACAddress2Port.Read() / 256;
  uint64_t MAC4 = MACAddress4Port.Read() % 256;
  uint64_t MAC5 = MACAddress4Port.Read() / 256;

  uint64_t MAC = MAC5 << 5*8
               | MAC4 << 4*8 
               | MAC3 << 3*8 
               | MAC2 << 2*8 
               | MAC1 << 1*8 
               | MAC0 ;
    

  // set the device to 32 bit mode
  registerAddressPort.Write(20);
  busControlRegisterDataPort.Write(0x102);

  // STOP reset
  registerAddressPort.Write(0);
  registerDataPort.Write(0x04);

  // Initialization Block 
  initBlock.mode = 0x0000; // promiscuous mode = false
  initBlock.reserved1 = 0;
  initBlock.numSendBuffers = 3;
  initBlock.reserved2 = 0;
  initBlock.numRecvBuffers = 3;
  initBlock.physicalAddress = MAC;
  initBlock.reserved3 = 0;
  initBlock.logicalAddress = 0;

  /* DIAGRAM:
                   (uint32_t) 0xF := 0x000000FF => (00000000 00000000 00000000 00001111)
                  ~(uint32_t) 0xF := 0x000000FF => (11111111 11111111 11111111 11110000)
      compared to (uint32_t) 0x0 := 0x00000000 => (00000000 00000000 00000000 00000000)
    therefore, ~((uint32_t) 0xF) != ((uint32_t) 0x0) */
  sendBufferDescrPtr = (BufferDescriptor*)((    ((uint32_t) &sendBufferDescMemory[0]) + 15) // memory address of the sendBufferDescr + 15 bytes, the offset is for the 16-bit device alignment
                                             & ~((uint32_t) 0xF)); // NOTE: ~((uint32_t) 0xFF) != ((uint32_t) 0x00), see DIAGRAM
  initBlock.sendBufferDescrAddress = (uint32_t)sendBufferDescrPtr;

  recvBufferDescrPtr = (BufferDescriptor*)((    ((uint32_t) &recvBufferDescMemory[0]) + 15) // memory address of the sendBufferDescr + 15 bytes, the offset is for the 16-bit device alignment
                                             & ~((uint32_t) 0xF)); // NOTE: ~((uint32_t) 0xFF) != ((uint32_t) 0x00)
  initBlock.recvBufferDescrAddress = (uint32_t)recvBufferDescrPtr;

  for (uint8_t i = 0; i < 8; i++) {
    sendBufferDescrPtr[i].address = (((uint32_t) &sendBufferDescMemory[0]) + 15) & ~((uint32_t)0xF);
    sendBufferDescrPtr[i].flags   = 0x7FF
                                  | 0xF000; // send the length of the BufferDescriptorPtr
    sendBufferDescrPtr[i].flags2  = 0;
    sendBufferDescrPtr[i].avail   = 0;

    recvBufferDescrPtr[i].address = (((uint32_t) &recvBufferDescMemory[0]) + 15) & ~((uint32_t)0xF);
    recvBufferDescrPtr[i].flags   = 0xF7FF
                                  | 0x80000000; // send the length of the BufferDescriptorPtr
    recvBufferDescrPtr[i].flags2  = 0;
    recvBufferDescrPtr[i].avail   = 0;
  }

  registerAddressPort.Write(1);
  registerDataPort.Write( (uint32_t)(&initBlock) & 0xFFFF);
  registerAddressPort.Write(2);
  registerDataPort.Write( ((uint32_t)(&initBlock) >> 16) & 0xFFFF);

}


amd_am79c973::~amd_am79c973() {
}
 

void amd_am79c973::Activate() {
  
  registerAddressPort.Write(0);
  registerDataPort.Write(0x41); 

  registerAddressPort.Write(4);
  uint32_t temp = registerDataPort.Read();
  registerAddressPort.Write(4);
  registerDataPort.Write(temp | 0xC00); 

  registerAddressPort.Write(0);
  registerDataPort.Write(0x42); 

}


int amd_am79c973::Reset() {
  resetPort.Read();
  resetPort.Write(0);
  return 10; // NOTE: wait for 10 ms
}


uint32_t amd_am79c973::HandleInterrupt(uint32_t esp) {
  printf("INTERRUPT FROM AMD am79c973\n");

  registerAddressPort.Write(0);
  uint32_t temp = registerDataPort.Read();

  if((temp & 0x8000) == 0x8000) printf("AMD am79c973 GENERAL ERROR\n");
  if((temp & 0x2000) == 0x2000) printf("AMD am79c973 COLLISION ERROR\n");
  if((temp & 0x1000) == 0x1000) printf("AMD am79c973 MISSED FRAME ERROR (too much data)\n");
  if((temp & 0x0800) == 0x0800) printf("AMD am79c973 MEMORY ERROR\n");
  if((temp & 0x0400) == 0x0400) printf("AMD am79c973 DATA RECIEVED !\n");
  if((temp & 0x0200) == 0x0200) printf("AMD am79c973 DATA SENT !\n");

  // acknowledge
  registerAddressPort.Write(0);
  registerDataPort.Write(temp);

  if((temp & 0x0100) == 0x0100) printf("AMD am79c973 INIT DONE\n");

  return esp;

}
