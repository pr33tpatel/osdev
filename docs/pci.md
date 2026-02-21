```markdown
# PCI Subsystem

## Responsibility

- Discover PCI devices on the system bus.
- Read configuration space (IDs, class codes, BARs, interrupt lines).
- Enable features like bus mastering on selected devices.
- Instantiate and register appropriate drivers (e.g., AMD am79c973 NIC) with `DriverManager`.

---

## Components

- `class PeripheralComponentInterconnectController`
- `struct PeripheralComponentInterconnectDeviceDescriptor`
- `struct BaseAddressRegister`
- Uses:
  - `Port32Bit` (via `commandPort` at 0xCF8 and `dataPort` at 0xCFC).
  - `DriverManager` and `Driver` for driver registration.
  - `InterruptManager` for IRQ integration.
  - Specific device driver `amd_am79c973`.

---

## PCI Device Descriptor

### `PeripheralComponentInterconnectDeviceDescriptor`

Represents key configuration data for a single PCI function:

- Fields (from implementation usage):
  - `uint16_t bus;`
  - `uint16_t device;`
  - `uint16_t function;`
  - `uint16_t vendor_id;`
  - `uint16_t device_id;`
  - `uint8_t class_id;`
  - `uint8_t subclass_id;`
  - `uint8_t interface_id;`
  - `uint8_t revision;`
  - `uint8_t interrupt;`
  - `uint32_t portBase;` (I/O base, set via BAR parsing when applicable).
- Constructors/destructor are trivial (no special logic).

---

## PCI Configuration Access

### Ports and controller construction

```cpp
PeripheralComponentInterconnectController::PeripheralComponentInterconnectController()
    : dataPort(0xCFC), commandPort(0xCF8) {}

PeripheralComponentInterconnectController::~PeripheralComponentInterconnectController() {}
```

- Uses standard PCI configuration mechanism:
  - Command port: `0xCF8`
  - Data port: `0xCFC`

### Read / Write API

```cpp
uint32_t PeripheralComponentInterconnectController::Read(
    uint16_t bus, uint16_t device, uint16_t function, uint32_t registerOffset);

void PeripheralComponentInterconnectController::Write(
    uint16_t bus, uint16_t device, uint16_t function, uint32_t registerOffset, uint32_t value);
```

- Address construction (configuration address):

```cpp
uint32_t id =
    0x1 << 31
    | ((bus & 0xFF) << 16)
    | ((device & 0x1F) << 11)
    | ((function & 0x07) << 8)
    | (registerOffset & 0xFC);
```

- Read:
  - Writes `id` to `commandPort`.
  - Reads 32 bits from `dataPort`.
  - Returns the requested byte shifted out of the 32‑bit result:
    - `result >> (8 * (registerOffset % 4))`.
- Write:
  - Writes `id` to `commandPort`.
  - Writes `value` to `dataPort` (full 32 bits).

### Multi-function detection

```cpp
bool PeripheralComponentInterconnectController::DeviceHasFunctions(uint16_t bus, uint16_t device) {
  return Read(bus, device, 0, 0x0E) & (1 << 7);
}
```

- Checks bit 7 of the header type register (offset 0x0E).
- If set: device is multi-function (up to 8 functions).
- Else: assume a single function.

---

## Device Enumeration and Driver Selection

### `SelectDrivers`

```cpp
void PeripheralComponentInterconnectController::SelectDrivers(
    DriverManager* driverManager, InterruptManager* interrupts);
```

- Enumerates a subset of the PCI bus:
  - `bus` in `[0, 8)`
  - `device` in `[0, 32)`
  - `function` count:
    - 8 if `DeviceHasFunctions(bus, device)` is true.
    - 1 otherwise.
- For each function:
  1. Obtain `PeripheralComponentInterconnectDeviceDescriptor dev = GetDeviceDescriptor(bus, device, function);`
  2. Skip if `dev.vendor_id` is `0x0000` or `0xFFFF` (no device / invalid).
  3. For each BAR index `barNum` in `[0, 6)`:
     - `BaseAddressRegister bar = GetBaseAddressRegister(bus, device, function, barNum);`
     - If `bar.address` is non‑null and `bar.type == InputOutput`, set:
       - `dev.portBase = (uint32_t)bar.address;`
  4. Ask for a driver:
     - `Driver* driver = GetDriver(dev, interrupts);`
     - If non‑null, call `driverManager->AddDriver(driver);`

- Invariants:
  - Only I/O BARs are used to populate `dev.portBase`.
  - Device detection ignores vendor ID `0x0000/0xFFFF`.

### Debug printing

```cpp
void PeripheralComponentInterconnectController::PrintPCIDrivers();
```

- Same enumeration as `SelectDrivers`, but instead of instantiating drivers, prints:

```cpp
printf("PCI BUS %04x, DEVICE %04x, VENDOR %04x\n",
       (uint32_t)bus,
       (uint32_t)dev.device_id,
       (uint32_t)dev.vendor_id);
```

- Can be used as the kernel’s equivalent of `lspci` to see available devices.

---

## Base Address Registers (BARs)

### `GetBaseAddressRegister`

```cpp
BaseAddressRegister PeripheralComponentInterconnectController::GetBaseAddressRegister(
    uint16_t bus, uint16_t device, uint16_t function, uint16_t bar);
```

- Determines how a function maps its resources (I/O vs memory) via BARs.

Process:

1. Determine header type:

```cpp
uint32_t headertype = Read(bus, device, function, 0x0E) & 0x7F;
int maxBARs = 6 - (4 * headertype);
if (bar >= maxBARs) return result;  // returns default (address == null)
```

- For non‑standard header types, `maxBARs` is reduced accordingly.

2. Read BAR value:

```cpp
uint32_t bar_value = Read(bus, device, function, 0x10 + 4 * bar);
result.type = (bar_value & 0x1) ? InputOutput : MemoryMapping;
```

- If `InputOutput`:
  - `result.address = (uint8_t*)(bar_value & ~0x3);`
    - Mask off the low two bits (flags) to get the I/O base.
  - `result.prefetchable = false;`
- If `MemoryMapping`:
  - For now, only checks the address type:

```cpp
switch ((bar_value >> 1) & 0x3) {
  case 0: // 32-bit
  case 1: // 20-bit
  case 2: // 64-bit
    break;
}
```

  - No further memory mapping setup is performed here; memory‑mapped BARs are not fully handled yet.

- Invariants:
  - If `bar` index is out of range for current header type, the returned `BaseAddressRegister` has a null/zero address.
  - I/O BARs are interpreted as port ranges; memory BARs currently have no fully implemented mapping logic.

---

## Device Descriptor Retrieval

### `GetDeviceDescriptor`

```cpp
PeripheralComponentInterconnectDeviceDescriptor
PeripheralComponentInterconnectController::GetDeviceDescriptor(
    uint16_t bus, uint16_t device, uint16_t function);
```

- Populates a descriptor as follows:

```cpp
result.bus        = bus;
result.device     = device;
result.function   = function;

result.vendor_id  = Read(bus, device, function, 0x00);
result.device_id  = Read(bus, device, function, 0x02);

result.class_id     = Read(bus, device, function, 0x0b);
result.subclass_id  = Read(bus, device, function, 0x0a);
result.interface_id = Read(bus, device, function, 0x09);

result.revision  = Read(bus, device, function, 0x08);
result.interrupt = Read(bus, device, function, 0x3c);
```

- This captures identity (vendor/device), type (class/subclass/interface), revision, and interrupt line.

---

## Driver Instantiation

### `GetDriver`

```cpp
Driver* PeripheralComponentInterconnectController::GetDriver(
    PeripheralComponentInterconnectDeviceDescriptor dev,
    InterruptManager* interrupts);
```

- Chooses and constructs a driver based on `vendor_id`, `device_id`, and class.

Current logic:

- AMD devices:

```cpp
case 0x1022:  // AMD
  switch (dev.device_id) {
    case 0x2000:  // am79c973 NIC
      EnableBusMastering(&dev);
      driver = new amd_am79c973(&dev, interrupts);
      if (driver != 0) {
        new (driver) amd_am79c973(&dev, interrupts); // placement new (currently redundant)
      }
      return driver;
  }
```

- Intel devices (placeholder):

```cpp
case 0x8086:  // Intel
  switch (dev.device_id) {
    case 0x7000:
      // TODO: intel_piix3 driver
      break;
  }
```

- Class‑based handling (example):

```cpp
switch (dev.class_id) {
  case 0x03:  // graphics
    switch (dev.subclass_id) {
      case 0x00:  // VGA graphics
        // TODO: VGA driver integration
        break;
    }
    break;
}
```

- Returns:
  - Pointer to a constructed `Driver` subclass if recognized.
  - `0` if no driver is available for this device.

Invariants:

- Any driver created here is expected to be registered with `DriverManager` and activated later.
- `EnableBusMastering` is invoked for the AMD NIC before driver construction.

---

## Enabling Bus Mastering

### By (bus, device, function)

```cpp
void PeripheralComponentInterconnectController::EnableBusMastering(
    uint16_t bus, uint16_t device, uint16_t function);
```

- Reads the PCI command register at offset `0x04`:

```cpp
uint32_t command = Read(bus, device, function, 0x04);
if (!(command & 0x4)) {
  Write(bus, device, function, 0x04, command | 0x4);
}
```

- Sets bit 2 if not already set:
  - Enables bus mastering (DMA), allowing the device to initiate memory transactions.

### By device descriptor

```cpp
void PeripheralComponentInterconnectController::EnableBusMastering(
    PeripheralComponentInterconnectDeviceDescriptor* dev);
```

- Convenience wrapper that calls the above using `dev->bus`, `dev->device`, and `dev->function`.

Invariants:

- Bus mastering must be enabled for devices like NICs that rely on DMA.
- Must only be called after configuration space is accessible and before the driver begins heavy use.

---

## Invariants and Assumptions

- Only buses `0–7` are scanned; systems with devices on higher bus numbers are not yet fully supported.
- Devices with `vendor_id == 0x0000` or `0xFFFF` are treated as non‑existent or invalid.
- BAR parsing:
  - I/O BARs are correctly parsed and used to set `dev.portBase`.
  - Memory BARs are partially recognized but not fully mapped/used.
- `GetDriver` contains explicit knowledge of certain vendor/device IDs (e.g., AMD am79c973) and can be extended with more device support over time.
- Bus mastering is enabled only when needed; other command register bits (I/O space, memory space, etc.) are not modified here.

---

## Open Questions / TODO

- Extend bus enumeration beyond bus 7 if needed for more complex hardware setups.
- Fully implement handling for memory‑mapped BARs:
  - Map device memory into the kernel’s virtual address space.
  - Document alignment and caching behavior.
- Replace hard‑coded vendor/device IDs with a table‑driven registration mechanism for cleaner extensibility.
- Integrate `PrintPCIDrivers()` output with the CLI (e.g., `lspci`‑like command) for runtime inspection.
- Clarify how the `interrupt` field from `GetDeviceDescriptor()` ties into `InterruptManager` and driver IRQ registration (document expected IRQ wiring per device).
```
