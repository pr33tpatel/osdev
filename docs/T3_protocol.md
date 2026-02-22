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

All protocol‑related docs live under `docs/`:

- `docs/T3_protocol.md` – This file, the protocol definition.
- Existing subsystem docs:
  - `docs/kernel.md`
  - `docs/memorymanagement.md`
  - `docs/storage.md`
  - `docs/cli.md`
  - `docs/utils.md`
  - `docs/ds.md`
  - `docs/development.md`

Individual threads **do not** normally create their own top‑level files; instead they:

- Update the relevant subsystem document(s).
- Optionally add a short “Thread notes” subsection if a change is large or subtle.

If a thread really needs its own doc (for a large multi‑step effort), it should:

- Place it under `docs/threads/NAME.md`.
- Link it from the relevant subsystem doc(s) and from this file under an appropriate section.

---

## Thread lifecycle

Every development thread should follow the same three phases:

1. **Scan and align**
2. **Implement and keep notes**
3. **Write back to the docs**

### 1. Scan and align

Before writing code, a thread should:

1. Identify the affected **subsystems**.
   - Example: changing CLI commands → `docs/cli.md` and possibly `docs/utils.md`.
2. Read the relevant sections in those docs and this protocol:
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

> If a change would surprise someone reading the existing `.md` files, it needs to be reflected there before the thread is complete.

### 3. Write back to the docs

Before the thread merges:

1. Open the relevant docs (`docs/*.md`) that describe the area you changed.
2. Update them to match the new reality:
   - Add new bullets, fields, or sections.
   - Adjust existing descriptions so they do not lie or mislead.
   - Remove text that describes behavior that no longer exists.
3. Prefer **small, precise edits** over large rewrites:
   - “Add a new subsection for the new command.”
   - “Update the list of fields in this struct.”

If a change is large, you may:

- Add a small “Thread notes” subsection at the end of the relevant section:

  ```markdown
  #### Thread notes – CLI refactor (2026-02)

  - Replaced constructor side effects in `whoami`, `echo`, `clear` with proper `execute()` implementations.
  - `Shell::ExecuteCommand` now passes a clean `args` buffer without modifying the original command history.
  ```

This is optional but helpful for future threads that need to understand why something is the way it is.

---

## Interaction rules between threads

This is the critical part: **how threads should behave with respect to each other** by using T3.

### 1. Respect the docs as a contract

- If a behavior is documented, you should assume **other threads depend on it**.
- You may change it, but when you do:
  - Update the docs in the same commit.
  - Make the change obvious enough that future threads will see it.

If you must temporarily violate the doc (e.g., intermediate step in a long refactor):

- Add a short **temporary note** in the doc, clearly marked, and remove it when the work completes.

Example:

```markdown
> NOTE (2026-02, CLI refactor): History support is temporarily disabled while the shell buffer is being reworked.
```

### 2. Use docs to negotiate boundaries

When two threads touch the same subsystem:

- The later thread should:
  - Re‑read the relevant docs.
  - Look for recent “Thread notes” sections.
- If the planned change clashes with a recent note:
  - Adapt to the new behavior if reasonable.
  - Or update the “Thread notes” and the main text to reflect the new understanding.

Threads should not silently override each other’s contracts.

### 3. Keep changes local and explicit

Each thread should:

- Prefer updating **only** the documents that match the code it touches.
- Avoid broad, vague edits that span multiple files unless necessary.

If a thread touches many subsystems, it is often better to:

- Split it into smaller threads.
- Or clearly annotate each section with **which part of the change it belongs to**.

### 4. Avoid hidden assumptions

When you introduce a new invariant or assumption, document it.

Examples:

- “`HashMap` capacity is fixed and does not grow dynamically.”
- “The ATA driver assumes 512‑byte sectors and single‑sector ops.”
- “CLI commands must not allocate large buffers on the stack.”

This way, future threads can discover these rules without reading every line of code.

---

## Recommended document structure

To keep T3 usable, subsystem docs should follow a consistent pattern (which we’ve already started):

1. **Responsibility / high‑level description**
2. **Key types, functions, or modules**
3. **Behavior / algorithms (with complexity where relevant)**
4. **Invariants and assumptions**
5. **Usage / examples**
6. **Open questions / TODO**
7. **Thread notes (optional, dated)**

Threads should:

- Align new sections with this pattern.
- Put new behavior in the right place instead of scattering it.

---

## Example: how a thread should use T3

### Example 1 – New CLI command

A “Network diagnostics” thread wants to add `traceroute` to the CLI.

Steps:

1. Scan:
   - Read `docs/cli.md` (command abstraction, flags, registry).
   - Read `docs/utils.md` if new string helpers are needed.
2. Implement:
   - Add `Traceroute` command class.
   - Wire it into the `CommandRegistry`.
3. Update docs:
   - In `docs/cli.md`, under **Concrete Commands**:
     - Add a bullet or subsection for `traceroute`.
     - Document arguments, flags, and expected behavior.
   - If new helpers were added to `utils`, update `docs/utils.md` accordingly.
4. If some behavior is currently rough (e.g., only IPv4, no error codes):
   - Add a small “Open questions / TODO” note under the command’s subsection.

### Example 2 – Changing a data structure invariant

A thread decides to fix `LinkedList::Remove` to work strictly on values rather than node pointers.

Steps:

1. Scan:
   - Read `docs/ds.md` `LinkedList<T>` section.
2. Implement:
   - Adjust `Remove` and any call sites.
3. Update docs:
   - Update `LinkedList<T>` → **Notes / Caveats**:
     - Remove the mention that `Remove` assumes `T` behaves like a node pointer.
     - Replace with the new semantics.
   - Add a short dated “Thread notes” entry describing the change and any compatibility implications.

---

## Style and tone guidelines

When editing T3‑related docs:

- Be **precise** and **technical**.
- Favor facts over marketing language.
- Write as if you are leaving notes for another low‑level engineer:
  - What they need to know to avoid breaking invariants.
  - What surprised you while working on the code.

Avoid:

- Long narrative histories.
- Personal commentary or conversational tone.

If a detailed explanation is needed (e.g. algorithm walkthrough), keep it focused and link to external references where appropriate.

---

## Minimal checklist for every thread

Before you consider a thread “done,” check:

- [ ] Did I read the relevant docs and this T3 file before coding?
- [ ] Did I change any **observable behavior**?
  - If yes:
    - [ ] Did I update the corresponding `.md` files?
- [ ] Did I add or change any **invariants or assumptions**?
  - If yes:
    - [ ] Did I document them in the appropriate section?
- [ ] If the change is large or subtle:
  - [ ] Did I add a brief dated “Thread notes” snippet?

If all boxes are checked, the thread is in good shape for future threads to build on.

---

## Future extensions

Potential improvements to the T3 system:

- A `docs/threads/` index listing major historical threads and their docs.
- Lightweight tags in docs (e.g., `T3:[cli-refactor-2026-02]`) to make it easier to search for related changes.
- Scripts to:
  - Check for stale references (e.g., functions removed from code but still listed in docs).
  - Generate a simple “subsystem map” from the docs.

Until then, this protocol is intentionally simple: **read the docs, change the code, then fix the docs.**
```
