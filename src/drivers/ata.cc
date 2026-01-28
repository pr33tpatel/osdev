#include <drivers/ata.h>

using namespace os;
using namespace os::common;
using namespace os::drivers;

void printf(const char*);
void printfHex(uint8_t key);

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
  controlPort.Write(0); // clears HOB bit
  // controlPort.Write(0x02); // disable interrupts by setting nIEN

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
  
  // chekc for ATAPI before polling
  uint8_t lbaMid = lbaMidPort.Read();
  uint8_t lbaHi = lbaHiPort.Read();
  if (lbaMid != 0 || lbaHi != 0) {
    printf("ATAPI device detected\n");
  }

  // wait for device to get ready
  while(((status & 0x80) == 0x80) // flag for device is busy 
      && ((status & 0x01) != 0x01)) // flag for if there was an error
    status = commandPort.Read();


  if (status & 0x01){
    printf("ERROR in ATA IDENTIFICATION");
    return;
  }

  // printf("ATA Device Detected!\n");
}


void AdvancedTechnologyAttachment::Read28(common::uint32_t sector, common::uint8_t* data, int count){
  if(sector & 0xF0000000) 
    return;

  if(count > bytesPerSector)
    return;

  devicePort.Write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));
  for (int i = 0; i < 25; i++) {
    controlPort.Read();
  }
  errorPort.Write(0);
  sectorCountPort.Write(1);
  controlPort.Write(0x02);

  lbaLowPort.Write((sector & 0x000000FF));
  lbaMidPort.Write((sector & 0x0000FF00) >> 8);
  lbaHiPort.Write((sector & 0x00FF0000) >> 16);
  commandPort.Write(0x20);

  // Discard first 4 status reads
  for (int i = 0; i < 4; i++) {
    uint8_t discard_status = commandPort.Read();
  }

  uint8_t status = commandPort.Read();
  
  // Wait for BSY to clear
  while(((status & 0x80) == 0x80) && ((status & 0x01) != 0x01))
    status = commandPort.Read();

  if (status & 0x01){
    printf("ERROR in ATA READ (BSY)\n");
    printf("\nError Hex Code: 0x");
    uint8_t error = errorPort.Read();
    printfHex(error);
    printf("\n");
    return;
  }

  // Wait for DRQ to set
  while(!(status & 0x08) && !(status & 0x01)) 
    status = commandPort.Read();

  if (status & 0x01){
    printf("ERROR in ATA READ (DRQ)\n");
    return;
  }

  printf("Reading from ATA: ");
  
  for (uint16_t i = 0; i < count; i += 2) {
    uint16_t wdata = dataPort.Read();
    
    // **ADD DEBUG: Print raw wdata before storing**
    printf("\n  Raw wdata = 0x");
    printfHex((wdata >> 8) & 0xFF);
    printfHex(wdata & 0xFF);
    
    // Store in the data buffer parameter
    data[i] = wdata & 0xFF;
    if(i + 1 < count)
      data[i + 1] = (wdata >> 8) & 0xFF;
    
    // Print for debugging
    // printf(" -> data[%d]=0x", i);
    printf("-> data[");
    printfHex(i);
    printf("]=0x");
    printfHex(data[i]);
    if(i + 1 < count) {
      printf("-> data[");
      printfHex((i+1));
      printf("]=0x");
      printfHex(data[i+1]);
    }
  }

  printf("\n || DONE Reading.\n");

  // Read remaining sector data
  for (uint16_t i = count + (count % 2); i < bytesPerSector; i += 2) {
    dataPort.Read();
  }
  
  // 450ns delay
  for (int i = 0; i < 15; i++) {
    controlPort.Read();
  }
}


void AdvancedTechnologyAttachment::Write28(common::uint32_t sector, common::uint8_t* data, int count) {
  
  if(sector & 0xF0000000) 
    return;

  if(count > bytesPerSector)
    return;

  devicePort.Write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));

  for (int i = 0; i < 25; i++) {
    controlPort.Read();
  }
  errorPort.Write(0);
  sectorCountPort.Write(1);
  controlPort.Write(0x02);

  lbaLowPort.Write((sector & 0x000000FF));
  lbaMidPort.Write((sector & 0x0000FF00) >> 8);
  lbaHiPort.Write((sector & 0x00FF0000) >> 16);
  commandPort.Write(0x30);

  uint8_t status = commandPort.Read();
  
  // Wait for BSY to clear
  while(((status & 0x80) == 0x80) && ((status & 0x01) != 0x01))
    status = commandPort.Read();

  if (status & 0x01){
    printf("ERROR in ATA WRITE (BSY check)\n");
    return;
  }

  // Wait for DRQ to set
  while(!(status & 0x08) && !(status & 0x01)) 
    status = commandPort.Read();

  if (status & 0x01){
    printf("ERROR in ATA WRITE, DRQ not set.\n");
    return;
  }

  
printf("Writing to ATA: ");

for (uint16_t i = 0; i < count; i += 2) {
    uint16_t wdata = data[i];              // Get low byte
    if(i + 1 < count)
      wdata |= ((uint16_t)data[i+1]) << 8; // Add high byte

    // Debug: Print what we're about to write
    printf("\n  wdata = 0x");
    printfHex((wdata >> 8) & 0xFF);  // High byte
    printfHex(wdata & 0xFF);          // Low byte
    printf(" (");
    
    char text[3] = {0};
    text[0] = wdata & 0xFF;
    if (i + 1 < count)
      text[1] = (wdata >> 8) & 0xFF;
    else
      text[1] = '\0';
    printf(text);
    printf(")");
    
    dataPort.Write(wdata);  // Write the complete 16-bit word
}
// printf("\n || DONE WRITING.\n");
  for (uint16_t i = count + (count % 2); i < bytesPerSector; i += 2) {
    dataPort.Write(0x0000);
  }
// printf("Waiting for write completion...\n");
    status = commandPort.Read();
    while((status & 0x80) == 0x80)  // Wait for BSY to clear
        status = commandPort.Read();
    
    if (status & 0x01) {
        printf("ERROR after writing data\n");
        
        uint8_t error = errorPort.Read();
        printf("\nError Hex Code: 0x");
        printfHex(error);
        printf("\n");
        
        return;
    }
    printf("Write completed successfully\n");
  for (int i = 0; i < 15; i++) {
    controlPort.Read();
  }
}
void AdvancedTechnologyAttachment::Flush() {
  devicePort.Write( master ? 0xE0 : 0xF0 );
  controlPort.Write(0x02);
  commandPort.Write(0xE7);

  uint8_t status = commandPort.Read();
  if(status == 0x00){
    printf("[DEBUG FLUSH] status is 0x00, returning early");
    return;
  }

  while(((status & 0x80) == 0x80)
      && ((status & 0x01) != 0x01))
    status = commandPort.Read();

  if(status & 0x01)
  {
    printf("ERROR IN ATA FLUSH");
    uint8_t error = errorPort.Read();
    printf("\nError Hex Code: 0x");
    printfHex(error);
    printf("\n");
    return;
  }
}
