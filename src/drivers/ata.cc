#include <drivers/ata.h>

using namespace os;
using namespace os::common;
using namespace os::drivers;
using namespace os::hardwarecommunication;


void printf(const char*);
<<<<<<< HEAD
void printfHex(uint8_t key);
=======
void printfHex(uint8_t);

>>>>>>> main

AdvancedTechnologyAttachment::AdvancedTechnologyAttachment(common::uint16_t portBase, bool master)
	
	: dataPort(portBase),
	  errorPort(portBase + 0x01),
	  sectorCountPort(portBase + 0x02),
	  lbaLowPort(portBase + 0x03),
	  lbaMidPort(portBase + 0x04),
	  lbaHiPort(portBase + 0x05),
	  devicePort(portBase + 0x06),
	  commandPort(portBase + 0x07),
	  controlPort(portBase + 0x206)

	{

	bytesPerSector = 512;
	this->master = master;
}



AdvancedTechnologyAttachment::~AdvancedTechnologyAttachment() {
<<<<<<< HEAD

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
=======
>>>>>>> main
}



bool AdvancedTechnologyAttachment::Identify() {

<<<<<<< HEAD
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
=======
	devicePort.Write(master ? 0xa0 : 0xb0);
	controlPort.Write(0);

	devicePort.Write(0xa0);
	uint8_t status = commandPort.Read();
	
	if (status == 0xff) { return false; }

	devicePort.Write(master ? 0xa0 : 0xb0);
	sectorCountPort.Write(0);
	lbaLowPort.Write(0);
	lbaMidPort.Write(0);
	lbaHiPort.Write(0);
	commandPort.Write(0xec);

	status = commandPort.Read();

	if (status == 0x00) {
	
		printf("NO DEVICE FOUND\n");
		return false;
	}

	//while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
	while (((status & 0x80) == 0x80) && ((status & 0x08) != 0x08)) {
	
		status = commandPort.Read();
	}

	if (status & 0x01) {
	
		printf("ATA ERROR\n");
		return false;
	}

	/*
	for (uint16_t i = 0; i < 256; i++) {
	
		
		uint16_t data = dataPort.Read();
		
		char* foo = "  ";
		foo[1] = (data >> 8) & 0x00ff;
		foo[0] = data & 0x00ff;
		printf(foo);
	}
	*/

	return true;
}
>>>>>>> main

  // Discard first 4 status reads
  for (int i = 0; i < 4; i++) {
    uint8_t discard_status = commandPort.Read();
  }

<<<<<<< HEAD
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
=======

void AdvancedTechnologyAttachment::Read28(common::uint32_t sector, common::uint8_t* data, int count, int offset) {	

	if ((sector & 0xf0000000) || count > bytesPerSector) {
	
		printf("STORAGE UNAVAILABLE\n");
		return;
	}

	devicePort.Write((master ? 0xe0 : 0xf0) | ((sector & 0x0f000000) >> 24));
	errorPort.Write(0x00);
	sectorCountPort.Write(0x01);

	lbaLowPort.Write( sector & 0x000000ff);
	lbaMidPort.Write((sector & 0x0000ff00) >> 8);
	lbaHiPort.Write(( sector & 0x00ff0000) >> 16);
/*	^
	|
	|

	I accidentally mixed up these ports troubleshooting these drivers (again)
	and it made for some really crazy file corruption in case you wanted 
	to see that :^)
*/

	commandPort.Write(0x20);
	

	uint8_t status = commandPort.Read();
	while (((status & 0x80) == 0x80) || ((status & 0x08) != 0x08)) {

		status = commandPort.Read();
	}




	if (status & 0x01) {
	
		printf("ATA ERROR\n");
		return;
	}


	if (offset) {
		for (uint16_t i = 0; i < offset; i += 2) {
	
			dataPort.Read();
		}
	}

	
  printf("Reading ATA:");
	for (uint16_t i = offset; i < count; i+= 2) {
	
		uint16_t rdata = dataPort.Read();
		data[i] = rdata & 0x00ff;
    char *text = "  \0";
    text[0] = (rdata >> 8) & 0xFF;
    text[1] = rdata & 0xFF;
    printf(text);

		if (i+1 < count) {
		
			data[i+1] = (rdata >> 8) & 0x00ff;
		}
	}

	for (uint16_t i = count + (count % 2); i < bytesPerSector; i += 2) {
	
		dataPort.Read();
	}
>>>>>>> main
}



void AdvancedTechnologyAttachment::Write28(common::uint32_t sector, common::uint8_t* data, int count, int offset) {
	
	if ((sector & 0xf0000000) || count > bytesPerSector) {
	
		printf("STORAGE UNAVAILABLE\n");
		return;
	}

	devicePort.Write((master ? 0xe0 : 0xf0) | ((sector & 0x0f000000) >> 24));
	errorPort.Write(0x00);
	sectorCountPort.Write(0x01);

	lbaLowPort.Write( sector & 0x000000ff);
	lbaMidPort.Write((sector & 0x0000ff00) >> 8);
	lbaHiPort.Write( (sector & 0x00ff0000) >> 16);
	
	commandPort.Write(0x30);
	
	
	uint8_t status = commandPort.Read();
	while (((status & 0x80) == 0x80) || ((status & 0x08) != 0x08)) {

		status = commandPort.Read();
	}
	
	
	if (offset) {

		uint8_t fillData[offset];
		this->Read28(sector, fillData, offset, 0);	
		
		for (uint16_t i = 0; i < offset; i += 2) {

			uint16_t wdata = fillData[i];

			if ((i + 1) < count) {
		
				wdata |= ((uint16_t)fillData[i+1]) << 8;
			}
			dataPort.Write(wdata);
		}
	}

  
  printf("Writing ATA: ");	
	for (uint16_t i = offset; i < count; i += 2) {
	
		uint16_t wdata = data[i];
    char *text = "  \0";
    text[0] = (wdata >> 8) & 0xFF;
    text[1] = wdata & 0xFF;
    printf(text);

<<<<<<< HEAD
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
=======
		if ((i + 1) < count) {
		
			wdata |= ((uint16_t)data[i+1]) << 8;
		}
		dataPort.Write(wdata);
	}
	
	
	for (uint16_t j = count + (count % 2); j < bytesPerSector; j += 2) {
	
		dataPort.Write(0x0000);
	}

	//very important
	this->Flush();
>>>>>>> main
}




void AdvancedTechnologyAttachment::Flush() {
	
	devicePort.Write(master ? 0xe0 : 0xf0);
	commandPort.Write(0xe7);

<<<<<<< HEAD
  uint8_t status = commandPort.Read();
  if(status == 0x00){
    printf("[DEBUG FLUSH] status is 0x00, returning early");
    return;
  }
=======
	uint8_t status = commandPort.Read();
>>>>>>> main

	if (status == 0x00) { return; }

<<<<<<< HEAD
  if(status & 0x01)
  {
    printf("ERROR IN ATA FLUSH");
    uint8_t error = errorPort.Read();
    printf("\nError Hex Code: 0x");
    printfHex(error);
    printf("\n");
    return;
  }
=======

	while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
	
		status = commandPort.Read();
	}

	if (status & 0x01) {
	
		printf("ATA ERROR");
		return;
	}
>>>>>>> main
}
