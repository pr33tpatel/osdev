# Hardware Communication

## Responsibility

- Provide low-level CPU I/O port access (8/16/32‑bit, slow variants).
- Configure and manage the Programmable Interrupt Controller (PIC) and Interrupt Descriptor Table (IDT).
- Dispatch interrupts to C++ handlers and integrate with the task scheduler.
- Provide a generic driver base class and a driver manager for activation.
- Configure the Programmable Interval Timer (PIT) and expose a simple tick/Wait API.

PCI bus support is documented separately in `pci.md`.

---

## Port I/O (`port.cc`)

### Components

- `class Port`
- `class Port8Bit`
- `class Port8BitSlow`
- `class Port16Bit`
- `class Port32Bit`

### Model

- `Port` holds a 16‑bit I/O port number and is the base class for all typed ports.

```cpp
Port::Port(uint16_t portnumber);
Port::~Port();
```

- Typed ports wrap architecture-specific in/out instructions (implemented in `port.h` via `Write8`, `Read8`, etc.):

```cpp
class Port8Bit : public Port {
 public:
  void Write(uint8_t data);
  uint8_t Read();
};

class Port8BitSlow : public Port8Bit {
 public:
  void Write(uint8_t data);  // uses Write8Slow (extra delay)
};

class Port16Bit : public Port {
 public:
  void Write(uint16_t data);
  uint16_t Read();
};

class Port32Bit : public Port {
 public:
  void Write(uint32_t data);
  uint32_t Read();
};
```

### Usage and Invariants

- **Usage**:
  - Instantiate with the hardware port number, then call `Write`/`Read`.
  - Typical consumers: PIC ports (0x20, 0x21, 0xA0, 0xA1), PIT ports (0x40, 0x43), and device drivers.
- **Invariants**:
  - Port access must respect hardware timing constraints; use `Port8BitSlow` where slow I/O is required.
  - Only low‑level HAL and drivers should touch `Port` directly; other subsystems should use driver/HAL APIs.

---

## Interrupt Management (`interrupts.cc` + `interruptstubs.s`)

### Components

- `class InterruptHandler`
- `class InterruptManager`
- Assembly stubs in `interruptstubs.s`:
  - Exception entry points.
  - IRQ entry points.
  - Common prologue/epilogue (`int_bottom`).
  - `InterruptIgnore`.

### InterruptHandler

```cpp
class InterruptHandler {
 protected:
  uint8_t InterruptNumber;
  InterruptManager* interruptManager;

 public:
  InterruptHandler(InterruptManager* interruptManager, uint8_t InterruptNumber);
  virtual ~InterruptHandler();

  virtual uint32_t HandleInterrupt(uint32_t esp);
};
```

- When constructed:
  - Registers itself in `interruptManager->handlers[InterruptNumber]`.
- On destruction:
  - Clears its entry in `handlers` if it is still the active handler.
- Default `HandleInterrupt`:
  - Returns `esp` unchanged; subclasses override this for specific interrupts (e.g., PIT, keyboard, mouse).

### InterruptManager state

- Static members:

```cpp
static GateDescriptor interruptDescriptorTable;
static InterruptManager* ActiveInterruptManager;
```

- Per-instance state (simplified):
  - `uint16_t hardwareInterruptOffset;` (e.g., 0x20).
  - Ports for PIC:
    - Master command: 0x20
    - Master data: 0x21
    - Slave command: 0xA0
    - Slave data: 0xA1
  - `TaskManager* taskManager;`
  - `InterruptHandler* handlers[256];`

### IDT setup

```cpp
void InterruptManager::SetInterruptDescriptorTableEntry(
    uint8_t interrupt,
    uint16_t CodeSegment,
    void (*handler)(),
    uint8_t DescriptorPrivilegeLevel,
    uint8_t DescriptorType);
```

- Fills a `GateDescriptor` entry with:
  - Handler address low/high bits.
  - GDT code segment selector.
  - Access flags:
    - Present bit (0x80).
    - Descriptor privilege level (DPL).
    - Descriptor type (e.g., 0xE for interrupt gate).
- `InterruptManager` constructor:

```cpp
InterruptManager::InterruptManager(
    uint16_t hardwareInterruptOffset,
    GlobalDescriptorTable* globalDescriptorTable,
    TaskManager* taskManager);
```

- Steps:
  1. Store `hardwareInterruptOffset` and `taskManager`.
  2. Get code segment selector from `globalDescriptorTable->CodeSegmentSelector()`.
  3. Initialize all 256 IDT entries to `InterruptIgnore` and clear `handlers[]`.
  4. Install CPU exception handlers for vectors 0x00–0x13.
  5. Install hardware IRQ handlers for:
     - `hardwareInterruptOffset + 0x00` … `+ 0x0F` (PIC IRQ0–IRQ15).
  6. Install handler for `0x80` (syscall interrupt).
  7. Program the PICs (8259A):
     - Send initialization command (`0x11`) to master/slave command ports.
     - Remap:
       - Master base: `hardwareInterruptOffset`.
       - Slave base: `hardwareInterruptOffset + 8`.
     - Set cascade configuration (`0x04` for master, `0x02` for slave).
     - Set mode (`0x01`) for both.
     - Clear masks (`0x00`) to unmask all IRQs (further masking may happen elsewhere).
  8. Load IDT with `lidt`:
     - Build `InterruptDescriptorTablePointer` with:
       - `size = 256 * sizeof(GateDescriptor) - 1`.
       - `base = (uint32_t)interruptDescriptorTable`.

### Activation and deactivation

```cpp
uint16_t InterruptManager::HardwareInterruptOffset();

void InterruptManager::Activate();
void InterruptManager::Deactivate();
```

- `Activate()`:
  - Deactivates any previously active `InterruptManager`.
  - Sets `ActiveInterruptManager = this`.
  - Enables interrupts with `sti`.
- `Deactivate()`:
  - If this is the active manager, sets `ActiveInterruptManager = 0` and disables interrupts with `cli`.

### C entry point for interrupt handling

```cpp
static uint32_t InterruptManager::HandleInterrupt(uint8_t interrupt, uint32_t esp);
uint32_t InterruptManager::DoHandleInterrupt(uint8_t interrupt, uint32_t esp);
```

- `HandleInterrupt(...)` is the static entry used by assembly stubs:
  - If `ActiveInterruptManager` exists, forwards to `DoHandleInterrupt`.
  - Otherwise returns `esp` unchanged.
- `DoHandleInterrupt(...)`:
  1. If `handlers[interrupt]` is non‑null:
     - Call `handlers[interrupt]->HandleInterrupt(esp)` and update `esp` with its return value.
  2. Else, if `interrupt != hardwareInterruptOffset`:
     - Print `"UNHANDLED INTERRUPT 0x00"` followed by the interrupt byte (`printByte(interrupt)`).
  3. If `interrupt == hardwareInterruptOffset` (typically IRQ0 / timer):
     - Call `taskManager->Schedule((CPUState*)esp)` and set `esp` to the returned value (context switch).
  4. Acknowledge hardware interrupts:
     - If `hardwareInterruptOffset <= interrupt < hardwareInterruptOffset + 16`:
       - Send `0x20` to master PIC command port.
       - If `interrupt >= hardwareInterruptOffset + 8`, also send `0x20` to slave PIC command port.
  5. Return possibly updated `esp` to the assembly stub.

### Assembly stubs (`interruptstubs.s`)

- Declares external C++ handler:

```asm
.extern _ZN2os21hardwarecommunication16InterruptManager15HandleInterruptEhj
```

- Macros for exceptions and IRQs:

```asm
.macro HandleException num
  .global _ZN2os21hardwarecommunication16InterruptManager19HandleException\num\()Ev
  _ZN2os21hardwarecommunication16InterruptManager19HandleException\num\()Ev:
      movb $\num, (interruptnumber)
      jmp int_bottom
.endm

.macro HandleInterruptRequest num
  .global _ZN2os21hardwarecommunication16InterruptManager26HandleInterruptRequest\num\()Ev
  _ZN2os21hardwarecommunication16InterruptManager26HandleInterruptRequest\num\()Ev:
      movb $\num + IRQ_BASE, (interruptnumber)
      pushl $0                 # push dummy error code
      jmp int_bottom
.endm
```

- Generates:
  - Exception handlers for 0x00–0x13 and 0x80.
  - IRQ handlers for 0x00–0x0F, 0x31, and 0x80.
- Common bottom (`int_bottom`):

```asm
int_bottom:
    # save registers
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax

    # call C++ handler
    pushl %esp              # push current stack pointer (CPUState*)
    push (interruptnumber)  # push interrupt number
    call _ZN2os21hardwarecommunication16InterruptManager15HandleInterruptEhj
    mov %eax, %esp          # switch to possibly new stack (after scheduling)

    # restore registers
    popl %eax
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp

    add $4, %esp            # skip dummy error code / alignment
```

- `InterruptIgnore` entry:

```asm
.global _ZN2os21hardwarecommunication16InterruptManager15InterruptIgnoreEv
_ZN2os21hardwarecommunication16InterruptManager15InterruptIgnoreEv:
    iret
```

- Static variable:

```asm
.data
    interruptnumber: .byte 0
```

### Invariants

- IDT has 256 entries; all are valid gates pointing to some handler (many to `InterruptIgnore`).
- `hardwareInterruptOffset` must be consistent across:
  - PIC remap.
  - IDT entries for IRQs.
  - PIT and other drivers that compute their IRQ vector via `interrupts->HardwareInterruptOffset()`.
- `InterruptManager::Activate()` must be called after the IDT and PIC are fully configured and any required handlers/InterruptHandler instances have been registered.
- `TaskManager` must be valid if the timer IRQ is used as a scheduling trigger.

---

## Generic Driver Base and Driver Manager (`driver.cc`)

### Components

- `class Driver`
- `class DriverManager`

### Driver

```cpp
class Driver {
 public:
  Driver();
  virtual ~Driver();

  virtual void Activate();
  virtual int Reset();
  virtual void Deactivate();
};
```

- Default implementations:
  - `Activate()` / `Deactivate()` are empty.
  - `Reset()` returns 0.
- Concrete drivers (keyboard, mouse, NIC, etc.) subclass `Driver` and override these methods.

### DriverManager

```cpp
class DriverManager {
 public:
  DriverManager();
  void AddDriver(Driver* drv);
  void ActivateAll();

  Driver* drivers;
  uint8_t numDrivers;
};
```

- Behavior:
  - `AddDriver`:
    - Appends driver pointer to `drivers` and increments `numDrivers`.
  - `ActivateAll`:
    - Iterates from `0` to `numDrivers - 1` and calls `Activate()` on each driver.
- Invariants:
  - `numDrivers` must remain within the bounds of `drivers[]` (caller responsibility).
  - Drivers should be fully constructed and configured before `ActivateAll()` is invoked.

---

## Programmable Interval Timer (PIT) (`timer.cc`)

### Components

- `class ProgrammableIntervalTimer : public InterruptHandler`

### Initialization

```cpp
ProgrammableIntervalTimer::ProgrammableIntervalTimer(
    InterruptManager* interrupts,
    uint32_t frequency)
    : InterruptHandler(interrupts, interrupts->HardwareInterruptOffset() + 0),
      dataPort(0x40),
      commandPort(0x43)
{
  ticks = 0;
  uint32_t internalOscillator = 1193182;
  uint32_t divisor = internalOscillator / frequency;

  commandPort.Write(0x36);   // mode/command byte

  uint8_t divisor_lowByte  = (uint8_t)(divisor & 0xFF);
  uint8_t divisor_highByte = (uint8_t)((divisor >> 8) & 0xFF);

  dataPort.Write(divisor_lowByte);
  dataPort.Write(divisor_highByte);

  printf(GREEN_COLOR, BLACK_COLOR, "[PIT] PIT initalized at %dHz\n", frequency);
}
```

- Registers as an `InterruptHandler` for `interrupts->HardwareInterruptOffset() + 0` (usually IRQ0).
- Programs the PIT (8253/8254) via:
  - Command port `0x43` with `0x36` (mode configuration).
  - Data port `0x40` with low and high bytes of the divisor.
- `ticks` starts at 0 and increments on each timer interrupt.

### Handling interrupts

```cpp
uint32_t ProgrammableIntervalTimer::HandleInterrupt(uint32_t esp) {
  ticks++;
  return esp;
}
```

- Every timer tick increments `ticks`.
- Returns `esp` unchanged (context switching is handled at `InterruptManager` level, not here).

### Busy-wait delay

```cpp
void ProgrammableIntervalTimer::Wait(uint32_t milliseconds) {
  uint32_t ticksToWait = milliseconds;
  uint64_t endTicks = ticks + ticksToWait;

  while (ticks < endTicks) {
    asm volatile("nop");
  }
}
```

- Implements a simple busy‑wait based on the `ticks` counter.
- Assumes:
  - `frequency` in the constructor is effectively “ticks per millisecond” (or you adjust the math accordingly).
- This is blocking and CPU‑intensive:
  - Intended for short delays or debugging.
  - Not suitable for long sleeps in a multitasking environment.

### Invariants

- `ProgrammableIntervalTimer` must be constructed after `InterruptManager` and before `InterruptManager::Activate()`.
- `InterruptManager::HardwareInterruptOffset()` must match the offset used when defining the PIT IRQ in the IDT, or timer interrupts will not be routed correctly.
- `Wait()` depends on interrupts being enabled and PIT firing; if interrupts are disabled, `ticks` will not advance and `Wait()` will spin forever.

---

## Open Questions / TODO

- Document which interrupts are reserved for which subsystems (e.g., timer, keyboard, NIC) to avoid conflicts.
- Define a clear policy for masking/unmasking IRQs at the PIC level vs. in software handlers.
- Replace busy‑wait `Wait()` with a sleep API integrated with the scheduler (e.g., tick‑based wakeup queue).
- Add detailed descriptions of error/exception handling policies for CPU exceptions 0x00–0x13.
- Move any remaining hard‑coded interrupt numbers into a central constants/config header for easier changes.
```
