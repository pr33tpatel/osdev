# Development Guide

This document explains how to build, run, and work on DracOS: directory layout, toolchain, Makefile targets, QEMU configuration, and code style.

> **Safety:** DracOS (this software) is intended to be run under emulation (e.g., `qmeu-system-i386`) only.
> Booting this kernel on real hardware is not supported or tested and may corrupt data or leave your machine in an undefined state.
> Use this software at your own risk. See [License, Disclaimer, Safety](license-disclaimer-safety) for more information.

## Project layout

See the other docs for detailed subsystem design:

- [Kernel](kernel.md)
- [Memory management](memorymanagement.md)
- [Storage / ATA](storage.md)
- [CLI and shell](cli.md)
- [Utils library](utils.md)
- [Data structures](ds.md)

The high-level tree (simplified):

- `src/` – kernel and driver sources (`.cc`, `.s`, `linker.ld`).
- `include/` – public headers.
- `obj/` – build artifacts (object files), created by the Makefile.
- `docs/` – documentation (`*.md`).
- `mykernel.bin` – linked kernel binary.
- `mykernel.iso` / `myos.iso` – bootable ISO images.
- `Image.img` – raw disk image used by QEMU.
- `.clang-format` – C++ formatting rules.

---

## Toolchain and dependencies

You need a 32‑bit capable C++ toolchain and QEMU.
Note: All development and testing is done on Linux (Debian/Ubuntu). Other UNIX-like systems may work but are not officially supported or documented.

### Required tools

On a typical Linux system:

- `g++` – C++ compiler with 32‑bit support (used as `CC`).
- `as` – GNU assembler (used as `AS`).
- `ld` – GNU linker (used as `LD`).
- `qemu-system-i386` – emulator for running the OS.
- `qemu-img` – to create the raw disk image.
- `grub-mkrescue` – to create bootable ISO images.
- A 32‑bit libc/dev package so `g++ -m32` works (e.g. `libc6-dev-i386` on Debian/Ubuntu).

Example (Debian/Ubuntu):

```bash
sudo apt-get install g++ binutils qemu-system-x86 qemu-utils grub-pc-bin xorriso libc6-dev-i386
```

### Compiler and linker flags

From the `Makefile`:

```make
CC      = g++
AS      = as
LD      = ld

CFLAGS  = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions \
          -Wno-write-strings -Iinclude

ASFLAGS = --32
LDFLAGS = -melf_i386
```

Key points:

- `-m32` – build 32‑bit code.
- `-nostdlib`, `-fno-builtin` – no host C/C++ runtime; we provide our own runtime pieces.
- `-fno-rtti`, `-fno-exceptions` – keep the kernel C++ subset simple (no RTTI/exceptions).
- `-Iinclude` – include path for DracOS headers.
- `-melf_i386` – link as 32‑bit ELF.

---

## Build system (Makefile)

The root `Makefile` orchestrates the whole build.

### Object list

All compiled units go into `obj/`:

```make
OBJECTS = obj/loader.o \
          obj/gdt.o \
          obj/memorymanagement.o \
          obj/drivers/driver.o \
          obj/hardwarecommunication/port.o \
          obj/hardwarecommunication/interruptstubs.o \
          obj/hardwarecommunication/interrupts.o \
          obj/drivers/timer.o \
          obj/syscalls.o \
          obj/multitasking.o \
          obj/drivers/amd_am79c973.o \
          obj/hardwarecommunication/pci.o \
          obj/drivers/keyboard.o \
          obj/drivers/mouse.o \
          obj/drivers/terminal.o \
          obj/drivers/vga.o \
          obj/drivers/ata.o \
          obj/gui/widget.o \
          obj/gui/window.o \
          obj/gui/desktop.o \
          obj/net/etherframe.o \
          obj/net/arp.o \
          obj/net/ipv4.o \
          obj/net/icmp.o \
          obj/utils/print.o \
          obj/utils/string.o \
          obj/utils/math.o \
          obj/cli/shell.o \
          obj/cli/commandregistry.o \
          obj/cli/commands/networkCmds.o \
          obj/kernel.o
```

### Pattern rules

Compile C++ and assembly into `obj/` with matching subdirectories:

```make
obj/%.o: src/%.cc
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.s
	mkdir -p $(@D)
	$(AS) $(ASFLAGS) -c $< -o $@
```

- `$<` – input source file.
- `$@` – output object file.
- `$(@D)` – directory portion of the target (ensures nested `obj/...` exists).

### Kernel binary

Link all objects into the kernel image using `linker.ld`:

```make
mykernel.bin: src/linker.ld $(OBJECTS)
	$(LD) $(LDFLAGS) -T src/linker.ld -o $@ $(OBJECTS)
```

`all` defaults to building `mykernel.bin`:

```make
all: mykernel.bin
```

---

## Running under QEMU

There are two main flows: run from an ISO, or directly use the kernel binary.

### 1. ISO + disk image (recommended for full boot flow)

Targets:

- `mykernel.iso` – build bootable ISO.
- `Image.img` – create an empty 128 MiB disk.
- `run` – start QEMU with both.

#### Build ISO

```make
mykernel.iso: mykernel.bin
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp mykernel.bin iso/boot/mykernel.bin
	echo 'set timeout=0'                      > iso/boot/grub/grub.cfg
	echo 'set default=0'                     >> iso/boot/grub/grub.cfg
	echo ''                                  >> iso/boot/grub/grub.cfg
	echo 'menuentry "My Operating System" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/mykernel.bin'    >> iso/boot/grub/grub.cfg
	echo '  boot'                            >> iso/boot/grub/grub.cfg
	echo '}'                                 >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=mykernel.iso iso
	rm -rf iso
```

- Uses GRUB and a small `grub.cfg` to boot `mykernel.bin` as a Multiboot kernel.

#### Create disk image

```make
Image.img:
	qemu-img create -f raw Image.img 128M
```

#### Run

```make
run: mykernel.iso Image.img
	qemu-system-i386 \
		-boot d \
		-cdrom mykernel.iso \
		-m 512 \
		-smp 1 \
		-net nic,model=pcnet -net user \
		-drive id=disk,file=Image.img,format=raw,if=ide,index=0 \
		-vga qxl
```

- Boot from CD (`-boot d`) using `mykernel.iso`.
- 512 MiB RAM, 1 vCPU.
- Network:
  - `pcnet` NIC model (for the AMD am79c973 driver).
  - User-mode networking for simple outbound connectivity.
- Storage:
  - `Image.img` attached as an IDE disk at index 0.
- Video:
  - QXL VGA.

### 2. Direct kernel run (fast debug loop)

```make
kernel-debug: mykernel.bin
	qemu-system-i386 -kernel mykernel.bin -no-reboot -no-shutdown -serial stdio -d cpu,int
```

- Boots `mykernel.bin` directly (bypassing GRUB/ISO).
- Useful for quick iteration.
- `-serial stdio` – redirect guest serial output to host terminal.
- `-d cpu,int` – enable QEMU debug logging for CPU and interrupts (noisy, but useful).

### Alternate ISO flow

There’s an additional `iso` / `run-iso` pair using an external `grub.cfg`:

```make
iso: mykernel.bin grub.cfg
	mkdir -p iso/boot/grub
	cp mykernel.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o myos.iso iso

run-iso: iso
	qemu-system-i386 -cdrom myos.iso
```

Use this if you want a more customized `grub.cfg` (kept in the repo).

---

## Installation to host /boot (optional)

```make
install: mykernel.bin
	sudo cp $< /boot/mykernel.bin
```

- Copies `mykernel.bin` into the host’s `/boot` directory.
- Only needed if you want to chain-load via your actual GRUB; not required for QEMU development.

---

## Cleaning and rebuilding

- Remove build artifacts and images:

  ```bash
  make clean
  ```

  This removes `obj/`, `mykernel.bin`, `mykernel.iso`, and `Image.img`.

- Remove only objects:

  ```bash
  make clean-objects
  ```

  This deletes `obj/` but keeps `mykernel.bin` and images.

- Typical dev loop:

  ```bash
  make clean-objects   # optional
  make                 # builds mykernel.bin
  make mykernel.iso    # or let `make run` build it
  make run             # starts QEMU
  ```

---

## Code style and formatting

The project uses `.clang-format` with a **Google-based** C++ style and a few customizations.

Key settings (see `.clang-format`):

- Base:

  ```yaml
  Language: Cpp
  BasedOnStyle: Google
  ```

- Indentation:

  ```yaml
  IndentWidth: 2
  TabWidth: 2
  UseTab: Never
  ```

- Pointer alignment:

  ```yaml
  PointerAlignment: Left   # e.g., `char* ptr`
  DerivePointerAlignment: false
  ```

- Line length:

  ```yaml
  ColumnLimit: 105
  ```

  - Increased from 80 to 105 because namespaces and kernel paths are long.

- Includes:

  ```yaml
  # SortIncludes: true
  # IncludeBlocks: Preserve
  ```

  - Include blocks are preserved (e.g., drivers vs utils separated by blank lines).

- Function and parameter layout:

  ```yaml
  BinPackArguments: false
  BinPackParameters: false
  AlignAfterOpenBracket: BlockIndent
  PenaltyBreakBeforeFirstCallParameter: 70
  AllowShortFunctionsOnASingleLine: Empty
  ```

- Access modifiers and braces:

  ```yaml
  AccessModifierOffset: -1
  BreakBeforeBraces: Attach
  ```

- Spacing:

  ```yaml
  MaxEmptyLinesToKeep: 2
  ```

### Using clang-format

Typical usage:

```bash
# Format a single file
clang-format -i src/kernel.cc

# Format all .cc and .h files (simple example)
find src include -name '*.[ch]pp' -o -name '*.cc' -o -name '*.h' | xargs clang-format -i
```

Or set up your editor (VS Code, CLion, etc.) to run `clang-format` on save, using the project’s `.clang-format` file.

---

## Summary for contributors

- Install the listed toolchain (32‑bit g++, binutils, QEMU, GRUB tools).
- Use `make` to build:
  - `make` → `mykernel.bin`
  - `make mykernel.iso` → ISO
  - `make run` → run with QEMU and a raw disk image.
- For quick iteration, use `make kernel-debug`.
- Keep code formatted using `.clang-format`.
- See the other `docs/*.md` for subsystem-specific internals.

---

## License, Disclaimer, Safety

DracOS (this software) is provided for educational and research purposes only, without warranty of any kind.  
Running custom kernels, bootloaders, or disk images always carries risk; you are responsible for how you build, run, and distribute this code on your own machines and hardware.

IT IS STRONGLY ADVISED TO RUN THIS OPERATING SYSTEM ONLY UNDER EMULATION (E.G., QEMU).  
Booting this software on real hardware is not supported and has not been tested. Doing so may corrupt data or leave your machine in an undefined state, and you are solely responsible for any damage or loss resulting from any interaction with this software.

---

## Credits

Parts of the project are inspired by and derived from
 ["Write Your Own Operating System" (WYOOS)](https://wyoos.org/) guide and its accompanying source code.  
That source code remains under its original license; this repository does not claim ownership of it. Where code has been copied or closely adapted, it is kept under the original license terms.

This project also references [wiki.osdev.org](https://wiki.osdev.org/) for certain implementations and code examples.
