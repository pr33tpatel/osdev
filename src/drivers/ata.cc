#include <drivers/ata.h>

using namespace os;
using namespace os::common;
using namespace os::drivers;
using namespace os::hardwarecommunication;

void printf(const char*);
void printfHex(uint8_t);

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
}

bool AdvancedTechnologyAttachment::Identify() {
	devicePort.Write(master ? 0xA0 : 0xB0);
	controlPort.Write(0);

	devicePort.Write(0xA0);
	uint8_t status = commandPort.Read();
	
	if (status == 0xFF) { 
		return false; 
	}

	devicePort.Write(master ? 0xA0 : 0xB0);
	sectorCountPort.Write(0);
	lbaLowPort.Write(0);
	lbaMidPort.Write(0);
	lbaHiPort.Write(0);
	commandPort.Write(0xEC);

	status = commandPort.Read();

	if (status == 0x00) {
		printf("NO DEVICE FOUND\n");
		return false;
	}

	// Check for ATAPI before polling
	uint8_t lbaMid = lbaMidPort.Read();
	uint8_t lbaHi = lbaHiPort.Read();
	if (lbaMid != 0 || lbaHi != 0) {
		printf("ATAPI device detected\n");
		return false;
	}

	// Wait for BSY to clear and DRQ to set
	while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
		status = commandPort.Read();
	}

	if (status & 0x01) {
		printf("ATA ERROR IN IDENTIFY\n");
		return false;
	}

	// Read identification data (256 words)
	for (uint16_t i = 0; i < 256; i++) {
		dataPort.Read();
	}

	return true;
}

void AdvancedTechnologyAttachment::Read28(common::uint32_t sector, common::uint8_t* data, int count) {
	if (sector & 0xF0000000) {
		printf("INVALID SECTOR\n");
		return;
	}

	if (count > bytesPerSector) {
		printf("COUNT TOO LARGE\n");
		return;
	}

	// Select device and set LBA bits 27-24
	devicePort.Write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));
	
	// 400ns delay after device selection
	for (int i = 0; i < 15; i++) {
		controlPort.Read();
	}

	errorPort.Write(0);
	sectorCountPort.Write(1);
	controlPort.Write(0x02); // Disable interrupts

	lbaLowPort.Write((sector & 0x000000FF));
	lbaMidPort.Write((sector & 0x0000FF00) >> 8);
	lbaHiPort.Write((sector & 0x00FF0000) >> 16);
	commandPort.Write(0x20); // Read sectors command

	uint8_t status = commandPort.Read();
	
	// Wait for BSY to clear
	while(((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
		status = commandPort.Read();
	}

	if (status & 0x01) {
		printf("ERROR in ATA READ (BSY)\n");
		uint8_t error = errorPort.Read();
		printf("Error code: 0x");
		printfHex(error);
		printf("\n");
		return;
	}

	// Wait for DRQ to set
	while(!(status & 0x08) && !(status & 0x01)) {
		status = commandPort.Read();
	}

	if (status & 0x01) {
		printf("ERROR in ATA READ (DRQ)\n");
		return;
	}

	printf("Reading ATA: ");
	
	// Read data
	for (uint16_t i = 0; i < count; i += 2) {
		uint16_t wdata = dataPort.Read();
		
		// Store in buffer
		data[i] = wdata & 0xFF;
		if(i + 1 < count) {
			data[i + 1] = (wdata >> 8) & 0xFF;
		}
		
		// Print for debugging
		char text[3];
		text[0] = data[i];
		text[1] = (i + 1 < count) ? data[i + 1] : '\0';
		text[2] = '\0';
		printf(text);
	}

	printf(" || DONE\n");

	// Read remaining sector data
	for (uint16_t i = count + (count % 2); i < bytesPerSector; i += 2) {
		dataPort.Read();
	}
	
	// 400ns delay
	for (int i = 0; i < 15; i++) {
		controlPort.Read();
	}
}

void AdvancedTechnologyAttachment::Write28(common::uint32_t sector, common::uint8_t* data, int count) {
	if (sector & 0xF0000000) {
		printf("INVALID SECTOR\n");
		return;
	}

	if (count > bytesPerSector) {
		printf("COUNT TOO LARGE\n");
		return;
	}

	// Select device and set LBA bits 27-24
	devicePort.Write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));
	
	// 400ns delay after device selection
	for (int i = 0; i < 15; i++) {
		controlPort.Read();
	}

	errorPort.Write(0);
	sectorCountPort.Write(1);
	controlPort.Write(0x02); // Disable interrupts

	lbaLowPort.Write((sector & 0x000000FF));
	lbaMidPort.Write((sector & 0x0000FF00) >> 8);
	lbaHiPort.Write((sector & 0x00FF0000) >> 16);
	commandPort.Write(0x30); // Write sectors command

	uint8_t status = commandPort.Read();
	
	// Wait for BSY to clear
	while(((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
		status = commandPort.Read();
	}

	if (status & 0x01) {
		printf("ERROR in ATA WRITE (BSY)\n");
		return;
	}

	// Wait for DRQ to set
	while(!(status & 0x08) && !(status & 0x01)) {
		status = commandPort.Read();
	}

	if (status & 0x01) {
		printf("ERROR in ATA WRITE (DRQ)\n");
		return;
	}

	printf("Writing ATA: ");
	
	// Write data
	for (uint16_t i = 0; i < count; i += 2) {
		uint16_t wdata = data[i];
		if(i + 1 < count) {
			wdata |= ((uint16_t)data[i+1]) << 8;
		}
		
		// Print for debugging
		char text[3];
		text[0] = wdata & 0xFF;
		text[1] = (wdata >> 8) & 0xFF;
		text[2] = '\0';
		printf(text);
		
		dataPort.Write(wdata);
	}

	printf(" || DONE\n");

	// Fill rest of sector with zeros
	for (uint16_t i = count + (count % 2); i < bytesPerSector; i += 2) {
		dataPort.Write(0x0000);
	}

	// Wait for write completion
	status = commandPort.Read();
	while((status & 0x80) == 0x80) {
		status = commandPort.Read();
	}
	
	if (status & 0x01) {
		printf("ERROR after writing data\n");
		uint8_t error = errorPort.Read();
		printf("Error code: 0x");
		printfHex(error);
		printf("\n");
		return;
	}

	// 400ns delay
	for (int i = 0; i < 15; i++) {
		controlPort.Read();
	}
}

void AdvancedTechnologyAttachment::Flush() {
	devicePort.Write(master ? 0xE0 : 0xF0);
	commandPort.Write(0xE7); // Flush cache command

	uint8_t status = commandPort.Read();

	if (status == 0x00) { 
		return; 
	}

	// Wait for flush to complete
	while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
		status = commandPort.Read();
	}

	if (status & 0x01) {
		printf("ERROR IN ATA FLUSH\n");
		uint8_t error = errorPort.Read();
		printf("Error code: 0x");
		printfHex(error);
		printf("\n");
		return;
	}
}
