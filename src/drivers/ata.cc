#include <drivers/ata.h>

using namespace os;
using namespace os::common;
using namespace os::drivers;

void printf(const char*);


AdvancedTechnologyAttachment::AdvancedTechnologyAttachment(common::uint16_t portBase, bool master)
: dataPort(portBase),
  errorPort(portBase + 1),
  sectorCountPort(portBase + 2),
  lbaLowPort(portBase + 3),
  lbaMidPort(portBase + 4),
  lbaHiPort(portBase + 5),
  devicePort(portBase + 6),
  commandPort(portBase + 7),
  controlPort(portBase + 0x206)
{
  bytesPerSector = 512;
  this->master = master;
}
AdvancedTechnologyAttachment::~AdvancedTechnologyAttachment() {

}

void AdvancedTechnologyAttachment::Identify() {
  devicePort.Write(master ? 0xA0 : 0xB0);
  // controlPort.Write(0); // clears HOB bit
  controlPort.Write(0x02); // disable interrupts by setting nIEN

  devicePort.Write(0xA0); // read status of master
  uint8_t status = commandPort.Read();
  if (status == 0xFF) // 0xFF == 255, this means no device exists on this bus
    return;

  // at this point, there exists at least one device on the bus
  devicePort.Write(master ? 0xA0: 0xB0);
  sectorCountPort.Write(0);
  lbaLowPort.Write(0);
  lbaMidPort.Write(0);
  lbaHiPort.Write(0);
  commandPort.Write(0xEC);

  status = commandPort.Read();
  if (status == 0x00) 
    return; // no device


  // wait for device to get ready
  while(((status & 0x80) == 0x80) // flag for device is busy 
      && ((status & 0x01) != 0x01)) // flag for if there was an error
    status = commandPort.Read();


  if (status & 0x01){
    printf("ERROR in ATA IDENTIFICATION");
    return;
  }
}


void AdvancedTechnologyAttachment::Read28(common::uint32_t sector, common::uint8_t* data, int count){
  if(sector & 0xF0000000) 
    return;

  if(count > bytesPerSector)
    return;

  devicePort.Write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));
  errorPort.Write(0);
  sectorCountPort.Write(1);

  controlPort.Write(0x02); // polling mode so disable interrupts

  lbaLowPort.Write((sector & 0x000000FF));
  lbaMidPort.Write((sector & 0x0000FF00) >> 8);
  lbaHiPort.Write( (sector & 0x00FF0000) >> 16);
  commandPort.Write(0x20); // read mode


  uint8_t status = commandPort.Read();
  // wait for device to get ready
  while(((status & 0x80) == 0x80) // flag for device is busy 
      && ((status & 0x01) != 0x01)) // flag for if there was an error
    status = commandPort.Read();

  if (status & 0x01){
    printf("ERROR in ATA READ");
    return;
  }

  while(!(status & 0x08) && !(status & 0x01)) 
    status = commandPort.Read();

  if (status & 0x01){
    printf("ERROR in ATA READ, the DRQ is not set.\n");
    return;
  }

  printf("Reading from ATA: ");
  
  for (uint16_t i = 0; i < count; i+=2) {
    uint16_t wdata = dataPort.Read();

    char *text = "  \0";
    text[0] = wdata & 0xFF;

    if(i+1 < count)
      text[1] = (wdata >> 8) & 0xFF;
    else
      text[1] = '\0';

    printf(text);
  }
  printf(" || DONE Reading.\n");

  for (uint16_t i = count + (count % 2); i < bytesPerSector; i+=2) {
    dataPort.Read(); // if we read less than bytesPerSector, then read the rest of the sector (should be 0)
  }
  
  // TEST: 400ns wait 
  for (int i = 0; i < 4; i++) {
    controlPort.Read();
  }
}


void AdvancedTechnologyAttachment::Write28(common::uint32_t sector, common::uint8_t* data, int count) {
  
  if(sector & 0xF0000000) 
    return;

  if(count > bytesPerSector)
    return;

  devicePort.Write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));
  errorPort.Write(0);
  sectorCountPort.Write(1);

  controlPort.Write(0x02); // disable interrupts


  lbaLowPort.Write((sector & 0x000000FF));
  lbaMidPort.Write((sector & 0x0000FF00) >> 8);
  lbaHiPort.Write( (sector & 0x00FF0000) >> 16);
  commandPort.Write(0x30); // write mode

uint8_t status = commandPort.Read();
  
  // FIRST: Wait for BSY to clear
  while(((status & 0x80) == 0x80) && ((status & 0x01) != 0x01))
    status = commandPort.Read();

  if (status & 0x01){
    printf("ERROR in ATA WRITE (BSY check)\n");
    return;
  }

  // SECOND: Wait for DRQ to set
  while(!(status & 0x08) && !(status & 0x01)) 
    status = commandPort.Read();

  if (status & 0x01){
    printf("ERROR in ATA WRITE, DRQ not set.\n");
    return;
  }

  printf("Writing to ATA: ");
  
  for (uint16_t i = 0; i < count; i+=2) {
    uint16_t wdata = data[i];
    if(i+1 < count)
      wdata |= ((uint16_t)data[i+1]) << 8;
    char* foo = "  \0";
    foo[1] = (wdata >> 8) & 0x00FF; // print high byte
    foo[0] = (wdata) & 0x00FF; // print low byte
    printf(foo);

    dataPort.Write(wdata);
  }
  printf(" || DONE WRITING.\n");

  for (uint16_t i = count + (count % 2); i < bytesPerSector; i+=2) {
    dataPort.Write(0x0000); // if we write less than bytesPerSector, then fill rest with zeros
  }

  for (int i = 0; i < 4; i++) {
    controlPort.Read();
  }

}
void AdvancedTechnologyAttachment::Flush() {
  devicePort.Write( master ? 0xE0 : 0xF0 );
  controlPort.Write(0x02);
  commandPort.Write(0xE7);

  uint8_t status = commandPort.Read();
  if(status == 0x00)
    return;

  while(((status & 0x80) == 0x80)
      && ((status & 0x01) != 0x01))
    status = commandPort.Read();

  if(status & 0x01)
  {
    printf("ERROR IN ATA FLUSH");
    return;
  }
}
