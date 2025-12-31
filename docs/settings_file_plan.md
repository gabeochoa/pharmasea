# Versioned Settings File Plan (non-JSON)

This document proposes a versioned, human-editable **non-JSON** settings file format that is:

- **Versioned**
- **Diff-friendly** (text, stable ordering)
- **Exact for floats** (IEEE-754 stored as hex bits)
- **Fast to load** (skip defaults / apply only overrides)
- **Not required to be backwards compatible**

Source notes: `serializing_notes.md` (lines 1–22).

---

## Goals (from the notes)

- Store floats in hex to avoid precision issues.
- Mark values that are not the default with a `*` so loading can skip unchanged settings.
- Have a version number.
- Track when keys are introduced / deprecated across versions (e.g. `@v2`, `@v3-4`).

---

## Current decisions (based on follow-up)

1. **Defaults source of truth**: TBD (recommendation below).
2. **File location/name**: keep the same path and filename for now (currently `settings.bin` under the save-games folder).
3. **Key naming**: snake_case.
4. **Unknown/deprecated keys**: ignore and `log_warn`.
5. **Duplicate keys**: `log_warn`, but last-one-wins.
6. **Comments**: support both `#` and `//`.
7. **Migration**: ignore the old binary format for now (no backwards compatibility).

Recommendation for (1):

- **Short term (Phase 2–3)**: keep C++ as the authority for defaults while we stabilize the runtime format.
- **Long term (Phase 4+)**: make the schema/DSL authoritative and generate the C++ tables, so version annotations/defaults/types are truly maintained in one place.

---

## High-level approach

### Defaults live in code; file stores overrides

- The **code owns**:
  - Default values
  - Types
  - Field lifecycle metadata (since/until version)
- The on-disk settings file stores **only user overrides** (the “diff” from defaults).

This naturally satisfies “skip unchanged”: if something equals the default, it is simply not written.

Optionally, support a debug “full dump” mode that writes every key, but still uses `*` to indicate overrides.

---

## How we implement this (recommended): schema/DSL + codegen

If we want `@vN` / `@vN-M` annotations to be easy to update as the format evolves, the cleanest approach is to keep a **single source of truth** schema file and generate:

- C++ definitions / metadata tables (keys, types, defaults, lifecycle)
- A parser/serializer for the on-disk text format (or at least the table the parser uses)
- Optional documentation output (e.g. a generated “all settings + defaults” markdown)

### Proposed schema file

Add a small DSL file (or YAML/TOML) committed to the repo, e.g.:

- `resources/config/settings.schema` (DSL)
- or `resources/config/settings.schema.yaml` (YAML)

This schema contains:

- **current schema version** (`version: 5`)
- **settings list**: section, key name, type, default value
- **lifecycle**: `since`, optional `until` (maps directly to `@v2` and `@v3-4`)

Example DSL (illustrative):

```text
version: 5;

[audio]
master_volume : f32 = f32(0x3F000000); @v1
music_volume  : f32 = f32(0x3F000000); @v1
sound_volume  : f32 = f32(0x3F000000); @v1

[video]
fullscreen : bool = false;            @v1
vsync      : bool = true;             @v1-5
resolution : i32x2 = i32x2(1280,720); @v1

[ui]
lang_name : str = str("en_us");       @v1
ui_theme  : str = str("");            @v2
```

Notes:

- `@vN` means `since=N`.
- `@vA-B` means `since=A, until=B`.
- The typed literals for defaults match the runtime format, which keeps parsing consistent.

### Code generation flow

- A small generator (likely a `scripts/*.py` tool) reads the schema and writes generated files like:
  - `src/generated/settings_schema.h` (enum/IDs + metadata table)
  - `src/generated/settings_schema.cpp` (defaults + lifecycle info)
  - optionally `docs/generated_settings_schema.md`
- The runtime settings loader:
  - Reads the on-disk `settings` file (overrides-only)
  - Checks `version` matches `CURRENT_SETTINGS_VERSION` (which can be generated from the schema)
  - For each `key* = value;`:
    - Look up key in generated metadata table
    - Verify key is valid for this version (`since/until`)
    - Parse typed literal into the expected C++ type
    - Apply to in-memory settings

### Why a schema/DSL helps

- **Version annotations live in one place** (the schema), not scattered across parsing code.
- Adding/removing/renaming keys becomes an **edit to one file**, then regenerate.
- The schema can also enforce determinism (ordering, canonical names).

---

## Multi-phase implementation plan

This plan is intentionally incremental: early phases can ship value (human-readable, versioned settings) while later phases reduce maintenance overhead (codegen, validation, tests).

### Phase 0 — Decide scope + file conventions (1 short PR)

**Decisions**

- **On-disk filename/extension** (recommended: `settings.psettings` or `settings.cfg` instead of `settings.bin`)
- **Canonical persistence policy**: overrides-only file vs full dump
- **Schema version source of truth**: generated from schema file (recommended)

**Deliverables**

- Update this doc with the chosen filename and conventions.
- Add a stub schema file path (even if empty) so it’s part of the repo.

**Done when**

- There is an agreed file name and format contract.

---

### Phase 1 — Introduce the schema file (no runtime behavior change yet)

**Goal**: create the single source of truth for settings keys + defaults + lifecycle.

**Work**

- Add `resources/config/settings.schema` (or `.yaml`) containing:
  - `version: N`
  - Sections + keys + types + defaults
  - Lifecycle annotations (`@vN`, `@vA-B`)
- Mirror the current runtime settings (`settings::Data`) so the schema covers all existing user-configurable fields.

**Deliverables**

- Schema file committed and reviewed.

**Done when**

- Every current persisted setting has an entry in the schema.

---

### Phase 2 — Build a minimal runtime parser/writer (manual mapping OK)

**Goal**: validate the text format end-to-end before investing in codegen.

**Work**

- Implement:
  - Tokenization for: comments, sections, `version:`, `key[*] = value;`
  - Typed literal parsing for the types you need now (`bool`, `i32`, `str`, `f32`, `i32x2`)
  - Float bit-cast helpers (`f32(0x...)` ⇄ float)
- Implement a writer that emits deterministic ordering and overrides-only output.

**Integration**

- Add new code paths:
  - `Settings::load_save_file_text()` / `Settings::write_save_file_text()`
- Keep the old binary path intact for now (feature flag / temporary toggle).

**Deliverables**

- A new settings text file can be written and loaded locally.

**Done when**

- Round-trip works: write → read → settings state matches.
- Invalid syntax fails safely (defaults) with a clear warning.

---

### Phase 3 — Switch production to the text format (break old files intentionally)

**Goal**: make the new text file the canonical persisted settings.

**Work**

- Change the configured settings filename from `settings.bin` to the new text name.
- Replace `load_save_file()` / `write_save_file()` to use the text format only.
- Implement version rule:
  - If file version != `CURRENT_SETTINGS_VERSION`: discard and regenerate defaults.

**Deliverables**

- Shipping build reads/writes the new settings file.

**Done when**

- The app runs cleanly starting from:
  - No settings file
  - A valid settings file
  - An invalid or wrong-version settings file

---

### Phase 4 — Add generator + generated schema table (remove manual mapping)

**Goal**: stop duplicating keys/types/defaults between schema and C++.

**Work**

- Create a generator script (e.g. `scripts/gen_settings_schema.py`) that reads the schema and outputs:
  - `src/generated/settings_schema.h/.cpp`
  - Optional `docs/generated_settings_schema.md`
- Update runtime parsing to:
  - Resolve keys via generated table
  - Enforce expected type per key
  - Enforce lifecycle (`since/until`) for the active version
- Update writer to iterate the generated table for stable ordering.

**Deliverables**

- Generated files checked in (or generated at build—pick one and document it).

**Done when**

- Adding a new setting requires only:
  - Editing the schema
  - Re-running generator

---

### Phase 5 — Hardening: validation, UX, and tests

**Goal**: make it robust and maintainable.

**Work**

- Add validation improvements:
  - Unknown keys: ignore + log once (with suggestion if similar key exists)
  - Type mismatch: ignore assignment + warn
  - Duplicate keys: last-one-wins (or error; choose and document)
- Add automated tests:
  - Parser unit tests (valid/invalid cases)
  - Float hex round-trip tests
  - Version mismatch behavior test
- Add a “dump full settings” debug action for troubleshooting (optional).

**Done when**

- Tests cover the parser and float encoding.
- The loader never leaves settings partially-applied on parse failure.

---

## On-disk file format (line-based text)

### Header

Every file begins with a schema version:

```text
version: 5;
```

### Sections (optional)

Sections are for readability only and do not affect parsing:

```text
[audio]
[video]
[ui]
```

### Assignments

Each setting is a single assignment terminated by `;`:

```text
key = value;
```

### Override marker (`*`)

Use `*` to mark keys that should be applied (i.e., are not the default):

```text
key* = value;
```

Recommended convention:

- **Canonical persisted file**: overrides-only → every assignment is starred.
- **Full dump file** (debug): all keys present, only non-default assignments starred.

---

## Value encoding (typed literals)

To keep parsing simple and type-safe, values use explicit typed literals.

### Literal set (v1 / focus for initial implementation)

Implement a small, explicit set of literals first (enough to cover current settings):

- `bool`: `true` / `false`
- `i32(n)`: signed 32-bit integer
- `str("...")`: UTF-8 string with minimal escaping
- `f32(0xXXXXXXXX)`: 32-bit float stored as IEEE-754 hex bits (exact)
- `i32x2(a, b)`: 2-tuple for resolution-like values (e.g. width/height)

Everything else is out of scope until we have a concrete need.

### Floats (exact)

Store floats as IEEE-754 bits in hex:

- `f32(0xXXXXXXXX)` for 32-bit floats
- (optional) `f64(0xXXXXXXXXXXXXXXXX)` for 64-bit doubles

Example:

```text
master_volume* = f32(0x3F000000);   # 0.5
```

### Other primitives

```text
fullscreen* = true;
LOG_LEVEL*  = i32(3);
lang_name*  = str("en_us");
```

### Strings and escaping

Based on current `resources/config/*.json`, strings are mostly:

- Identifiers (e.g. `"GAMEPAD_AXIS_LEFT_Y"`, `"en_us"`, `"pharmasea"`)
- Filenames/paths (e.g. `"constan.ttf"`, `"models/kennynl"`)

So we can start with **minimal escaping**:

- `\"` for quotes
- `\\` for backslash
- `\n`, `\t` (optional but easy)

Unicode escaping can be deferred unless we actually need it.

### Small structs (example)

```text
resolution* = i32x2(1920, 1080);
color*      = u8x4(237, 230, 227, 255);
```

The exact set of supported typed literals should match the setting schema in code.

---

## Example: canonical overrides-only file

```text
version: 5;

[audio]
master_volume* = f32(0x3F000000);      # 0.5
music_volume*  = f32(0x3E99999A);      # 0.3
sound_volume*  = f32(0x3F19999A);      # 0.6

[video]
fullscreen* = true;
vsync*      = true;
resolution* = i32x2(1920, 1080);

[ui]
lang_name* = str("en_us");
ui_theme*  = str("pharmasea");
```

---

## Versioning strategy (no backwards compatibility required)

Define a single constant in code:

- `CURRENT_SETTINGS_VERSION`

Load rules:

- If file is missing/unreadable → **use defaults**
- If `version != CURRENT_SETTINGS_VERSION` → **discard file and regenerate** (defaults)
- If version matches → parse and apply only `*` entries

This keeps the system simple: version bumps intentionally invalidate older files.

---

## Field lifecycle metadata (`@vN`, `@v3-4`)

Instead of embedding lifecycle tags in the user’s saved settings file, keep them in code (or generate a schema doc for developers):

- **since**: version where the key was introduced
- **until** (optional): version where the key stops being accepted

Loader behavior:

- Unknown keys → ignore + log once (helps debugging typos/manual edits)
- Known but out-of-range for this version → ignore (optionally warn)

This preserves the intent of:

```text
example_field       @v1
versiontwofield     @v2
deprecated_value    @v3-4
```

…without polluting the persisted user file.

---

## Determinism and diff-friendliness

- Write keys in a stable, deterministic order (e.g., by section then key name).
- Use a single canonical spelling for keys.
- Prefer one setting per line, always terminated with `;`.

---

## Summary

This design matches the original notes:

- **Version number**: `version: N;`
- **Float precision**: `f32(0x...)`
- **Default skipping**: `*` marks overrides (and the canonical file contains only overrides)
- **Versioned schema**: lifecycle tags live in code; old files can be invalidated on version bumps

---

## Out of scope (for now)

Replacing `resources/config/*.json` with this format would require richer data model support (arrays/objects, etc.). For now we’re focusing on **the literal set above** so the settings loader/writer can ship quickly.

