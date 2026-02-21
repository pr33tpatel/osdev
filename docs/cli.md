# CLI and Shell

## Status

The CLI is under active development and refactoring.  
This document describes the current design and behavior so that future threads can safely extend or change it and then update this doc.

---

## High-Level Design

- **Shell (`Shell`)**
  - Owns the input buffer and command registry (list of `Command*`).
  - Implements a keyboard handler interface via `OnKeyDown(char c)`.
  - Parses user input into command name + arguments and dispatches to matching `Command` objects.
- **Command abstraction (`Command`, `FlagOption`)**
  - Base class for all CLI commands.
  - Simple flag parsing helpers (`ParseFlags`, `PrintFlags`).
- **Command registry / dependency injection (`CommandRegistry`)**
  - Stores named subsystem dependencies (e.g., `"SYS.SHELL"`, `"NET.IPV4"`).
  - Provides runtime validation that required dependencies exist before enabling command groups.
- **Concrete commands**
  - System commands (e.g., `whoami`, `echo`, `clear`).
  - Network commands (e.g., `ping`) which use the network stack.

---

## Shell

### Responsibilities

- Capture and edit the current input line (`commandbuffer`).
- Render a prompt and echo typed characters through the `Terminal`.
- Handle basic cursor movement and scrolling via arrow keys and scroll commands.
- Parse the completed line into command name and argument string.
- Look up and execute registered commands.

### Lifecycle

```cpp
Shell::Shell() {}
Shell::~Shell() {}
```

- Constructed once in `kernelMain`.
- Registered as the keyboard event handler in text mode:
  - `KeyboardDriver keyboard(&interrupts, &shell);`

### Command registration

```cpp
void Shell::RegisterCommand(Command* cmd) {
  if (numCommands < 65535) {
    commandRegistry[numCommands] = cmd;
    numCommands++;
  } else {
    printf(RED_COLOR, BLACK_COLOR,
           "Command Registry Full. Max Commands Allowed: %d\n", numCommands);
  }
}
```

- `commandRegistry` is a flat array of `Command*` (size up to 65535).
- No de‑duplication or removal yet; future refactors may add:
  - Command namespacing.
  - Overwrite or unregister mechanics.

### Command execution

```cpp
void Shell::ExecuteCommand() {
  if (commandbuffer == '\0') return;

  char* cmdName = strtok(commandbuffer, " ");
  if (cmdName == 0) return; // only spaces

  char* args = commandbuffer + strlen(cmdName) + 1;
  while (args == ' ') args++; // skip spaces

  bool found = false;
  for (int i = 0; i < numCommands; i++) {
    if (strcmp(cmdName, commandRegistry[i]->name) == 0) {
      commandRegistry[i]->execute(args);
      found = true;
      break;
    }
  }

  if (!found) {
    printf(YELLOW_COLOR, BLACK_COLOR,
           "\"%s\" not in Command Registry\n", cmdName);
  }
}
```

- Current parsing is simple:
  - Uses `strtok` to extract the command name.
  - Everything after the first token (minus leading spaces) is passed as `args` to `Command::execute`.
- **Important**:
  - `ExecuteCommand` modifies `commandbuffer` via `strtok`; caller must treat `commandbuffer` as consumed for that input line.
  - Implementations of `Command::execute` are responsible for further tokenization of `args`.

### Prompt and buffer utilities

```cpp
void Shell::PrintPrompt() {
  char* prompt = "& ";
  printf(RED_COLOR, BLACK_COLOR, prompt);
}

void Shell::PrintPreviousCmd() {
  printf(RED_COLOR, BLACK_COLOR, "Command Received: ");
  printf(LIGHT_GRAY_COLOR, BLACK_COLOR, "%s\n", commandbuffer);
}

void Shell::ShellInit() {
  PrintPrompt();
}

void Shell::fillCommandBuffer(char fill_char, uint16_t length) {
  for (uint16_t i = 0; i < length; i++) {
    commandbuffer[i] = fill_char;
  }
  bufferIndex = length % (sizeof(commandbuffer) / sizeof(char)) + 1;
}
```

- `PrintPrompt` prints the current prompt; currently a static `"& "`.
- `fillCommandBuffer` is a helper for tests or future features; note the modulo logic may be revisited in refactors.

### Keyboard handling and line editing

```cpp
void Shell::OnKeyDown(char c) {
  if (Terminal::activeTerminal == 0) return;
```

- If no active terminal, shell ignores input.

#### Cursor navigation and scrolling

```cpp
  if ((uint8_t)c == ARROW_UP)    { Terminal::activeTerminal->moveCursor(0, -1); return; }
  if ((uint8_t)c == ARROW_RIGHT) { Terminal::activeTerminal->moveCursor(1, 0); cursorIndex++; return; }
  if ((uint8_t)c == ARROW_DOWN)  { Terminal::activeTerminal->moveCursor(0, 1); return; }
  if ((uint8_t)c == ARROW_LEFT)  { Terminal::activeTerminal->moveCursor(-1, 0); cursorIndex--; return; }

  if ((uint8_t)c == SHIFT_ARROW_DOWN) { Terminal::activeTerminal->ScrollDown(); return; }
  if ((uint8_t)c == SHIFT_ARROW_UP)   { Terminal::activeTerminal->ScrollUp();   return; }
```

- Uses special key codes (e.g., `ARROW_UP`) defined elsewhere (likely in keyboard/terminal headers).
- `cursorIndex` is tracked but **insertion/overwrite editing is not yet implemented**; current code behaves like a simple append-only line editor.

#### History (placeholder)

```cpp
  if ((uint8_t)c == SHIFT_ARROW_LEFT) {
    printf("history");
    return;
  }
```

- Future feature: command history navigation (currently prints `"history"`).

#### Enter and backspace

```cpp
  if (c == '\n') {
    putChar('\n');

    if (bufferIndex > 0) {
      commandbuffer[bufferIndex] = '\0';
      ExecuteCommand();
      bufferIndex = 0;
      // PrintPreviousCmd();
    }
    PrintPrompt();
  }

  else if (c == '\b') {
    if (bufferIndex > 0) {
      putChar('\b');
      bufferIndex--;
      commandbuffer[bufferIndex] = 0;
    }
  }
```

- On Enter:
  - Terminates the buffer with `'\0'`.
  - Calls `ExecuteCommand`.
  - Resets `bufferIndex` and prints a new prompt.
- Backspace:
  - Only works if there are characters to delete.
  - Updates both the terminal display and the internal buffer.

#### Normal character input

```cpp
  else {
    putChar(c);
    commandbuffer[bufferIndex] = c;
    bufferIndex++;
  }
}
```

- Appends printable characters to the buffer and echoes them to the terminal.
- No bounds checking yet on `bufferIndex` vs. `commandbuffer` capacity; refactors should add safety.

---

## Command Abstraction

### Command base class

```cpp
struct FlagOption {
  const char* name;
  const char* description;
  bool* value;
};

class Command {
 public:
  const char* name;
  Command(const char* name) : name(name) {}
  virtual void execute(char* args) = 0;
};
```

- Every concrete command:
  - Has a `name` used by the shell for lookup.
  - Implements `execute(char* args)` and does its own argument parsing.

### Flag utilities

```cpp
inline void ParseFlags(char* current_arg, FlagOption* flags, int flagCount);
inline void PrintFlags(
    FlagOption* flags,
    int flagCount,
    enum os::utils::VGAColor fg = utils::YELLOW_COLOR,
    enum os::utils::VGAColor bg = utils::BLACK_COLOR);
```

- `ParseFlags`:
  - Compares `current_arg` against `flags[i].name` using `os::utils::strcmp`.
  - When matched, sets `*(flags[i].value) = true`.
- `PrintFlags`:
  - Prints a table of flag names and descriptions with configurable colors.

These helpers are used, for example, in the `ping` command.

---

## CommandRegistry and Dependency Injection

### Role

- Provide a global registry of subsystem instances needed by commands:
  - E.g., `Shell`, `PCIController`, `MemoryManager`, `Terminal`, `ARP`, `IPv4`, `ICMP`, `TaskManager`, filesystem objects.
- Validate that required dependencies are present and marked as valid before enabling specific command groups.

### Structure

```cpp
class CommandRegistry {
  // internally: HashMap<const char*, DependencyEntry> dependencyMap;
public:
  CommandRegistry();

  void InjectDependency(const char* depName, void* depPtr);
  void* GetDependency(const char* depName);

  bool ValidateSystemDependencies();
  bool ValidateNetworkDependencies();
  bool ValidateProcessDependencies();
  bool ValidateFileSystemDependencies();
  bool ValidateAllDependencies();

private:
  bool ValidateGroup(const char** requiredDeps, uint32_t count);
};
```

- `DependencyEntry` (from header):
  - Likely contains `void* ptr;` and `bool isValid;`.

### Injecting dependencies

```cpp
void CommandRegistry::InjectDependency(const char* depName, void* depPtr) {
  DependencyEntry depEntry;
  depEntry.ptr = depPtr;
  depEntry.isValid = (depPtr != 0);
  dependencyMap.Insert(&depName, &depEntry);
}
```

- Called from `kernelMain`, for example:

  - `InjectDependency("SYS.SHELL", &shell);`
  - `InjectDependency("SYS.PCI", &PCIController);`
  - `InjectDependency("NET.ICMP", &icmp);`
  - etc.

### Getting dependencies

```cpp
void* CommandRegistry::GetDependency(const char* depName) {
  DependencyEntry depEntry;
  if (dependencyMap.Get(&depName, &depEntry)) {
    if (depEntry.isValid)
      return depEntry.ptr;
  }
  return 0;
}
```

- Used by commands to retrieve access to subsystems without global hard‑coding.

### Validation

```cpp
bool CommandRegistry::ValidateGroup(const char** requiredDeps, uint32_t count);
```

- For each name in `requiredDeps`:
  - Queries `dependencyMap`.
  - If missing or invalid, prints `[ERROR] MISSING DEPENDENCY: name` and returns `false`.
  - Otherwise prints `[SHELL] LOADED DEPENDENCY: name`.

Specific groups:

```cpp
bool CommandRegistry::ValidateSystemDependencies() {
  const char* systemDeps[] = {"SYS.SHELL", "SYS.PCI", "SYS.HEAP", "SYS.TERMINAL"};
  // prints AVAILABLE / UNAVAILABLE
}

bool CommandRegistry::ValidateNetworkDependencies() {
  const char* networkDeps[] = {"NET.ARP", "NET.IPV4", "NET.ICMP"};
}

bool CommandRegistry::ValidateProcessDependencies() {
  const char* processDeps[] = {"PROC.TASKMANAGER"};
}

bool CommandRegistry::ValidateFileSystemDependencies() {
  const char* filesystemDeps[] = {"FS.INIT"};
}

bool CommandRegistry::ValidateAllDependencies() {
  printf("Validating DracOS Dependencies...\n");
  bool sys  = ValidateSystemDependencies();
  bool net  = ValidateNetworkDependencies();
  bool proc = ValidateProcessDependencies();
  bool fs   = ValidateFileSystemDependencies();

  // Uses bitwise & (not &&) so all groups are checked.
  return sys & net & proc & fs;
}
```

- This is called in `kernelMain` after all `InjectDependency` calls.
- Commands can use `CommandRegistry` to fetch pointers to required subsystems when they are constructed or executed.

---

## Concrete Commands

### Network: `ping`

File: `cli/commands/networkCmds.cc`

```cpp
class Ping : public Command {
  InternetControlMessageProtocol* icmp;
public:
  Ping(InternetControlMessageProtocol* icmp);
  void execute(char* args) override;
};
```

#### Behavior

- Constructor:

  ```cpp
  Ping::Ping(InternetControlMessageProtocol* icmp)
    : Command("ping"), icmp(icmp) {}
  ```

- `execute(char* args)`:

  1. Tokenize `args` into at most 255 arguments:

     ```cpp
     char* argvList;
     uint8_t argcVal = 0;
     argvList = (char*)this->name;
     argcVal++;

     char* token = strtok(args, " ");
     while (token != 0 && argcVal < 255) {
       argvList[argcVal] = token;
       argcVal++;
       token = strtok(0, " ");
     }
     ```

  2. Define state:

     ```cpp
     char* ip_str = 0;
     bool helpFlag    = false;
     bool verboseFlag = false;
     bool debugFlag   = false;
     bool hexFlag     = false;
     ```

  3. Declare flags:

     ```cpp
     FlagOption flags[] = {
       {"-h",  "display help contents",                         &helpFlag},
       {"-v",  "display contents of packets",                   &verboseFlag},
       {"-d",  "debug mode, display user input and variables",  &debugFlag},
       {"-hex","display IP's in hexadecimal",                   &hexFlag}
     };
     int numFlags = sizeof(flags) / sizeof(flags);
     ```

  4. Second pass: classify flags vs main argument:

     ```cpp
     for (int i = 1; i < argcVal; i++) {
       char* argv = argvList[i];
       if (argv == '-') {
         ParseFlags(argv, flags, numFlags);
       } else {
         if (ip_str == 0) ip_str = argv;
       }
     }
     ```

  5. If `debugFlag` is set, print parsed tokens and IP string.

  6. If `helpFlag` is set:
     - Print usage string and flags via `PrintFlags`.
     - Return.

  7. If an IP string was provided:
     - Split `ip_str` on `.` into `ip_parts[4]` using `strtok`.
     - Convert each octet string to integer with `strToInt`.
     - Compose `targetIP_BE` using `convertToBigEndian`.
     - Print a “Pinging x.x.x.x…” line.
     - Call `icmp->Ping(targetIP_BE)`.

  8. Otherwise:
     - Print usage string.

- Future refactors may:
  - Add use of `verboseFlag`, `hexFlag` to tune output.
  - Integrate with history and better error messaging.

### System: `whoami`, `echo`, `clear`

File: `cli/commands/systemCmds.cc`

#### `whoami`

```cpp
whoami::whoami() : Command("whoami") {
  printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "pr33t\n");
}
```

- Current version prints `"pr33t"` in the constructor.
- **Note**: Constructors are run when the command object is created, not when the user types `whoami`.
  - Future refactor should move the printing logic into `execute`.

#### `echo`

```cpp
echo::echo() : Command("echo") {
  char* arg = strtok(0, " ");
  while (arg != 0) {
    printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "%s ", arg);
    arg = strtok(0, " ");
  }
  printf("\n");
}
```

- Similar to `whoami`, this currently does its work in the constructor using `strtok(0, " ")`.
- Intended behavior:
  - `echo <args...>` should print all subsequent arguments separated by spaces.
- Future refactor:
  - Move parsing/printing into `execute(char* args)`.

#### `clear`

```cpp
clear::clear() : Command("clear") {
  Terminal::activeTerminal->Clear();
}
```

- Immediately clears the active terminal in its constructor.
- Intended behavior:
  - `clear` should clear the terminal when executed, not when constructed.
- Future refactor:
  - Provide a proper `execute` implementation.

---

## Invariants and Current Limitations

- Shell:
  - Single prompt style `"& "`; no user/hostname or path context yet.
  - No line editing beyond backspace; insertion at arbitrary `cursorIndex` is commented out and not yet supported.
  - No real history mechanism; `SHIFT_ARROW_LEFT` is a placeholder.
  - No bounds checks on `commandbuffer`/`bufferIndex` yet.
- Commands:
  - `whoami`, `echo`, `clear` are currently misusing constructors for runtime behavior; they need proper `execute` methods.
  - `ping` uses `strtok` heavily (`args` and `ip_str`), so reentrancy and nested parsing must be used with care.
- CommandRegistry:
  - Only provides dependency injection and validation; it does not manage command registration itself.
  - Dependency names and groups are hard‑coded strings; refactors may add enums or centralized config.

---

## Planned Refactor Directions (for CLI threads)

CLI threads updating this doc should consider:

- Moving all command behavior into `execute`, not constructors.
- Centralizing argument parsing utilities (tokenization, quoting, etc.).
- Introducing a `CommandTable` that:
  - Maps names to `Command*`.
  - Alternatively, stores factory functions for lazy construction.
- Implementing:
  - Proper command history.
  - Line editing (insert, delete, left/right movement with buffer shifting).
  - Safer `commandbuffer` bounds handling.
- Expanding `CommandRegistry`:
  - Allow commands to query dependencies at construction time in a consistent pattern.
  - Possibly auto-register commands based on available subsystems.

When such changes are made, update this document to reflect:

- New parsing rules.
- New commands and flags.
- Any changes in dependency names or required subsystems.
```
