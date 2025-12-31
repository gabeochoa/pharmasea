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

