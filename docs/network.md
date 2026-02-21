```markdown
# Network Stack

## Overview

The network stack is built in layers:

- **NIC driver (AMD am79c973)**: talks to the real/virtual hardware and sends/receives raw Ethernet frames.
- **Ethernet layer (`EtherFrameProvider` / `EtherFrameHandler`)**: frames with MAC addresses and EtherType.
- **ARP (`AddressResolutionProtocol`)**: maps IPv4 addresses to MAC addresses, caching results.
- **IPv4 (`InternetProtocolProvider` / `InternetProtocolHandler`)**: routing, headers, checksum.
- **ICMP (`InternetControlMessageProtocol`)**: ping/echo on top of IPv4.

Each layer registers a handler with the layer below and exposes a simple “send payload, I’ll wrap it” API to the layer above.

---

## NIC: AMD am79c973 Driver

### Roles

- Initialize and configure the AMD am79c973 network device (QEMU `pcnet` card).
- Manage DMA ring buffers for transmit and receive.
- Handle interrupts from the device and notify higher layers of incoming data.
- Expose `Send` and `SetHandler` APIs used by `EtherFrameProvider`.

### RawDataHandler

```cpp
class RawDataHandler {
  amd_am79c973* backend;
 public:
  RawDataHandler(amd_am79c973* backend);
  virtual ~RawDataHandler();

  virtual bool OnRawDataReceived(uint8_t* buffer, uint32_t size);
  void Send(uint8_t* buffer, uint32_t size);
};
```

- Base class between the NIC and Ethernet layer.
- Constructor registers itself as the NIC’s handler: `backend->SetHandler(this)`.
- Default `OnRawDataReceived` returns `false` (no send-back).
- `Send` forwards raw bytes to `amd_am79c973::Send`.

### NIC construction and initialization

```cpp
amd_am79c973::amd_am79c973(
    PeripheralComponentInterconnectDeviceDescriptor* dev,
    InterruptManager* interrupts)
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
  // ...
}
```

- Registers as:
  - A generic `Driver`.
  - An `InterruptHandler` for the device’s interrupt line (`dev->interrupt + hardwareInterruptOffset`).
- Reads MAC address from three 16‑bit ports:
  - Low/high bytes at `portBase`, `portBase + 2`, `portBase + 4`.
  - Assembles into a 48‑bit `MAC` (`MAC0..MAC5`).
- Configures device:
  - Set to 32‑bit mode:

    ```cpp
    registerAddressPort.Write(20);
    busControlRegisterDataPort.Write(0x102);
    ```

  - Issue STOP/reset:

    ```cpp
    registerAddressPort.Write(0);
    registerDataPort.Write(0x04);
    ```

- Initialize `initBlock`:
  - `mode = 0x0000` (promiscuous off; can be set to `0x8000` for promiscuous).
  - `numSendBuffers = 3;`
  - `numRecvBuffers = 3;`
  - `physicalAddress = MAC;`
  - `logicalAddress = 0;` (filled later by `SetIPAddress`).
- Align and set up buffer descriptors:

  ```cpp
  sendBufferDescr = (BufferDescriptor*)((((uint32_t)&sendBufferDescrMemory) + 15) & ~0xFu);
  initBlock.sendBufferDescrAddress = (uint32_t)sendBufferDescr;

  recvBufferDescr = (BufferDescriptor*)((((uint32_t)&recvBufferDescrMemory) + 15) & ~0xFu);
  initBlock.recvBufferDescrAddress = (uint32_t)recvBufferDescr;
  ```

  - Aligns descriptor arrays to 16‑byte boundaries.
  - Fills send/receive descriptors with buffer addresses, flags, and default states (owned by card, etc.).
- Program NIC with init block address:

  ```cpp
  registerAddressPort.Write(1);
  registerDataPort.Write((uint32_t)(&initBlock) & 0xFFFF);
  registerAddressPort.Write(2);
  registerDataPort.Write(((uint32_t)(&initBlock) >> 16) & 0xFFFF);
  ```

### Activation

```cpp
void amd_am79c973::Activate() {
  printf("AMD am79c973 Activating...\n");

  // CSR0: INIT + IENA
  registerAddressPort.Write(0);
  registerDataPort.Write(0x41);

  // CSR4: enable auto-pad (fix short packets)
  registerAddressPort.Write(4);
  uint32_t temp = registerDataPort.Read();
  registerAddressPort.Write(4);
  registerDataPort.Write(temp | 0xC00);

  // CSR3: unmask RX/TX interrupts
  registerAddressPort.Write(3);
  uint32_t csr3 = registerDataPort.Read();
  registerAddressPort.Write(3);
  registerDataPort.Write(csr3 & ~((1 << 10) | (1 << 9)));

  // CSR0: STRT + IENA
  registerAddressPort.Write(0);
  registerDataPort.Write(0x42);
}
```

- Enables initialization, auto-padding, receive/transmit interrupts, then starts the card.

### Interrupt handling

```cpp
uint32_t amd_am79c973::HandleInterrupt(uint32_t esp) {
  registerAddressPort.Write(0);
  uint32_t temp = registerDataPort.Read();

  if ((temp & 0x8000) == 0x8000) printf("NETWORK ERROR: AMD am79c973 ERROR\n");
  if ((temp & 0x2000) == 0x2000) printf("NETWORK ERROR: AMD am79c973 COLLISION ERROR\n");
  if ((temp & 0x1000) == 0x1000) printf("NETWORK ERROR: AMD am79c973 MISSED FRAME\n");
  if ((temp & 0x0800) == 0x0800) printf("NETWORK ERROR: AMD am79c973 MEMORY ERROR\n");
  if ((temp & 0x0400) == 0x0400) printf("NETWORK INTERRUPT: DATA RECEIVED\n");
  Receive();
  if ((temp & 0x0200) == 0x0200) printf("NETWORK INTERRUPT: DATA SENT\n");

  registerAddressPort.Write(0);
  registerDataPort.Write(temp);   // acknowledge

  return esp;
}
```

- Reads CSR0 status bits, logs errors, calls `Receive` when data arrived, acknowledges the interrupt.

### Sending packets

```cpp
void amd_am79c973::Send(uint8_t* buffer, int size);
```

- Uses a ring of 8 send descriptors:
  - `sendDescriptor = currentSendBuffer;`
  - `currentSendBuffer = (currentSendBuffer + 1) % 8;`
- Clamps size to MTU (1518 bytes).
- Copies payload into the send buffer, starting from the end (device-specific requirement).
- Sets descriptor flags and triggers transmission:

```cpp
sendBufferDescr[sendDescriptor].flags = 0x8300F000 | ((uint16_t)((-size) & 0xFFF));
registerAddressPort.Write(0);
registerDataPort.Write(0x48);
```

### Receiving packets

```cpp
void amd_am79c973::Receive();
```

- Loops as long as the current receive descriptor is “owned by CPU” (bit 31 cleared):

```cpp
for (; (recvBufferDescr[currentRecvBuffer].flags & 0x80000000) == 0;
     currentRecvBuffer = (currentRecvBuffer + 1) % 8) {
  // ...
}
```

- Checks for valid packet:

```cpp
if (!(recvBufferDescr[currentRecvBuffer].flags & 0x40000000) &&
    (recvBufferDescr[currentRecvBuffer].flags & 0x03000000) == 0x03000000) {
  uint32_t size = recvBufferDescr[currentRecvBuffer].flags & 0xFFF;
  if (size > 64) size -= 4; // remove checksum
  uint8_t* buffer = (uint8_t*)(recvBufferDescr[currentRecvBuffer].address);

  if (handler != 0) {
    if (handler->OnRawDataReceived(buffer, size)) {
      Send(buffer, size);
    }
  }
}
```

- After consumption, marks descriptor as free/owned by NIC again:

```cpp
recvBufferDescr[currentRecvBuffer].flags2 = 0;
recvBufferDescr[currentRecvBuffer].flags = 0x8000F7FF;
```

### IP/MAC getters / setters

```cpp
void amd_am79c973::SetHandler(RawDataHandler* handler);
uint64_t amd_am79c973::GetMACAddress();
void amd_am79c973::SetIPAddress(uint32_t IP);
uint32_t amd_am79c973::GetIPAddress();
```

- `GetMACAddress` returns the 48‑bit MAC stored in `initBlock.physicalAddress`.
- `SetIPAddress` / `GetIPAddress` store a 32‑bit IP (big‑endian) in `initBlock.logicalAddress`.

---

## Ethernet Layer

### EtherFrameHandler

```cpp
class EtherFrameHandler {
 protected:
  EtherFrameProvider* backend;
  uint16_t etherType_BE;
 public:
  EtherFrameHandler(EtherFrameProvider* backend, uint16_t etherType);
  virtual ~EtherFrameHandler();

  virtual bool OnEtherFrameReceived(uint8_t* etherFramePayload, uint32_t size);
  void Send(uint64_t dstMAC_BE, uint8_t* data, uint32_t size);
};
```

- Registers for a specific EtherType:
  - Converts `etherType` to big‑endian and stores it in `etherType_BE`.
  - Registers itself:

    ```cpp
    backend->handlers[etherType_BE] = this;
    ```

- `OnEtherFrameReceived` default: `false` (no response).
- `Send` delegates to `EtherFrameProvider::Send`.

### EtherFrameProvider

```cpp
class EtherFrameProvider : public drivers::RawDataHandler {
  EtherFrameHandler* handlers;
  amd_am79c973* backend;
public:
  EtherFrameProvider(amd_am79c973* backend);
  ~EtherFrameProvider();

  bool OnRawDataReceived(uint8_t* buffer, uint32_t size) override;
  void Send(uint64_t dstMAC_BE, uint16_t etherType_BE, uint8_t* data, uint32_t size);

  uint64_t GetMACAddress();
  uint32_t GetIPAddress();
};
```

- On construction:
  - Calls `RawDataHandler(backend)`; registers itself with the NIC.
  - Initializes `handlers[]` to null.

#### Receiving frames

```cpp
bool EtherFrameProvider::OnRawDataReceived(uint8_t* buffer, uint32_t size) {
  if (size < sizeof(EtherFrameHeader)) return false;

  EtherFrameHeader* frame = (EtherFrameHeader*)buffer;
  bool sendBack = false;

  if (frame->dstMac_BE == 0xFFFFFFFFFFFF || frame->dstMac_BE == backend->GetMACAddress()) {
    if (handlers[frame->etherType_BE] != 0) {
      sendBack = handlers[frame->etherType_BE]->OnEtherFrameReceived(
          buffer + sizeof(EtherFrameHeader),
          size - sizeof(EtherFrameHeader)
      );
    }
  }

  if (sendBack) {
    frame->dstMac_BE = frame->srcMac_BE;
    frame->srcMac_BE = backend->GetMACAddress();
  }

  return sendBack;
}
```

- Accepts frames if:
  - Destination MAC is broadcast (`FF:FF:FF:FF:FF:FF`), or
  - Destination MAC equals our NIC’s MAC.
- If a handler is registered for the EtherType, it passes payload to that handler.
- If handler returns `true`:
  - Swaps MACs:
    - Destination becomes original source.
    - Source becomes our MAC.
  - Returns `true` to NIC layer, which will send the modified frame back out.

#### Sending frames

```cpp
void EtherFrameProvider::Send(
    uint64_t dstMAC_BE,
    uint16_t etherType_BE,
    uint8_t* data,
    uint32_t size);
```

- Allocates `sizeof(EtherFrameHeader) + size` bytes.
- Fills:

  ```cpp
  frame->dstMac_BE    = dstMAC_BE;
  frame->srcMac_BE    = backend->GetMACAddress();
  frame->etherType_BE = etherType_BE;
  ```

- Copies payload into buffer after the header.
- Calls `backend->Send(buffer, header + payload size)`.
- Frees the buffer afterward.

#### Helpers

- `GetMACAddress()` and `GetIPAddress()` just delegate to the NIC.

---

## ARP Layer

### Roles

- Provide IPv4 → MAC address resolution.
- Cache mappings for faster reuse.
- Listen on EtherType `0x0806` (ARP).

### Construction

```cpp
AddressResolutionProtocol::AddressResolutionProtocol(EtherFrameProvider* backend)
    : EtherFrameHandler(backend, 0x806) {
  numCacheEntries = 0;
}
```

- Registers for EtherType `0x0806` (ARP).
- Maintains:

  ```cpp
  uint32_t IPcache;
  uint64_t MACcache;
  int numCacheEntries;
  ```

### Debug helpers

- `printIPAddress(uint32_t IP)` and `printMACAddress(uint64_t MAC)` print in dotted form.
- `printSrcIPAddress()` / `printSrcMACAddress()` print the local IP/MAC.
- `printARPmsg(...)` prints parsed ARP packet fields.

### Receiving ARP

```cpp
bool AddressResolutionProtocol::OnEtherFrameReceived(uint8_t* etherframePayload, uint32_t size);
```

- Validates packet length and fields:

  ```cpp
  if (arp->hardwareType == 0x0100 && // Ethernet
      arp->protocol == 0x0008 &&     // IPv4
      arp->hardwareAddressSize == 6 &&
      arp->protocolAddressSize == 4 &&
      arp->dstIP == backend->GetIPAddress()) {
    // ...
  }
  ```

- If ARP is targeted at our IP:

  - For `command == 0x0100` (request):
    - Build a reply in-place:

      ```cpp
      arp->command = 0x0200;
      arp->dstIP   = arp->srcIP;
      arp->dstMAC  = arp->srcMAC;
      arp->srcIP   = backend->GetIPAddress();
      arp->srcMAC  = backend->GetMACAddress();
      return true;   // EtherFrameProvider will swap MACs and send it back
      ```

  - For `command == 0x0200` (response):
    - Cache mapping:

      ```cpp
      if (numCacheEntries < 128) {
        IPcache[numCacheEntries]  = arp->srcIP;
        MACcache[numCacheEntries] = arp->srcMAC;
        numCacheEntries++;
      }
      ```

- Returns `true` only for ARP requests to us (so they get answered), otherwise `false`.

### Sending ARP

```cpp
void AddressResolutionProtocol::RequestMACAddress(uint32_t IP_BE);
```

- Constructs an ARP request:

  ```cpp
  arp.hardwareType = 0x0100;  // ethernet
  arp.protocol      = 0x0008; // ipv4
  arp.hardwareAddressSize = 6;
  arp.protocolAddressSize = 4;
  arp.command = 0x0100;       // request

  arp.srcMAC = backend->GetMACAddress();
  arp.srcIP  = backend->GetIPAddress();
  arp.dstMAC = 0xFFFFFFFFFFFF; // broadcast
  arp.dstIP  = IP_BE;
  ```

- Sends via `EtherFrameHandler::Send(dstMAC, &arp, sizeof(arp))`.

```cpp
void AddressResolutionProtocol::BroadcastMACAddress(uint32_t IP_BE);
```

- Constructs a reply-like packet where:
  - `srcMAC` / `srcIP` = our MAC/IP.
  - `dstMAC` = resolved MAC (or broadcast, depending on code path).
  - `dstIP` = target IP.
  - `command = 0x0200` (response).
- Sends it via Ethernet.

### Resolution and cache

```cpp
uint64_t AddressResolutionProtocol::GetMACFromCache(uint32_t IP_BE);
uint64_t AddressResolutionProtocol::Resolve(uint32_t IP_BE);
```

- `GetMACFromCache`:
  - Linear search through up to 128 entries, returns MAC or broadcast `0xFFFFFFFFFFFF` if not found.
- `Resolve`:
  - If cache miss:
    - Calls `RequestMACAddress(IP_BE)`.
  - Polls up to 128 attempts, waiting for ARP response to populate cache.
  - If still not found, prints `"ARP Resolve Time Out."`.
  - Returns the MAC (or broadcast if unresolved).

---

## IPv4 Layer

### InternetProtocolHandler

```cpp
class InternetProtocolHandler {
 protected:
  InternetProtocolProvider* backend;
  uint8_t ip_protocol;

 public:
  InternetProtocolHandler(InternetProtocolProvider* backend, uint8_t protocol);
  virtual ~InternetProtocolHandler();

  virtual bool OnInternetProtocolReceived(
      uint32_t srcIP_BE, uint32_t dstIP_BE,
      uint8_t* internetprotocolPayload, uint32_t size);

  void Send(uint32_t dstIP_BE, uint8_t* payload, uint32_t size);
};
```

- Registers for a given protocol number (e.g., 1 for ICMP).
- `OnInternetProtocolReceived` default: `false`.
- `Send` delegates to `InternetProtocolProvider::Send`.

### InternetProtocolProvider

```cpp
class InternetProtocolProvider : public EtherFrameHandler {
  InternetProtocolHandler* handlers;
  AddressResolutionProtocol* arp;
  uint32_t gatewayIP;
  uint32_t subnetMask;
public:
  InternetProtocolProvider(
      EtherFrameProvider* backend,
      AddressResolutionProtocol* arp,
      uint32_t gatewayIP,
      uint32_t subnetMask);
  ~InternetProtocolProvider();

  bool OnEtherFrameReceived(uint8_t* etherframePayload, uint32_t size) override;
  void Send(uint32_t dstIP_BE, uint8_t protocol, uint8_t* data, uint32_t size);

  static uint16_t Checksum(void* data, uint32_t lengthInBytes);
};
```

- Registers for EtherType `0x0800` (IPv4).
- Keeps:
  - `handlers[protocol]` for TCP/UDP/ICMP/etc.
  - `arp`, `gatewayIP`, `subnetMask` for routing.

#### Receiving IPv4

```cpp
bool InternetProtocolProvider::OnEtherFrameReceived(uint8_t* etherframePayload, uint32_t size) {
  if (size < sizeof(InternetProtocolMessage)) return false;

  InternetProtocolMessage* ip_message = (InternetProtocolMessage*)etherframePayload;
  bool sendBack = false;

  if (ip_message->dstIP == backend->GetIPAddress()) {
    int length = ip_message->totalLength;
    if (length > size) length = size;

    if (handlers[ip_message->protocol] != 0) {
      sendBack = handlers[ip_message->protocol]->OnInternetProtocolReceived(
          ip_message->srcIP,
          ip_message->dstIP,
          etherframePayload + 4 * ip_message->headerLength,
          size - 4 * ip_message->headerLength
      );
    } else {
      printf("IPV4 error: no handler for protocol %d\n", ip_message->protocol);
    }
  } else {
    printf("IPV4 drop: DstIP: %08x != MyIP %08x\n", ip_message->dstIP, ip_message->srcIP);
  }

  if (sendBack) {
    uint32_t temp = ip_message->dstIP;
    ip_message->dstIP = ip_message->srcIP;
    ip_message->srcIP = temp;

    ip_message->timeToLive = 0x40;
    ip_message->checksum = 0;
    ip_message->checksum =
        Checksum((uint16_t*)(void*)ip_message, 4 * ip_message->headerLength);
  }

  return sendBack;
}
```

- Only processes packets where `dstIP == my IP`.
- Routes them to the registered handler for `ip_message->protocol`.
- If handler returns `true`:
  - Swap src/dst IP.
  - Reset TTL to 64.
  - Recompute checksum.
  - Return `true` so the lower layer will send the modified packet back.

#### Sending IPv4

```cpp
void InternetProtocolProvider::Send(
    uint32_t dstIP_BE,
    uint8_t protocol,
    uint8_t* data,
    uint32_t size);
```

- Allocates `InternetProtocolMessage + payload` with `new[]`.
- Fills header:

  ```cpp
  message->version = 4;
  message->headerLength = sizeof(InternetProtocolMessage) / 4;
  message->tos = 0;
  message->totalLength = size + sizeof(InternetProtocolMessage);
  message->totalLength = ((message->totalLength & 0xFF00) >> 8) | ((message->totalLength & 0x00FF) << 8);
  message->identification = 0x0100;
  message->flagsAndOffset = 0x0040;
  message->timeToLive = 0x40;
  message->protocol = protocol;

  message->dstIP = dstIP_BE;
  message->srcIP = backend->GetIPAddress();

  message->checksum = 0;
  message->checksum = Checksum((uint16_t*)(void*)message, sizeof(InternetProtocolMessage));
  ```

- Copies payload immediately after the header.
- Routing decision:
  - If `(dstIP_BE & subnetMask) != (srcIP & subnetMask)`, then route via gateway:
    - `dstIP_BE = gatewayIP;`
- Uses ARP to resolve the final MAC:

  ```cpp
  backend->Send(arp->Resolve(dstIP_BE), this->etherType_BE, buffer,
                sizeof(InternetProtocolMessage) + size);
  ```

- Frees the buffer with `delete[]`.

#### Checksum

```cpp
uint16_t InternetProtocolProvider::Checksum(void* data_, uint32_t lengthInBytes);
```

- Computes 16‑bit Internet checksum in big‑endian order.
- Steps:
  - Interpret data as `uint16_t*`.
  - Add each 16‑bit word with swapped bytes (big‑endian).
  - If odd length, add last byte shifted into high bits.
  - Fold carries until result fits in 16 bits.
  - Return bitwise complement, bytes swapped to big‑endian.

---

## ICMP Layer

### InternetControlMessageProtocol

```cpp
class InternetControlMessageProtocol : public InternetProtocolHandler {
public:
  InternetControlMessageProtocol(InternetProtocolProvider* backend);
  ~InternetControlMessageProtocol();

  bool OnInternetProtocolReceived(
      uint32_t srcIP_BE, uint32_t dstIP_BE,
      uint8_t* internetprotocolPayload, uint32_t size) override;

  void Ping(uint32_t ip_BE);
};
```

- Registers for IPv4 protocol `1` (ICMP).

#### Receiving ICMP

```cpp
bool InternetControlMessageProtocol::OnInternetProtocolReceived(
    uint32_t srcIP_BE, uint32_t dstIP_BE,
    uint8_t* internetprotocolPayload, uint32_t size) {
  printf("ICMP RECV\n");
  if (size < sizeof(InternetControlMessageProtocolMessage)) return false;

  InternetControlMessageProtocolMessage* msg =
      (InternetControlMessageProtocolMessage*)internetprotocolPayload;

  switch (msg->type) {
    case 0: // echo reply
      printf("ping response from: 0x%08x\n", srcIP_BE);
      return false;

    case 8: // echo request
      msg->type = 0;   // convert to reply
      msg->checksum = 0;
      msg->checksum =
          InternetProtocolProvider::Checksum((uint16_t*)msg,
                                             sizeof(InternetControlMessageProtocolMessage));
      return true;
  }
  return false;
}
```

- Prints when an ICMP packet is received.
- For type 0 (reply) logs and does not respond.
- For type 8 (echo request):
  - Converts to echo reply in place.
  - Recomputes checksum.
  - Returns `true` so IPv4 layer will swap src/dst IP and send back.

#### Sending ICMP (Ping)

```cpp
void InternetControlMessageProtocol::Ping(uint32_t ip_BE) {
  InternetControlMessageProtocolMessage icmp;
  icmp.type = 8;     // echo request
  icmp.code = 0;
  icmp.data = 0x3713; // arbitrary data
  icmp.checksum = 0;
  icmp.checksum =
      InternetProtocolProvider::Checksum((uint16_t*)&icmp,
                                         sizeof(InternetControlMessageProtocolMessage));

  InternetProtocolHandler::Send(ip_BE,
      (uint8_t*)&icmp, sizeof(InternetControlMessageProtocolMessage));
}
```

- Builds a simple ICMP echo request and sends it via the IPv4 layer.
- The stack will handle routing, ARP resolution, Ethernet framing, and NIC transmission.

---

## Data Flow Summary

- **Outbound** (e.g., ICMP Ping):
  1. CLI / kernel calls `icmp.Ping(targetIP_BE)`.
  2. ICMP builds an ICMP message and calls `InternetProtocolHandler::Send`.
  3. IPv4 layer wraps it in an `InternetProtocolMessage` and chooses route:
     - Direct if target in same subnet, otherwise via `gatewayIP`.
  4. IPv4 resolves target MAC via ARP (request/response and cache).
  5. IPv4 calls `EtherFrameProvider::Send(dstMAC, EtherType IPv4, buffer)`.
  6. Ethernet layer wraps it in an Ethernet frame and passes to NIC driver.
  7. NIC driver pushes frame into send ring and kicks hardware.

- **Inbound** (e.g., ARP request or ICMP reply):
  1. NIC hardware receives frame, writes into receive buffer, triggers interrupt.
  2. `amd_am79c973::HandleInterrupt` calls `Receive`, which calls `handler->OnRawDataReceived(...)` on `EtherFrameProvider`.
  3. Ethernet layer parses header, dispatches by EtherType:
     - ARP: `AddressResolutionProtocol::OnEtherFrameReceived`.
     - IPv4: `InternetProtocolProvider::OnEtherFrameReceived`.
  4. IPv4 validates destination IP and passes payload to protocol handler (e.g., ICMP).
  5. ICMP prints/logs or turns a request into a reply (in‑place), causing a send-back through the stack.

---

## Open Questions / TODO

- Define precise endianness conventions for IP and MAC in all structures and ensure they are consistently used across layers.
- Add support for more IPv4 protocols (e.g., UDP, TCP) by implementing additional `InternetProtocolHandler` subclasses.
- Replace blocking ARP resolution loops with a non‑blocking or timeout-aware mechanism integrated with the scheduler.
- Harden error handling and logging (e.g., more detailed NIC error bits, IPv4 header validation).
- Add configuration options (e.g., DHCP, multiple interfaces, dynamic routes) on top of the static IP/gateway/subnet currently set in `kernelMain`.
```
