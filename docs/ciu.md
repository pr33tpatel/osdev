# Central Intelligence Unit (CIU)

## Status

The Central Intelligence Unit (CIU) is under active development.  
This document describes the current design and behavior so future threads can extend CIU (e.g., CIU terminal, filters) and then update this doc accordingly. 

***

## High‑Level Design

- **CIU core ([`CIU`](#ciu-core))**
  - Central collection point for structured system events (“reports”) coming from CIU officers.
  - Applies routing policy based on subsystem and severity, then forwards reports to output sinks (currently the main terminal only).
  - Owns severity and subsystem color themes for a consistent visual log style.

- **Report structure ([`CIUReport`](#ciureport))**
  - Represents a single event: core fields (severity, subsystem, code, message) plus optional metadata key–value pairs.
  - Metadata is stored in `HashMap<const char*, const char*>` from the DracOS data structures library for flexible tagging and later analysis. 

- **Officer abstraction ([`CIUOfficer`](#ciuofficer))**
  - Lightweight adapter that subsystems instantiate with their identity (e.g., `"SHELL"`, `"NETWORK"`).
  - Provides convenience methods for emitting warnings/errors/etc. without repeating boilerplate at call sites.
  - Builds `CIUReport` objects and forwards them to CIU core.

- **Output sinks**
  - **Main terminal sink**: formats and prints CIU reports to the active terminal using existing colored `printf` utilities.
  - **CIU terminal sink**: planned; routing flags already support sending events to a dedicated CIU debug terminal in the future.

***

## CIUReport

### Responsibilities

- Capture the **essential structure** of a single CIU event:
  - Severity and subsystem identity.
  - A short, stable code for the event.
  - A human‑readable message.
- Provide an **optional metadata map** for richer tagging and debugging context.
- Support **low‑boilerplate** enrichment at call sites (chainable `meta` calls).

### Structure

`CIUReport` is a simple data class:

- Core fields (required):
  - `CIUSeverity severity`  
    - Enum: `Trace`, `Info`, `Warning`, `Error`, `Critical`.
  - `const char* subsystem`  
    - Subsystem identity (e.g., `"SHELL"`, `"NETWORK"`, `"KERNEL"`).
  - `const char* code`  
    - Stable identifier string (e.g., `"NETWORK_NOT_INITIALIZED"`, `"SHELL_NOT_INITIALIZED"`).
  - `const char* message`  
    - Short human‑readable description.

- Metadata (optional):
  - `HashMap<const char*, const char*> metadata;`
  - Used for arbitrary tags such as:
    - `PHASE` – `"boot"`, `"runtime"`, `"shutdown"`.
    - `CATEGORY` – `"init"`, `"io"`, `"config"`, `"logic"`.
    - `MODULE` – usually `__FILE_NAME__`.
    - `FUNCTION` – usually `__func__`.
    - Additional keys as needed per subsystem.

- Builder helper:
  - `CIUReport& meta(const char* key, const char* value);`
    - Inserts the key–value pair into `metadata` and returns `*this` for chaining.

Example (network dependency failure in `CommandRegistry`):

```cpp
CIUReport report(CIUSeverity::Warning,
                 "SHELL",
                 "NETWORK_NOT_INITIALIZED",
                 "NETWORK COMMANDS UNAVAILABLE");

report.meta("PHASE",    "boot")
      .meta("CATEGORY", "init")
      .meta("MODULE",   __FILE_NAME__)
      .meta("FUNCTION", __func__);

// officer.send(report);
```

This produces a structured event with a stable code and a set of tags describing where and when it happened.

***

## CIUOfficer

### Role

- Acts as a **middleman** between subsystems and CIU core.
- Encapsulates the subsystem identity so call sites do not repeat it.
- Offers **simple, one‑line** helpers for common severities, plus a `send` method for fully customized reports.

### Structure and usage

- Construction:
  - Each subsystem creates its own static officer with a chosen identity:

    ```cpp
    static CIUOfficer officer("SHELL");
    ```

- Main entrypoint:
  - `void send(CIUReport& report);`
    - Takes a pre‑built report (with metadata if needed) and forwards it to CIU core (`CIU::Report`).

- Convenience methods:
  - `void trace(const char* code, const char* message);`
  - `void info(const char* code, const char* message);`
  - `void warning(const char* code, const char* message);`
  - `void error(const char* code, const char* message);`
  - `void critical(const char* code, const char* message);`
  - Each helper:
    - Constructs a `CIUReport` with the officer’s `subsystem` and the given code/message.
    - Calls `send(report)` internally.

Example (simple warning without metadata):

```cpp
officer.warning("NETWORK_NOT_INITIALIZED", "NETWORK COMMANDS UNAVAILABLE");
```

Example (warning with metadata):

```cpp
CIUReport report(CIUSeverity::Warning,
                 "SHELL",
                 "NETWORK_NOT_INITIALIZED",
                 "NETWORK COMMANDS UNAVAILABLE");

report.meta("PHASE",    "boot")
      .meta("CATEGORY", "init");

officer.send(report);
```

***

## CIU Core

### Responsibilities

- Initialize CIU internal state (`Init()` / `IsReady()`).
- Accept reports from officers (`Report(const CIUReport&)`).
- Resolve routing policies based on subsystem and severity.
- Delegate rendering to sinks (currently main terminal only).

### Initialization and lifecycle

- `CIU::Init()`:
  - Called from `kernelMain` after the heap is initialized and before subsystems start logging.
  - Sets up:
    - Default routing table (severity → sinks).
    - Severity color map.
    - Subsystem color map.
  - Sets `CIU::ready = true`.

- `CIU::IsReady()`:
  - Returns whether CIU initialization has completed.

If a report arrives before CIU is ready, CIU emits a fallback message to the main terminal indicating that CIU is not ready and prints the code/message.

### Routing policy

- Routing table:
  - Backed by `HashMap<uint32_t, CIURouteFlags> routingMap;`.
  - Keys are derived from `(subsystem, severity)` via `MakeRouteKey(subsystem, severity)`.
  - Values are `CIURouteFlags`:

    - `bool toMainTerminal;`
    - `bool toCIUTerminal;` (reserved for future CIU terminal).

- Default severity policy (via wildcard `"*"` subsystem):
  - `Trace` → CIU terminal only (planned).
  - `Info` → CIU terminal only (planned).
  - `Warning` → main terminal + CIU terminal.
  - `Error` → main terminal + CIU terminal.
  - `Critical` → main terminal + CIU terminal.

- Resolution:
  - For a given report:
    1. Look up a subsystem‑specific rule `(subsystem, severity)`.
    2. If none exists, fall back to wildcard rule `("*", severity)`.
    3. If no rule is found at all, default to “no sinks”.

This makes it easy to adjust behavior per subsystem or per severity without touching call sites.

### Color theming

- Severity colors:
  - `HashMap<uint8_t, CIUColor> colorMap;`
  - Maps `CIUSeverity` values to foreground/background `VGAColor` pairs.

- Subsystem colors:
  - `HashMap<uint32_t, CIUColor> subsystemColorMap;`
  - Maps hashed subsystem names to a distinct accent color.

Both maps are set up in `SetupDefaultColors()` so the visual theme is centralized and easy to tweak.

***

## Main Terminal Output

### Format

The main terminal sink prints CIU reports in a compact tagged format:

- Header:

  ```text
  [SUBSYSTEM][SEVERITY] MESSAGE (CODE)
  ```

  - `[SUBSYSTEM]` – colored using the subsystem accent.
  - `[SEVERITY]` – colored using the severity color.
  - `MESSAGE` – printed in severity color.
  - `(CODE)` – printed in a neutral label color.

- Metadata (if present):

  - Printed on one or more lines below the header as key–value pairs.
  - For example (one key per line style):

    ```text
    [SHELL][WARNING] NETWORK COMMANDS UNAVAILABLE (NETWORK_NOT_INITIALIZED)
    {PHASE=boot}
    {CATEGORY=init}
    {MODULE=commandregistry.cc}
    {FUNCTION=ValidateNetworkDependencies}
    ```

  - Keys and values come from the report’s `metadata` map using `GetKeys`/`Get`.

### Example: missing network dependency

When `CommandRegistry::ValidateNetworkDependencies()` fails due to a missing `"NET.ICMP"` dependency:

- Subsystem prints a user‑facing message:

  ```text
  [SHELL] NETWORK COMMANDS UNAVAILABLE
  ```

- CIU officer emits a structured report:

  ```cpp
  CIUReport report(CIUSeverity::Warning,
                   "SHELL",
                   "NETWORK_NOT_INITIALIZED",
                   "NETWORK COMMANDS UNAVAILABLE");

  report.meta("PHASE",    "boot")
        .meta("CATEGORY", "init")
        .meta("MODULE",   __FILE_NAME__)
        .meta("FUNCTION", __func__);

  officer.send(report);
  ```

- CIU main terminal output:

  ```text
  [SHELL][WARNING] NETWORK COMMANDS UNAVAILABLE (NETWORK_NOT_INITIALIZED)
  {PHASE=boot}
  {CATEGORY=init}
  {MODULE=commandregistry.cc}
  {FUNCTION=ValidateNetworkDependencies}
  ```

This gives a quick visual clue plus precise context for debugging.

***

## Invariants and Current Limitations

- CIU must be initialized **after** the heap and **before** subsystems start logging.
- Reports rely on stable `const char*` strings for `subsystem`, `code`, and metadata keys/values.
- Routing currently only drives the **main terminal**; CIU terminal and other sinks are not implemented yet.
- Metadata ordering is not guaranteed beyond what is implemented in CIU (e.g., current behavior is based on iteration over the metadata map, possibly with simple sorting helpers).
- CIU does not yet:
  - Persist logs beyond in‑memory terminal output.
  - Perform rate limiting or deduplication.
  - Support dynamic reconfiguration of routing or colors at runtime.

Future CIU threads may:

- Implement the dedicated CIU terminal and enable `toCIUTerminal` routing.
- Add per‑subsystem log level controls.
- Introduce basic correlation IDs and higher‑level “intelligence summaries”.
- Extend metadata conventions and terminal formatting as more subsystems adopt CIU.
