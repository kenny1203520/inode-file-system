# AI Coding Agent Instructions for this Workspace

This workspace is currently empty (no source files detected). These instructions provide a lightweight scaffold so agents can be productive once code is added. Update this document as the project structure emerges.

## Scope & Assumptions
- Project name: inode-file-system (likely OS/FS coursework).
- Language and tooling: Unknown. Agents must infer from added files.
- Goal: Document discoverable, project-specific practices only.

## What Agents Should Do First
- Discover project type via root files: search for `package.json`, `pyproject.toml`, `Cargo.toml`, `Makefile`, `CMakeLists.txt`, or `README.md`.
- Map architecture by scanning `src/`, `lib/`, `include/`, `tests/`, and any `docs/`.
- Identify build tasks from config files (e.g., `Makefile` targets, npm scripts).

## Architecture Notes (to be updated when code lands)
- If this is a filesystem/inode simulation:
  - Expect modules for `inode` structures, `directory` management, `block`/`disk` abstraction, and `fs` operations (mount, read/write, allocate/free).
  - Common data flows: syscall or CLI → `fs` API → inode lookup → block IO → persistence layer.
  - Watch for fixed-size tables (inode table, free list/bitmap) and path resolution (`/a/b/c` → parent + child inodes).
- Reference files to update here once present: `src/fs/*`, `src/inode.*`, `src/disk.*`, `tests/*`.

## Conventions to Capture
- Code style: derive from formatter configs if present (`.editorconfig`, `.clang-format`, `.prettierrc`).
- Error handling: capture project-specific return codes or exceptions.
- Logging/printing: central helpers vs. inline prints.
- Persistence: file-backed images (e.g., `fs.img`) vs. in-memory.

## Developer Workflows (fill in when discoverable)
- Build: prefer existing scripts/targets. Examples to add when present:
  - `make build` or `npm run build` or `cargo build`.
- Test: mirror project’s setup, e.g. `ctest`, `pytest`, `cargo test`, `npm test`.
- Run: document the primary entrypoint and example commands.
- Debug: note typical tooling (`gdb`, `lldb`, `node --inspect`, `pytest -k`).

## Patterns & Examples (to be replaced with real ones)
- Path resolution: implement `lookup(path)` using split components and inode traversal.
- Allocation strategy: maintain free block/inode bitmap; choose first-fit unless otherwise specified.
- Write barrier: ensure metadata updates (inode size, block pointers) are atomic relative to data writes.

## External Dependencies
- Record libraries/tools discovered from manifests (e.g., `libuv`, `pytest`, `googletest`). Pin versions if specified.

## How to Update This File
- When new files are added:
  - Summarize architecture in 4-6 bullets, referencing key files like `src/fs.c` or `src/inode.rs`.
  - List exact build/test commands found in config.
  - Document project-specific conventions (naming, error codes, IO patterns) with code references.
  - Add 1-2 concrete examples (function names, data flows) from the code.

## Agent Operating Rules
- Prefer minimal, surgical changes aligned to existing style.
- Do not introduce libraries or frameworks unless manifests or course guidelines allow.
- Keep changes focused; avoid refactors without tests or clear rationale.
- Update this document when patterns are clearly discoverable and stable.

## Open Questions for Maintainers
- Language/toolchain intended (C/C++/Rust/Python/JS)?
- Expected output artifacts (binary, library, disk image)?
- Testing framework preference and coverage goals?
- Any course-specific constraints or grading rubrics to honor?
