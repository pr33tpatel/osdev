# CLI and Shell

## Status

The CLI is under active development and refactoring.  
This document describes the current design and behavior so that future threads can safely extend or change it and then update this doc.

---

## High‑Level Design

- **Shell (`Shell`)**
  - Owns the input buffer, cursor state, and prompt behavior.
  - Uses a command table implemented as `HashMap<const char*, Command*>` to map command names to their implementation logic handlers.
  - Implements a keyboard handler interface via `OnKeyDown(char c)`.
  - Parses user input into command name + arguments and dispatches to matching `Command` objects.

- **Command abstraction (`Command`, `FlagOption`)**
  - `Command` is the base class for all CLI commands; each instance has a `const char* name` and an `execute(char* args)` method.
  - `FlagOption`, `ParseFlags`, and `PrintFlags` provide simple flag parsing and help output for commands that support flags (e.g. `ping`).

- **Command registry / dependency injection (`CommandRegistry`)**
  - Stores named subsystem dependencies (e.g., `"SYS.SHELL"`, `"SYS.TERMINAL"`, `"NET.IPV4"`, `"NET.ICMP"`).
  - Validates that required dependencies exist before enabling specific command groups (system, network, process, filesystem).
  - Constructs concrete commands using those dependencies and registers them with the shell.

- **Concrete commands**
  - System commands (e.g., `whoami`, `echo`, `clear`) for local shell/system behavior.
  - Network commands (e.g., `ping`) which use the network stack via injected subsystems.

---

## Shell

### Responsibilities

- Capture and edit the current input line in a fixed‑size `commandbuffer`.
- Render a prompt (currently `"& "`) and echo typed characters through the active `Terminal`.
- Handle basic cursor movement and scrolling via arrow keys and scroll commands.
- Parse the completed line into command name and argument string.
- Use the command table (`HashMap<const char*, Command*>`) to look up and execute registered commands.

### Lifecycle

- A single `Shell` instance is constructed in `kernelMain`.
- In text mode, it is registered as the keyboard event handler:

  - `KeyboardDriver keyboard(&interrupts, &shell);` (see `kernel.md` and keyboard driver docs).

- The shell initializes itself by printing the prompt once on startup.

### Command registration

- The shell exposes `RegisterCommand(Command* cmd)` for registering commands.
- Registration behavior:
  - Uses `cmd->name` as the key into the `HashMap<const char*, Command*>`.
  - If a command with the same name already exists, the shell prints a warning and overwrites the existing entry.
  - Otherwise, it inserts a new mapping from name → command pointer.
- Command names are expected to be stable C‑strings (typically static string literals such as `"ping"` or `"whoami"`).

At the moment, the shell does not delete registered commands; commands allocated with `new` are effectively permanent for the lifetime of the shell.  
**TODO:** define a clear ownership/cleanup strategy if commands become dynamically added/removed in future threads.

### Command execution

When the user presses Enter:

1. The shell terminates `commandbuffer` with a null byte and calls `ExecuteCommand()`.
2. `ExecuteCommand`:
   - Uses `strtok` on `commandbuffer` with space as a delimiter to extract the first token as the command name.
   - Treats everything after the first token (skipping any leading spaces) as the `args` string.
   - Looks up the command in the `commandMap` hashmap using the parsed name.
   - If found, calls `cmd->execute(args)`.
   - If no command exists for that name, prints a message of the form `"\"<name>\" not in Command Registry"`. 
3. Because `strtok` modifies `commandbuffer`, the buffer is considered consumed for that input line; implementations of `Command::execute` are responsible for any further tokenization of `args` they require.

The lookup is now O(1) on average thanks to `HashMap`, instead of a linear scan through an array.

### Prompt and buffer utilities

- `PrintPrompt`:
  - Prints the current prompt (currently a static `"& "`) in a fixed color scheme.
- `PrintPreviousCmd`:
  - Can be used to echo back the last command line; currently not used in normal flow.
- `ShellInit`:
  - Responsible for printing the initial prompt.
- `fillCommandBuffer`:
  - Test/debug helper that fills the input buffer with a given character and adjusts `bufferIndex` accordingly.
  - The modulo logic may be revisited in future refactors.

### Keyboard handling and line editing

`Shell::OnKeyDown(char c)` is the main entry point for keyboard events.

- If there is no active terminal (`Terminal::activeTerminal == 0`), the shell ignores all input.
- Cursor navigation and scrolling:
  - Arrow keys move the cursor within the terminal.
  - Shift + arrow up/down scrolls the terminal buffer up or down.
  - `cursorIndex` tracks the logical cursor position in `commandbuffer`, but insertion/overwrite editing is not implemented yet; editing behaves like an append‑only line editor.
- History (placeholder):
  - `SHIFT_ARROW_LEFT` currently just prints `"history"`; a real history mechanism is planned but not implemented.
- Enter and backspace:
  - On Enter:
    - Appends a newline to the terminal.
    - If the buffer has content, null‑terminates `commandbuffer`, calls `ExecuteCommand`, resets `bufferIndex`, and prints a new prompt.
  - On Backspace:
    - Only acts if `bufferIndex > 0`.
    - Removes the last character from both the terminal display and `commandbuffer` and decrements `bufferIndex`.
- Normal character input:
  - Appends printable characters to `commandbuffer` and echoes them to the terminal.
  - Currently there is no bounds checking on `bufferIndex` relative to the size of `commandbuffer`; a future safety refactor should add this.

---

## Command Abstraction

### Command base class

- `Command` is the abstract base class for all CLI commands:

  - Each command has a `const char* name`.
  - Each command implements `void execute(char* args)` which is invoked by the shell.
  - `name` is used as the lookup key in the shell’s `HashMap<const char*, Command*>`.

- Implementations are responsible for:
  - Parsing `args` as needed (e.g., via `strtok` or custom parsing).
  - Handling error conditions (missing required arguments, invalid flags, etc.).
  - Producing user‑visible output via the utils `printf` and related functions.

### Flag utilities

- `FlagOption` ties a flag name to a boolean field and a description:

  - `name`: the flag string (e.g., `"-h"`, `"-v"`).
  - `description`: human‑readable explanation printed in help.
  - `value`: pointer to a `bool` that will be set when the flag is present.

- `ParseFlags(current_arg, flags, flagCount)`:
  - Compares `current_arg` against each `flags[i].name` using `os::utils::strcmp`.
  - When a match is found, sets `*(flags[i].value) = true`.
- `PrintFlags(flags, flagCount, fgColor, bgColor)`:
  - Prints all flags and descriptions in a simple table, using the given VGA colors (defaults to yellow on black).

These helpers are used by commands like `ping` to implement `-h`, `-v`, `-d`, and similar options.

---

## CommandRegistry and Dependency Injection

### Role

- Acts as a simple, global dependency injection container for CLI commands and other subsystems.
- Stores subsystem instances under string keys (e.g., `"SYS.SHELL"`, `"SYS.PCI"`, `"SYS.HEAP"`, `"SYS.TERMINAL"`, `"NET.ARP"`, `"NET.IPV4"`, `"NET.ICMP"`, `"PROC.TASKMANAGER"`, `"FS.INIT"`).
- Provides validation routines to ensure required dependencies exist before registering groups of commands (system, network, process, filesystem).

### Structure and data

- Internally maintains a `HashMap<const char*, DependencyEntry>` where:
  - `DependencyEntry` contains:
    - `void* ptr` – pointer to the subsystem instance.
    - `bool isValid` – whether this dependency is ready for use.
- Keys are C‑string names; the `HashMap` uses the utils `Hasher<const char*>` specialization to hash by string contents and compare names via `strcmp`.

### Injecting and getting dependencies

- `InjectDependency(const char* depName, void* depPtr)`:
  - Constructs a `DependencyEntry` with `ptr = depPtr` and `isValid = (depPtr != 0)`.
  - Inserts it into the hashmap via `dependencyMap.Insert(depName, depEntry)`.
  - Typically called from `kernelMain` after subsystems are constructed.
- `GetDependency(const char* depName)`:
  - Looks up `depName` in the hashmap with `dependencyMap.Get(depName, depEntry)`.
  - If present and `depEntry.isValid` is true, returns `depEntry.ptr`; otherwise returns `0`.
- Typical injections include:
  - `"SYS.SHELL"` → the `Shell` instance.
  - `"SYS.TERMINAL"` → active `Terminal` or terminal subsystem.
  - `"NET.ARP"`, `"NET.IPV4"`, `"NET.ICMP"` → network stack components.
  - `"PROC.TASKMANAGER"` → task manager.
  - `"FS.INIT"` → filesystem initialization/entry point.

### Validation

- `ValidateGroup(requiredDeps, count)`:
  - For each `depName` in `requiredDeps`:
    - Queries `dependencyMap`.
    - If missing or invalid, prints `[ERROR] MISSING DEPENDENCY: <name>` and marks the group as invalid.
    - If present and valid, prints `[SHELL] LOADED DEPENDENCY: <name>` (this logging can be dialed down if too noisy).
- Specific validation helpers:
  - `ValidateSystemDependencies()` – checks `"SYS.SHELL"`, `"SYS.PCI"`, `"SYS.HEAP"`, `"SYS.TERMINAL"`.
  - `ValidateNetworkDependencies()` – checks `"NET.ARP"`, `"NET.IPV4"`, `"NET.ICMP"`.
  - `ValidateProcessDependencies()` – checks `"PROC.TASKMANAGER"`.
  - `ValidateFileSystemDependencies()` – checks `"FS.INIT"`.
- `ValidateAllDependencies()`:
  - Prints a header `Validating DracOS Dependencies...`.
  - Calls each group validator.
  - Returns the bitwise AND of all four results to ensure all groups are evaluated even if some fail.

These validation routines are typically called once during kernel initialization after all `InjectDependency` calls.

### Command registration via CommandRegistry

- For each command group (system, network, process, filesystem), the registry exposes `RegisterSystemCommands()`, `RegisterNetworkCommands()`, etc.
- Each registration method:
  1. Validates its dependency group.
  2. Uses `GetDependency("SYS.SHELL")` to fetch the shell and other group‑specific subsystems (e.g., `"NET.ICMP"` for network commands).
  3. Constructs concrete `Command` objects (currently using `new`) that depend on those subsystems.

- Example (network overview):
  - Requires `"NET.ARP"`, `"NET.IPV4"`, `"NET.ICMP"` and `"SYS.SHELL"`.
  - After validation and lookups:
    - Creates a `Ping` command bound to the ICMP provider.
    - Calls `shell->RegisterCommand(new Ping(icmp))`.
  - This makes the `ping` command available in the CLI once the network stack is ready.

**Note:** Commands allocated with `new` through the registry are treated as permanent. There is currently no mechanism to unregister or free them.

---

## Concrete Commands

### Network: `ping`

File: `cli/commands/networkCmds.{h,cc}`

- `Ping` is a `Command` that sends ICMP echo requests using the injected `InternetControlMessageProtocol` instance.
- Behavior:
  - Parses the argument line into a small `argv`‑style array (up to 255 entries), with `argv[0]` being `"ping"` and subsequent entries coming from `args`.
  - Supports flags:
    - `-h` – display usage and flag descriptions.
    - `-v` – (planned) display packet contents.
    - `-d` – debug mode; prints parsed tokens and intermediate values.
    - `-hex` – (planned) display IPs in hexadecimal.
  - Distinguishes between flags and the main IP argument:
    - Flags start with `'-'` and are processed via `ParseFlags`.
    - The first non‑flag argument is interpreted as the target IP string.
  - When an IP string is present:
    - Splits on `'.'` into four octets using `strtok`.
    - Converts each octet to an integer using `strToInt`.
    - Combines them into a 32‑bit big‑endian IPv4 address using `convertToBigEndian`.
    - Prints a “Pinging a.b.c.d...” line.
    - Calls `icmp->Ping(targetIP_BE)` to send the echo request.
  - When no IP is provided:
    - Prints a usage string and available flags (via `PrintFlags`).
- `Ping` relies on the CLI string and math utilities (`strtok`, `strToInt`, `convertToBigEndian`) and the print utilities for colored output.

### System: `whoami`, `echo`, `clear`

File: `cli/commands/systemCmds.{h,cc}`

- Current state (to be refactored):
  - `whoami`, `echo`, and `clear` still perform their behavior in constructors rather than in `execute`, which is not aligned with the design.
  - Intended behavior:
    - `whoami` should print the effective user name when executed.
    - `echo` should print its arguments separated by spaces when executed.
    - `clear` should clear the terminal when executed, not when constructed.
- Future refactor thread should:
  - Move all side effects into `execute(char* args)`.
  - Register these commands through `CommandRegistry::RegisterSystemCommands()` in the same pattern as `Ping` for network commands.

---

## Invariants and Current Limitations

- **Shell**
  - Prompt is a simple static `"& "`; there is no notion of user, hostname, or current directory yet.
  - Line editing is limited to appending characters and backspace; insertion at arbitrary positions is not implemented.
  - No real command history; `SHIFT_ARROW_LEFT` is a placeholder.
  - No bounds checks on `bufferIndex` vs. `commandbuffer` capacity; overlong input could overflow without additional safeguards.
  - Command lookup uses `HashMap<const char*, Command*>`; command names must be stable C‑strings (string literals or equivalent).
  - The shell does not delete `Command*`s; commands registered from the heap are effectively permanent.

- **Commands**
  - `whoami`, `echo`, `clear` violate the intended pattern by doing work in constructors rather than in `execute`.
  - `ping` uses `strtok` for both argument tokenization and IP parsing; callers must be aware of the global `strtok` state and avoid nested parsing pitfalls.

- **CommandRegistry**
  - Provides dependency injection and command registration but does not manage unregistration or command lifetime.
  - Dependency names and groups are hard‑coded strings; future threads may introduce enums or centralized configuration.

---

## Planned Refactor Directions (for CLI threads)

Future CLI threads should consider:

- Moving all command behavior into `execute`, not constructors (especially system commands).
- Centralizing argument parsing utilities (tokenization, quoting, etc.) so commands share robust parsing instead of each using `strtok` independently.
- Extending the command table as needed:
  - Allowing namespacing or hierarchical commands (e.g., `net.ping`).
  - Possibly supporting factories for lazy construction if command sets become large.
- Implementing:
  - Proper command history with navigation and replay.
  - Richer line editing (insert, delete, left/right movement with buffer shifting).
  - Safer handling of `commandbuffer` bounds and error conditions.
- Extending `CommandRegistry`:
  - Standardizing how commands query dependencies at construction time.
  - Potentially auto‑registering command groups based on which subsystems are available.
  - Defining a strategy for unregistering and/or freeing command objects if commands become dynamic.

When such changes are made, update this document to reflect:

- New parsing rules.
- New commands and flags.
- Any changes in dependency names, groups, or required subsystems.
