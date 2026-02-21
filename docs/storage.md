# Storage (ATA Driver)

## Responsibility

- Provide low-level access to ATA (IDE) disks in 28-bit LBA mode.
- Support basic operations:
  - Device identification (`Identify`).
  - Sector reads (`Read28`).
  - Sector writes (`Write28`).
  - Cache flush (`Flush`).
- Intended to be used as the hardware backend for future filesystem layers.

---

## Hardware Model

`AdvancedTechnologyAttachment` represents a single ATA channel endpoint (primary/secondary, master/slave).

### I/O Ports

Constructed with a base port and `master` flag:

```cpp
AdvancedTechnologyAttachment::AdvancedTechnologyAttachment(uint16_t portBase, bool master)
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
```

- Standard ATA register layout:
  - `dataPort` – 16‑bit data register.
  - `errorPort` – error/status details.
  - `sectorCountPort` – sector count for transfer.
  - `lbaLowPort`, `lbaMidPort`, `lbaHiPort` – LBA address bytes.
  - `devicePort` – device/head, plus upper LBA bits and master/slave select.
  - `commandPort` – command/status.
  - `controlPort` – device control (e.g., interrupts, 400 ns delays).

- Default:
  - `bytesPerSector = 512`.
  - `master` indicates whether this object is pointing at the master or slave device on the channel.

---

## Device Identification

```cpp
bool AdvancedTechnologyAttachment::Identify();
```

### Process

1. Select device:

   ```cpp
   devicePort.Write(master ? 0xA0 : 0xB0);
   controlPort.Write(0);
   ```

2. Basic presence check:

   ```cpp
   devicePort.Write(0xA0);
   uint8_t status = commandPort.Read();
   if (status == 0xFF)
     return false; // no device attached
   ```

3. Prepare IDENTIFY:

   ```cpp
   devicePort.Write(master ? 0xA0 : 0xB0);
   sectorCountPort.Write(0);
   lbaLowPort.Write(0);
   lbaMidPort.Write(0);
   lbaHiPort.Write(0);
   commandPort.Write(0xEC); // IDENTIFY
   ```

4. Check status:

   ```cpp
   status = commandPort.Read();
   if (status == 0x00) {
     printf("NO DEVICE FOUND\n");
     return false;
   }
   ```

5. Detect ATAPI:

   ```cpp
   uint8_t lbaMid = lbaMidPort.Read();
   uint8_t lbaHi  = lbaHiPort.Read();
   if (lbaMid != 0 || lbaHi != 0) {
     printf("ATAPI device detected\n");
     return false;
   }
   ```

6. Poll until not busy (BSY cleared) and no error:

   ```cpp
   while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
     status = commandPort.Read();
   }
   if (status & 0x01) {
     printf("ATA ERROR IN IDENTIFY\n");
     return false;
   }
   ```

7. Read and discard 256 words of identification data:

   ```cpp
   for (uint16_t i = 0; i < 256; i++) {
     dataPort.Read();
   }
   ```

- Returns `true` if a valid ATA device is found and successfully identified, `false` otherwise.

---

## Reading Sectors (LBA28)

```cpp
void AdvancedTechnologyAttachment::Read28(uint32_t sector, uint8_t* data, int count);
```

### Preconditions and validation

- Sector must fit in 28 bits:

  ```cpp
  if (sector & 0xF0000000) {
    printf("INVALID SECTOR\n");
    return;
  }
  ```

- `count` must not exceed one sector:

  ```cpp
  if (count > bytesPerSector) {
    printf("COUNT TOO LARGE\n");
    return;
  }
  ```

### Command setup

1. Select device and upper LBA bits:

   ```cpp
   devicePort.Write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));
   ```

2. 400 ns delay (roughly):

   ```cpp
   for (int i = 0; i < 15; i++)
     controlPort.Read();
   ```

3. Clear error, set sector count, disable interrupts:

   ```cpp
   errorPort.Write(0);
   sectorCountPort.Write(1);
   controlPort.Write(0x02); // disable IRQs
   ```

4. Set LBA low/mid/high bytes:

   ```cpp
   lbaLowPort.Write(sector & 0xFF);
   lbaMidPort.Write((sector >> 8) & 0xFF);
   lbaHiPort.Write((sector >> 16) & 0xFF);
   ```

5. Issue READ SECTORS command:

   ```cpp
   commandPort.Write(0x20);
   ```

### Polling for readiness

- Initial status:

  ```cpp
  uint8_t status = commandPort.Read();
  ```

- Wait for BSY to clear:

  ```cpp
  while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
    status = commandPort.Read();
  }
  if (status & 0x01) {
    printf("ERROR in ATA READ (BSY)\n");
    uint8_t error = errorPort.Read();
    printf("Error code: 0x");
    printByte(error);
    printf("\n");
    return;
  }
  ```

- Wait for DRQ to set:

  ```cpp
  while (!(status & 0x08) && !(status & 0x01)) {
    status = commandPort.Read();
  }
  if (status & 0x01) {
    printf("ERROR in ATA READ (DRQ)\n");
    return;
  }
  ```

### Data transfer

- Reads `count` bytes into `data`, 16 bits at a time:

  ```cpp
  printf("Reading ATA: ");

  for (uint16_t i = 0; i < count; i += 2) {
    uint16_t wdata = dataPort.Read();
    data[i] = wdata & 0xFF;
    if (i + 1 < count)
      data[i + 1] = (wdata >> 8) & 0xFF;

    // Debug print
    char text; [github](https://github.com/pac-ac/osakaOS/issues)
    text = data[i];
    text = (i + 1 < count) ? data[i + 1] : '\0'; [github](https://github.com/pac-ac/osakaOS)
    text = '\0'; [github](https://github.com/pac-ac/osakaOS/actions)
    printf(text);
  }

  printf(" || DONE\n");
  ```

- Drain the rest of the sector (if `count` < 512):

  ```cpp
  for (uint16_t i = count + (count % 2); i < bytesPerSector; i += 2) {
    dataPort.Read();
  }
  ```

- Final 400 ns delay:

  ```cpp
  for (int i = 0; i < 15; i++) {
    controlPort.Read();
  }
  ```

---

## Writing Sectors (LBA28)

```cpp
void AdvancedTechnologyAttachment::Write28(uint32_t sector, uint8_t* data, int count);
```

### Preconditions and validation

- Same sector and count checks as `Read28`:

  ```cpp
  if (sector & 0xF0000000) { ... }
  if (count > bytesPerSector) { ... }
  ```

### Command setup

- Matches read but with WRITE command:

  ```cpp
  devicePort.Write((master ? 0xE0 : 0xF0) | ((sector & 0x0F000000) >> 24));

  for (int i = 0; i < 15; i++)
    controlPort.Read();

  errorPort.Write(0);
  sectorCountPort.Write(1);
  controlPort.Write(0x02); // disable IRQs

  lbaLowPort.Write(sector & 0xFF);
  lbaMidPort.Write((sector >> 8) & 0xFF);
  lbaHiPort.Write((sector >> 16) & 0xFF);
  commandPort.Write(0x30); // WRITE SECTORS
  ```

- Polling for BSY cleared and DRQ set uses the same pattern as `Read28`, with appropriate error messages.

### Data transfer

- Writes `count` bytes from `data`, two bytes at a time:

  ```cpp
  printf("Writing ATA: ");

  for (uint16_t i = 0; i < count; i += 2) {
    uint16_t wdata = data[i];
    if (i + 1 < count)
      wdata |= ((uint16_t)data[i + 1]) << 8;

    // Debug print
    char text; [github](https://github.com/pac-ac/osakaOS/issues)
    text = wdata & 0xFF;
    text = (wdata >> 8) & 0xFF; [github](https://github.com/pac-ac/osakaOS)
    text = '\0'; [github](https://github.com/pac-ac/osakaOS/actions)
    printf(text);

    dataPort.Write(wdata);
  }

  printf(" || DONE\n");
  ```

- Fills the remainder of the 512‑byte sector with zeros:

  ```cpp
  for (uint16_t i = count + (count % 2); i < bytesPerSector; i += 2) {
    dataPort.Write(0x0000);
  }
  ```

### Completion and error checking

- Waits for the device to finish writing:

  ```cpp
  status = commandPort.Read();
  while ((status & 0x80) == 0x80) {
    status = commandPort.Read();
  }
  ```

- Checks for error:

  ```cpp
  if (status & 0x01) {
    printf("ERROR after writing data\n");
    uint8_t error = errorPort.Read();
    printf("Error code: 0x");
    printByte(error);
    printf("\n");
    return;
  }
  ```

- Final 400 ns delay:

  ```cpp
  for (int i = 0; i < 15; i++) {
    controlPort.Read();
  }
  ```

---

## Flush Cache

```cpp
void AdvancedTechnologyAttachment::Flush();
```

### Behavior

- Sends FLUSH CACHE command:

  ```cpp
  devicePort.Write(master ? 0xE0 : 0xF0);
  commandPort.Write(0xE7);
  ```

- If status is 0, returns immediately (no pending work):

  ```cpp
  uint8_t status = commandPort.Read();
  if (status == 0x00)
    return;
  ```

- Otherwise, waits for BSY to clear or error:

  ```cpp
  while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01)) {
    status = commandPort.Read();
  }
  if (status & 0x01) {
    printf("ERROR IN ATA FLUSH\n");
    uint8_t error = errorPort.Read();
    printf("Error code: 0x");
    printByte(error);
    printf("\n");
    return;
  }
  ```

---

## Usage and Integration

- A typical setup in `kernelMain` (currently commented in your code) looks like:

  ```cpp
  // primary ATA master on 0x1F0 (standard IDE base for primary channel)
  AdvancedTechnologyAttachment ata0m(0x1F0, true);
  if (ata0m.Identify())
    printf("ATA PRIMARY MASTER FOUND\n");
  ```

- Once identified, clients can:

  ```cpp
  uint8_t buffer;
  ata0m.Read28(sectorNumber, buffer, 512);
  // modify buffer
  ata0m.Write28(sectorNumber, buffer, 512);
  ata0m.Flush();
  ```

- This driver is meant to serve as a low-level backend for block‑oriented abstractions:
  - A future block device API (e.g., `BlockDevice` interface).
  - Filesystem implementations (FAT, ext2, custom).

---

## Invariants and Assumptions

- 28‑bit LBA mode only:
  - Valid sector range is 0–(2²⁸−1); higher bits are rejected.
- Sector size is fixed at 512 bytes (`bytesPerSector`).
- Only single‑sector operations (`sectorCountPort = 1`) are currently supported.
- All operations are synchronous and blocking:
  - Driver busy‑waits (polls status) and does not use interrupts for completion.
- Debug `printf` output in `Read28`/`Write28`:
  - Intended for development; may be removed or gated behind a debug flag for production.
- Error handling:
  - On errors, driver logs the error code from `errorPort` but does not attempt recovery or retries.

---

## Open Questions / TODO

- Add multi‑sector read/write support for improved performance.
- Wrap this driver in a higher‑level block device interface with:
  - Bounds checking.
  - Caching.
  - Better error propagation (return codes instead of only `printf`).
- Add support for secondary ATA channel (e.g., `portBase = 0x170`) and slave devices.
- Introduce non‑blocking or interrupt-driven I/O paths for multitasking environments.
- Consider abstracting the debug output away from the driver (e.g., via a logging interface).
```
