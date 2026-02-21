# Kernel

## Responsibility

- Own top‑level OS initialization and main control flow.
- Wire together core subsystems: boot info, memory, GDT, interrupts, multitasking, drivers, network stack, CLI, and (optionally) GUI.
- Provide the main event loop and CPU idle behavior.

## Entry and Control Flow

### Entry point

- External C entry: `extern "C" void kernelMain(const void* multiboot_structure, uint32_t multiboot_magic)`.
- Called by the bootloader via `loader.s` / `linker.ld` following the Multiboot convention.

### High‑level initialization sequence

Rough order inside `kernelMain`:

1. **Early console / terminal**
   - Construct `Terminal terminal;`.
   - Use `printf(...)` and `putChar(...)` for early output (assumes terminal + VGA work sufficiently early).

2. **GDT**
   - Construct `GlobalDescriptorTable gdt;`.

3. **Heap / memory manager**
   - Read `memupper` from the Multiboot info at `multiboot_structure + 8` (upper memory in KiB).
   - Set `heapStart = 10 * 1024 * 1024` (10 MiB).
   - Set `padding = 10 * 1024` (10 KiB).
   - Compute:
     - `heapSize = (*memupper) * 1024 - heapStart - padding`.
   - Construct `MemoryManager heap(heapStart, heapSize);`.
   - This defines the kernel heap region above 10 MiB, up to just below the upper memory limit minus padding.

4. **Multitasking**
   - Construct `TaskManager taskManager;`.
   - Construct test tasks `Task task1(&gdt, taskA);`, `Task task2(&gdt, taskB);`.
   - Currently, the calls `taskManager.AddTask(...)` are commented out (tasks exist but are not scheduled by default).

5. **Interrupts and syscalls**
   - Construct `InterruptManager interrupts(0x20, &gdt, &taskManager);`.
     - Interrupt vector base is `0x20`.
   - Construct `SyscallHandler syscalls(&interrupts, 0x80);`.
     - Syscall interrupt vector is `0x80`.

6. **Optional GUI desktop (GRAPHICSMODE)**
   - If `GRAPHICSMODE` is defined:
     - Construct `Desktop desktop(320, 200, 0xA8, 0x00, 0x00);` (basic 320x200 desktop).

7. **Driver manager and shell**
   - Construct `DriverManager drvManager;`.
   - Construct `Shell shell;`.
   - Construct `CommandRegistry commandRegistry;`.

8. **Timer**
   - Construct `ProgrammableIntervalTimer timer(&interrupts, 100);`.
     - PIT frequency set to 100 Hz (tick = 10 ms).
   - Store `uint64_t startTicks = timer.ticks;` (tick count from boot at this point).

9. **Mouse driver**
   - If `GRAPHICSMODE`:
     - `MouseDriver mouse(&interrupts, &desktop);` (mouse events go to GUI desktop).
   - Else:
     - `MouseToConsole mousehandler;` (local helper class in `kernel.cc` drawing a cursor in text mode).
     - `MouseDriver mouse(&interrupts, &mousehandler);`.
   - Register with driver manager: `drvManager.AddDriver(&mouse);`.

10. **Keyboard driver**
    - If `GRAPHICSMODE`:
      - `KeyboardDriver keyboard(&interrupts, &desktop);`.
    - Else:
      - `KeyboardDriver keyboard(&interrupts, &shell);` (CLI receives keyboard events).
    - Register with driver manager: `drvManager.AddDriver(&keyboard);`.

11. **PCI controller and device enumeration**
    - Construct `PeripheralComponentInterconnectController PCIController;`.
    - Call `PCIController.SelectDrivers(&drvManager, &interrupts);` to let PCI create device drivers and add them to `drvManager`.
    - `shell.SetPCI(...)` is currently commented out in `kernel.cc` (PCI is injected via `CommandRegistry` instead).

12. **VGA**
    - Construct `VideoGraphicsArray vga;`.
    - Used by GUI when `GRAPHICSMODE` is enabled.

13. **Driver activation**
    - `drvManager.ActivateAll();` to initialize all registered drivers (mouse, keyboard, NIC, etc.).

14. **PIC masking**
    - Manually adjust PIC2 mask via port `0xA1`:
      - Read existing mask.
      - `mask |= 0xC0;`
      - Write back to mask register.
    - This masks specific IRQ lines on the secondary PIC (e.g., to control spurious/unused interrupts).

15. **Network stack (conditional)**
    - Only compiled when `NETWORK` is defined.
    - Identify `eth0`:
      - Iterate `drvManager.drivers`.
      - Pick the first driver that is not `mouse` or `keyboard`, cast to `amd_am79c973*`.
      - If `eth0` is still `0`, kernel prints an error and halts in an infinite loop.
      - A second pass sets `eth0` to `drvManager.drivers[2]` if available (debug / fallback).
    - On success:
      - Print `"Welcome to DracOS Network"`.
      - Configure addresses:
        - IP: `10.0.2.15`.
        - Gateway: `10.0.2.2`.
        - Subnet mask: `255.255.255.0`.
      - Convert these to big‑endian `uint32_t` values.
      - `eth0->SetIPAddress(ip_BE);`.
      - Construct network stack:
        - `EtherFrameProvider etherframe(eth0);`
        - `AddressResolutionProtocol arp(&etherframe);`
        - `InternetProtocolProvider ipv4(&etherframe, &arp, gip_BE, subnet_BE);`
        - `InternetControlMessageProtocol icmp(&ipv4);`
      - Example operations (currently mostly commented out):
        - `arp.BroadcastMACAddress(gip_BE);`
        - `icmp.Ping(gip_BE);`
        - Sending IPv4 payloads via `ipv4.Send(...)`.

16. **GUI setup (conditional)**
    - If `GRAPHICSMODE`:
      - `vga.SetMode(320, 200, 8);` (320x200x256 mode).
      - Create windows:
        - `Window win1(&desktop, 10, 10, 20, 20, 0x00, 0x00, 0xA8);`
        - `Window win2(&win1, 30, 40, 30, 30, 0x00, 0xA8, 0x00);`
      - Attach windows to `desktop`.

17. **CLI and dependency injection**
    - Uses `CommandRegistry` as a DI container:
      - System dependencies:
        - `commandRegistry.InjectDependency("SYS.SHELL", &shell);`
        - `commandRegistry.InjectDependency("SYS.PCI", &PCIController);`
        - `commandRegistry.InjectDependency("SYS.HEAP", &heap);`
        - `commandRegistry.InjectDependency("SYS.TERMINAL", &terminal);`
      - Network dependencies:
        - `commandRegistry.InjectDependency("NET.ARP", &arp);`
        - `commandRegistry.InjectDependency("NET.IPV4", &ipv4);`
        - `commandRegistry.InjectDependency("NET.ICMP", &icmp);`
      - Process dependencies:
        - `commandRegistry.InjectDependency("PROC.TASKMANAGER", &taskManager);`
      - Filesystem dependencies:
        - (currently placeholder, none injected).
      - Validate:
        - `commandRegistry.ValidateAllDependencies();`.
    - The older API `setSystemCmdDependencies(...)` / `setNetworkCmdDependencies(...)` is commented out.

18. **Interrupt activation**
    - `interrupts.Activate();` is called **after** all subsystems and drivers are initialized and registered.

19. **Network test (example)**
    - Allocates test payload `"7777777"` and has commented‑out ARP + IPv4 send code.
    - `arp.BroadcastMACAddress(gip_BE);` is currently executed.

20. **Shell startup and main loop**
    - `shell.PrintPrompt();`
    - Main loop:

      ```cpp
      while (1) {
        asm volatile("hlt");
        #ifdef GRAPHICSMODE
          desktop.Draw(&vga);
        #endif
      }
      ```

    - CPU idles with `hlt` between interrupts, avoiding busy‑wait.
    - In graphics mode, the desktop is drawn each time execution resumes after `hlt`.

## Multitasking Test Tasks

- `taskA` and `taskB` are test tasks defined in `kernel.cc`:
  - Both call `sysprintf("A")` or `sysprintf("B")` in infinite loops.
  - Their registration in `TaskManager` is currently commented out.
- Intended as a simple concurrency test using the syscall interface.

## HashMap Test (Debug / Diagnostics)

- `TestHashTable()` is a kernel‑level test function for `os::utils::ds::HashMap<const char*, int>`.
- It exercises:
  - Basic insertion and lookup.
  - Updating an existing key.
  - Collision handling and retrieval of multiple keys/values/pairs via `LinkedList`.
  - Handling of missing keys.
- This test is currently commented out in `kernelMain` but can be enabled for debugging.

## Input Event Handlers in kernel.cc

- `PrintfKeyboardEventHandler`:
  - Simple `KeyboardEventHandler` that prints each key via `putChar(c)`.
  - Currently **not** wired; keyboard events are sent to `Shell` in text mode.
- `MouseToConsole`:
  - `MouseEventHandler` for text mode to visualize mouse movement.
  - Maintains `(x, y)` position in 80x25 text space.
  - On activation and move:
    - Inverts the color of the current cell by swapping foreground/background bits in VGA text memory at `0xB8000`.
  - Used as the mouse handler when `GRAPHICSMODE` is **not** defined.

## Invariants and Assumptions

- Bootloader passes a valid Multiboot info struct at `multiboot_structure`, and `memupper` is at offset `+8` and correctly describes available memory above 1 MiB in KiB.
- Heap:
  - Begins at 10 MiB (`0x00A00000`).
  - Ends at `heapStart + heapSize`, where `heapSize = memupper_bytes - heapStart - padding`.
  - Assumes sufficient physical memory so `heapSize` is positive and large enough for kernel allocations.
- Single‑core x86 environment (no explicit SMP handling).
- Interrupts are disabled during most initialization and only enabled after:
  - GDT, memory manager, interrupt manager, drivers, CLI, and network are set up.
- CLI and GUI:
  - In text mode, `Shell` is the keyboard event handler.
  - In graphics mode, `Desktop` is both keyboard and mouse handler.
- Network:
  - Assumes a single AMD am79c973 NIC that is detected by PCI and registered in `DriverManager`.
  - IP, gateway, and subnet mask are statically configured at kernel init.
- `hlt` main loop:
  - Kernel relies on interrupts (timer, input, NIC, etc.) to resume execution.

## Open Questions / TODO

- Solidify a canonical initialization order document (cross‑reference `boot_and_link.md`, `hardware_abstraction.md`, `memory.md`, `multitasking.md`, `drivers_*`, `cli_shell.md`, `gui.md`).
- Move test tasks (`taskA`, `taskB`) and `TestHashTable()` into a dedicated `tests/` or `diagnostics/` subsystem.
- Replace ad‑hoc NIC discovery (skipping mouse/keyboard, index 2 fallback) with a more explicit “network device selection” strategy.
- Decide on long‑term policy for:
  - When to enable interrupts relative to driver initialization.
  - How to transition from “kernel brings everything up” to a more modular service/daemon model.
- Clarify and document error handling paths (e.g., what to do when `eth0` is not found, beyond halting).

