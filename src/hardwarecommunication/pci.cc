#include "common/types.h"
#include <hardwarecommunication/pci.h>
using namespace os::common;
using namespace os::drivers;
using namespace os::hardwarecommunication;


void printf(const char* str);
void printfHex(uint8_t);



PeripheralComponentInterconnectDeviceDescriptor::PeripheralComponentInterconnectDeviceDescriptor(){

}
PeripheralComponentInterconnectDeviceDescriptor::~PeripheralComponentInterconnectDeviceDescriptor(){

}


// constructor
PeripheralComponentInterconnectController::PeripheralComponentInterconnectController()
: dataPort(0xCFC),
  commandPort(0xCF8)
{
  
}


// destructor
PeripheralComponentInterconnectController::~PeripheralComponentInterconnectController() {
}

uint32_t PeripheralComponentInterconnectController::Read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffest) {
  uint32_t id = 
    0x1 << 31
    | ((bus & 0xFF) << 16)
    | ((device & 0x1F) << 11)
    | ((function & 0x07) << 8)
    | (registeroffest & 0xFC);

  commandPort.Write(id);
  uint32_t result = dataPort.Read();
  return result >> (8* (registeroffest % 4));

}
void PeripheralComponentInterconnectController::Write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffest, uint32_t value) {
  uint32_t id = 
    0x1 << 31
    | ((bus & 0xFF) << 16)
    | ((device & 0x1F) << 11)
    | ((function & 0x07) << 8)
    | (registeroffest & 0xFC);

  commandPort.Write(id);
  dataPort.Write(value);
}

bool PeripheralComponentInterconnectController::DeviceHasFunctions(uint16_t bus, uint16_t device){
  return Read(bus, device, 0, 0x0E) & (1<<7); // only need the 7th bit of this address
}

void PeripheralComponentInterconnectController::SelectDrivers(DriverManager* driverManager, InterruptManager* interrupts) {
  for(int bus = 0; bus < 8; bus++) {
    for (int device = 0; device < 32; device++) {
      int numFunctions = DeviceHasFunctions(bus, device) ? 8 : 1; // if device has functions, then 8 functions, if "no" functions, then at least 1 function is exists
      for (int function = 0; function < numFunctions; function++) {
        PeripheralComponentInterconnectDeviceDescriptor dev = GetDeviceDescriptor(bus, device, function);

        if (dev.vendor_id == 0x0000 || dev.vendor_id == 0xFFFF)
          continue;


        for (int barNum = 0; barNum < 6; barNum++){
          BaseAddressRegister bar = GetBaseAddressRegister(bus, device, function, barNum);
          if(bar.address && (bar.type == InputOutput)) 
            dev.portBase = (uint32_t)bar.address;

          Driver* driver = GetDriver(dev, interrupts);
          if(driver != 0) {
            driverManager->AddDriver(driver);
          }
          
        }



      
        // same output as linux command: "lspci" : list of PCI devcies
        printf("PCI BUS ");
        printfHex(bus & 0xFF);

        printf(", DEVICE ");
        printfHex(device & 0xFF);

        printf(", FUNCTION ");
        printfHex(function & 0xFF);

        printf(" = VENDOR ");
        printfHex((dev.vendor_id & 0xFF00) >> 8);
        printfHex(dev.vendor_id & 0xFF);
        printf(", DEVICE ");
        printfHex((dev.device_id & 0xFF00) >> 8);
        printfHex(dev.device_id & 0xFF);
        printf("\n");
      }
    }
  }
}

BaseAddressRegister PeripheralComponentInterconnectController::GetBaseAddressRegister(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar) {
  BaseAddressRegister result;
  
  uint32_t headertype = Read(bus, device, function, 0x0E) & 0x7F;
  int maxBARs = 6 - (4*headertype);
  if(bar >= maxBARs)
    return result; // return is not set to anything so address is NULL at this point


  uint32_t bar_value = Read(bus, device, function, 0x10 + 4*bar);
  result.type = (bar_value & 0x1) ? InputOutput : MemoryMapping; // if 1 then I/O, else MemoryMapping
  uint32_t temp;

  if (result.type == MemoryMapping) {
    switch((bar_value >> 1) & 0x3) {
      case 0: // 32 bit Mode
      case 1: // 20 bit Mode
      case 2: // 64 bit Mode
        break;
    }

  } else { // InputOutput case
    result.address = (uint8_t*)(bar_value & ~0x3); // set to the bar value but cancel the last two bits
    result.prefetchable = false;
  }


  return result;
}

Driver* PeripheralComponentInterconnectController::GetDriver(PeripheralComponentInterconnectDeviceDescriptor dev, InterruptManager* interrupts) {
  Driver *driver = 0;
  switch(dev.vendor_id) {
    case 0x1022: // AMD
      switch(dev.device_id) {
        case 0x2000: // am79c973
          printf("AMD am79c973\n");
          break;
      }    
      break;
    
    case 0x8086: // Intel
        switch(dev.device_id) {
          case 0x7000:
            // driver = (intel_piix3*)MemoryManager::activeMemoryManager->malloc(sizeof(intel_piix3));
            // if(driver != 0)
            //   new (driver) intel_piix3(...);
            // printf("Intel Network Card: 82371SB PIIX3 ISA");
            break;
        }
      break;

  }

  switch(dev.class_id) {
    case 0x03: // graphics
      switch(dev.subclass_id) {
        case 0x00: // VGA graphics
          printf("VGA "); 
          break;
      }
      break;

  }


  return 0;
}

PeripheralComponentInterconnectDeviceDescriptor PeripheralComponentInterconnectController::GetDeviceDescriptor(uint16_t bus, uint16_t device, uint16_t function) {
PeripheralComponentInterconnectDeviceDescriptor result;

result.bus = bus;
  result.device = device;
  result.function =  function;

  result.vendor_id = Read(bus, device, function, 0x00);
  result.device_id = Read(bus, device, function, 0x02);

  
  result.class_id = Read(bus, device, function, 0x0b);
  result.subclass_id = Read(bus, device, function, 0x0a);
  result.interface_id = Read(bus, device, function, 0x09);

  result.revision  = Read(bus, device, function, 0x08);
  result.interrupt = Read(bus, device, function, 0x3c);
  return result;

}


