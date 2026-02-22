# Thread-to-Thread (T3) Protocol

The Thread‑to‑Thread (T3) protocol defines how DracOS development threads and documentation threads communicate with each other through Markdown files.

It is a lightweight “meta‑layer” on top of the code and docs that keeps features, refactors, and design notes consistent across the project.

---

## Goals

- Give every **development thread** (feature, refactor, investigation) a clear contract:
  - What it reads.
  - What it produces.
  - Which files it must update when it is done.
- Make the docs **self‑healing**:
  - If a thread changes behavior or contracts, it updates the relevant `.md` files in a predictable way.
- Keep everything **text‑only and git‑friendly**:
  - No ticket system required.
  - All state lives in the repo.
- Treat the GitHub `docs/` directory as the **single source of truth** for documentation:
  - Tools and helpers (including this Space) should read docs over HTTPS from GitHub when possible so they always reflect the latest committed state.

---

## Vocabulary

- **Thread** – A unit of work with a cohesive purpose.
  - Examples: “CLI refactor thread”, “Memory manager instrumentation thread”.
- **T3 document** – A Markdown file that:
  - Describes a thread or a group of related threads.
  - Defines expectations for how future threads that touch the same area must behave.
- **Protocol** – The set of rules threads must follow when:
  - Reading existing T3 docs.
  - Updating them after changes.
  - Interacting with other threads through those docs.

---

## File layout

All protocol‑related docs live under `docs/` in the GitHub repo (canonical):

- `docs/T3_protocol.md` – This file, the protocol definition.
- Subsystem docs:
  - `docs/kernel.md`
  - `docs/memorymanagement.md`
  - `docs/storage.md`
  - `docs/cli.md`
  - `docs/utils.md`
  - `docs/ds.md`
  - `docs/development.md`

Other tools or Spaces must treat the GitHub versions under `docs/` as authoritative. Any local copies (e.g. uploaded into OSDEV Space) are convenience snapshots and may lag behind; when in doubt, prefer the GitHub version fetched over HTTPS.

Individual threads **do not** normally create their own top‑level files; instead they:

- Update the relevant subsystem document(s).
- Optionally add a short “Thread notes” subsection if a change is large or subtle.

If a thread really needs its own doc (for a large multi‑step effort), it should:

- Place it under `docs/threads/NAME.md`.
- Link it from the relevant subsystem doc(s) and from this file under an appropriate section.

---

## Documentation style

To keep docs readable and maintainable:

- Prefer **high‑level descriptions** of behavior, design, and invariants.
- Avoid large, copy‑pasted source code blocks when the same code is available in the repository:
  - Include only short snippets when they clarify behavior or an API shape.
  - For full implementations, point readers to the relevant source files instead (paths under `src/` or `include/`).
- Use bullets and short paragraphs to describe:
  - How a function or class behaves.
  - How subsystems interact.
  - What contracts callers must obey.

Example pattern:

- Good: “`Shell::ExecuteCommand` parses the first token as the command name, uses a hashmap to resolve a `Command*`, and calls `execute(args)` if found. See `cli/shell.cc` for details.”
- Avoid: Pasting the entire `ExecuteCommand` implementation when a link to the source is enough.

When you refactor code, update the docs to explain the new design and point to the new implementation, but keep the documentation focused on intent and behavior rather than line‑by‑line code.

---

## Thread lifecycle

Every development thread should follow the same three phases:

1. **Scan and align**
2. **Implement and keep notes**
3. **Write back to the docs**

### 1. Scan and align

Before writing code, a thread should:

1. Identify the affected **subsystems**.
   - Example: changing CLI commands → `docs/cli.md` and possibly `docs/utils.md` / `docs/ds.md`.
2. Read the relevant sections in those docs and this protocol:
   - Always use the GitHub `docs/` versions as the latest reference.
   - Understand current contracts, invariants, and terminology.
3. Decide whether the change:
   - **Fits** into the existing design.
   - **Extends** it (new features).
   - **Contradicts** it (breaking changes).

If it contradicts the design, the thread must:

- Either adjust its plan to fit the existing design, or
- Explicitly plan to update the docs to describe the new, correct behavior.

### 2. Implement and keep notes

While working, a thread should keep short notes (mentally or in a scratch file) about:

- New behaviors:
  - New commands, new flags, new structures, new invariants.
- Changed behaviors:
  - Old calls that now behave differently.
- Removed behaviors:
  - Removed options, removed subsystems, or de‑scoped features.

The important rule:

> If a change would surprise someone reading the existing `.md` files in `docs/`, it needs to be reflected there before the thread is complete.

### 3. Write back to the docs

Before the thread merges:

1. Open the relevant docs (`docs/*.md`) in the GitHub repo that describe the area you changed.
2. Update them to match the new reality:
   - Add new bullets, fields, or sections.
   - Adjust existing descriptions so they do not lie or mislead.
   - Remove text that describes behavior that no longer exists.
3. Prefer **small, precise edits** over large rewrites:
   - “Add a new subsection for the new command.”
   - “Update the list of fields in this struct.”
   - “Clarify that the shell now uses a hashmap for command lookup instead of a flat array.”

If a change is large, you may:

- Add a small “Thread notes” subsection at the end of the relevant section with a tagline and date of the change
    - e.g. XYZ CHANGE (2/3/2026): ...
        - change1 ...
        - change2 ... 
