# Boot and Link

## Responsibility

- Define how the bootloader hands control to the kernel (`loader` → `kernelMain`).
- Describe the kernel’s binary format, memory layout, and segment setup via the linker script and GDT.
- Ensure all subsystems share a consistent view of addresses and CPU mode.

---

## Multiboot Header and Loader (`loader.s`)

### Multiboot header

- Constants:
  - `MAGIC = 0x1badb002`
  - `FLAGS = (1 << 0) | (1 << 1)`
  - `CHECKSUM = -(MAGIC + FLAGS)`
- Section:

```asm
.section .multiboot
    .long MAGIC
    .long FLAGS
    .long CHECKSUM
```

- This forms a valid Multiboot header:
  - `MAGIC` identifies the kernel as Multiboot‑compatible.
  - `FLAGS` request specific Multiboot info (e.g., memory info, aligned modules).
  - `CHECKSUM` makes `MAGIC + FLAGS + CHECKSUM == 0` (required by spec).
- The linker script places `.multiboot` at the start of `.text`, so GRUB/Multiboot will find it before any code.

### Entry symbol and call sequence

- Global entry symbol for the kernel image:

```asm
.section .text
.extern kernelMain
.extern callConstructors
.global loader
```

- `loader` is the ELF entry point (see `ENTRY(loader)` in `linker.ld`).
- Execution flow:

```asm
loader:
    mov $kernel_stack, %esp        # set up kernel stack (top)
    call callConstructors          # run C++ global/static constructors
    push %eax                      # push Multiboot magic (convention)
    push %ebx                      # push pointer to multiboot_info
    call kernelMain                # transfer control to C++ kernel
```

- Register conventions at entry:
  - Multiboot bootloader sets:
    - `%eax` = multiboot magic
    - `%ebx` = pointer to `multiboot_info` struct
  - `loader` forwards these as arguments to `kernelMain` via the stack.
- Stop path:

```asm
_stop:
    cli
    hlt
    jmp _stop
```

- Used as a halt loop if the kernel ever returns to `_stop`.

### Kernel stack

```asm
.section .bss
.space 2*1024*1024      # 2 MiB stack space
kernel_stack:
```

- Reserves 2 MiB in `.bss` for the kernel stack.
- `mov $kernel_stack, %esp` sets `%esp` at the end of this region (growing downward).

---

## Linker Script (`linker.ld`)

### Target and entry

```ld
ENTRY(loader)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)
```

- Kernel is a 32‑bit i386 ELF binary.
- Linker entry symbol is `loader` (matching the assembly label above).

### Memory layout

```ld
SECTIONS
{
  . = 0x0100000;   # load/virtual address: 1 MiB

  .text :
  {
    *(.multiboot)
    *(.text*)
    *(.rodata)
  }

  .data  :
  {
    start_ctors = .;
    KEEP(*( .init_array ));
    KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
    end_ctors = .;

    *(.data)
  }

  .bss  :
  {
    *(.bss)
  }

  /DISCARD/ : { *(.fini_array*) *(.comment) }
}
```

- The link address starts at `0x0010_0000` (1 MiB).
  - Traditional Multiboot‑friendly location; avoids low real‑mode memory.
- Sections:
  - `.text`:
    - `*(.multiboot)` at the very start, so the Multiboot header lives at the beginning of the kernel image.
    - All code `*(.text*)` and read‑only data `*(.rodata)`.
  - `.data`:
    - Defines `start_ctors` and `end_ctors` around `.init_array`:
      - `KEEP(*( .init_array ))`
      - `KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )))`
    - Used by `callConstructors()` in `kernel.cc` to run C++ global constructors.
    - Then places `*(.data)`.
  - `.bss`:
    - All zero‑initialized data `*(.bss)` (including the 2 MiB kernel stack).
  - `/DISCARD/`:
    - Drops `.fini_array*` and `.comment` sections from the final binary.

### C++ constructors interface

- `linker.ld` creates two symbols:
  - `start_ctors` – beginning of `.init_array` region.
  - `end_ctors` – end of `.init_array` region.
- `kernel.cc` provides:

```cpp
typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;

extern "C" void callConstructors() {
  for (constructor* i = &start_ctors; i != &end_ctors; i++)
    (*i)();
}
```

- `loader` calls `callConstructors` before `kernelMain`, so all global/static objects are initialized before kernel code uses them.

---

## Global Descriptor Table (`gdt.cc`)

### Responsibility

- Define and load a basic GDT with:
  - Null descriptor.
  - Unused placeholder descriptor.
  - Code segment descriptor.
  - Data segment descriptor.
- Provide segment selectors to other subsystems (e.g., multitasking, interrupts).

### Layout and construction

```cpp
GlobalDescriptorTable::GlobalDescriptorTable()
    : nullSegmentSelector(0, 0, 0),
      unusedSegmentSelector(0, 0, 0),
      codeSegmentSelector(0, 64 * 1024 * 1024, 0x9A),
      dataSegmentSelector(0, 64 * 1024 * 1024, 0x92)
{
  uint32_t i; [github](https://github.com/pac-ac/osakaOS/actions)
  i = (uint32_t)this; [github](https://github.com/pac-ac/osakaOS)
  i = sizeof(GlobalDescriptorTable) << 16;
  asm volatile("lgdt (%0)" : : "p"(((uint8_t*)i) + 2));
}
```

- Segment descriptors:
  - `nullSegmentSelector` – mandatory null descriptor (selector 0).
  - `unusedSegmentSelector` – reserved/unused descriptor.
  - `codeSegmentSelector`:
    - Base: `0`.
    - Limit: `64 * 1024 * 1024` bytes (64 MiB).
    - Type: `0x9A` (present, ring 0, executable, readable code).
  - `dataSegmentSelector`:
    - Base: `0`.
    - Limit: `64 * 1024 * 1024` bytes.
    - Type: `0x92` (present, ring 0, writable data).
- `lgdt` load:
  - Builds a pseudo‑descriptor `i` with:
    - `i[0]` high word = size of GDT.
    - `i [github](https://github.com/pac-ac/osakaOS/actions)` = base address of the GDT object.
  - Calls `lgdt` to load the GDT register with the address of this table.

### Segment selector accessors

```cpp
uint16_t GlobalDescriptorTable::DataSegmentSelector() {
  return (uint8_t*)&dataSegmentSelector - (uint8_t*)this;
}

uint16_t GlobalDescriptorTable::CodeSegmentSelector() {
  return (uint8_t*)&codeSegmentSelector - (uint8_t*)this;
}
```

- These return the offsets (selector values) of the code and data descriptors within the GDT object.
- Other components (e.g., task state, segment loading code) should use these functions to obtain selectors instead of hard‑coding values.

### SegmentDescriptor encoding

- Constructor:

```cpp
GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type)
{
  uint8_t* target = (uint8_t*)this;

  if (limit <= 65536) {
    target = 0x40;       // 16‑bit segment, byte granularity [github](https://github.com/AlgorithMan-de/wyoos/actions)
  } else {
    // 32‑bit segment with 4 KiB granularity
    if ((limit & 0xFFF) != 0xFFF)
      limit = (limit >> 12) - 1;
    else
      limit = limit >> 12;

    target = 0xC0;       // granularity bit set, 32‑bit segment [github](https://github.com/AlgorithMan-de/wyoos/actions)
  }

  // Limit (lower 16 bits + high 4 bits)
  target = limit & 0xFF;
  target = (limit >> 8) & 0xFF; [github](https://github.com/pac-ac/osakaOS)
  target [github](https://github.com/AlgorithMan-de/wyoos/actions) |= (limit >> 16) & 0xF;

  // Base (32 bits)
  target = base & 0xFF; [github](https://github.com/pac-ac/osakaOS/actions)
  target = (base >> 8) & 0xFF; [github](https://github.com/pac-ac/osakaOS/issues)
  target = (base >> 16) & 0xFF; [github](https://github.com/pac-ac/osakaOS/activity)
  target = (base >> 24) & 0xFF; [github](https://github.com/pac-ac/osakaOS/issues/19)

  // Type / access byte
  target = type; [github](https://github.com/pac-ac/osakaOS/blob/main/Makefile)
}
```

- Limit handling:
  - If `limit <= 65536`, segment uses byte granularity (16‑bit style).
  - Else:
    - Uses 4 KiB granularity (`G=1`).
    - Limit is shifted by 12 bits; if low bits are not all 1, it adjusts to keep the effective limit correct and within physical constraints.

- Helper methods:

```cpp
uint32_t GlobalDescriptorTable::SegmentDescriptor::Base();
uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit();
```

- `Base()` reconstructs base address from descriptor bytes.
- `Limit()` reconstructs limit and re‑applies granularity (if `G=1`, multiplies back by 4 KiB and or’s with `0xFFF`).

---

## Invariants and Assumptions

- CPU boots under a Multiboot‑compliant bootloader (e.g., GRUB) that:
  - Loads the kernel ELF at or compatible with the link address `0x0010_0000`.
  - Provides `multiboot_info` and magic values in `%ebx` and `%eax` before jumping to `loader`.
- `loader`:
  - Must be the first executed code in the kernel image.
  - Must not use C/C++ runtime features before `callConstructors`.
- Stack:
  - Kernel stack is a 2 MiB `.bss` region starting at `kernel_stack`, growing downward.
  - No stack guard or detection yet; overflow is undefined behavior.
- GDT:
  - Assumes flat 0‑based memory with code and data segments spanning at least 64 MiB.
  - Other subsystems rely on `CodeSegmentSelector()` and `DataSegmentSelector()` values instead of hard‑coded selectors.
- C++:
  - All global/static objects that need initialization must have their constructors placed into `.init_array`; `callConstructors()` must be called once before `kernelMain`.

---

## Open Questions / TODO

- Decide whether to explicitly document the transition into protected mode (currently assumed done by the bootloader) or add a protected‑mode setup stub.
- Confirm and document the exact physical vs virtual mapping strategy around `0x0010_0000` (tie this to `memory.md`).
- Consider adding a small diagram for memory layout showing:
  - bootloader, kernel `.text/.data/.bss`, stack, heap start (10 MiB), etc.
- If segmentation is later minimized (pure paging model), clarify which parts of GDT remain relevant (e.g., just flat code/data).
```
