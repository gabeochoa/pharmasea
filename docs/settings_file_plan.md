# PSCFG: Versioned Settings File Format (non-JSON)

Source notes: `serializing_notes.md` (lines 1–22).

This plan defines **PSCFG**, a **versioned**, **human-editable**, **diff-friendly** settings file format that:

- Stores **floats exactly** via IEEE-754 hex bits
- Uses `*` to mark **overrides** (non-default values)
- Is **not required to be backwards compatible**

---

## Status / decisions (current)

1. **File name/location**: keep the existing save-games location and filename (`settings.bin`) for now (even if the contents become text).
2. **Key naming**: snake_case.
3. **Comments**: support both `#` and `//`.
4. **Unknown/deprecated keys**: ignore and `log_warn`.
5. **Duplicate keys**: `log_warn`, last-one-wins.
6. **Migration**: ignore the old binary format for now.
7. **Defaults source of truth**:
   - **Near-term**: C++ remains authoritative while the runtime format stabilizes.
   - **Later**: schema/DSL becomes authoritative + codegen (optional, Phase 4).

---

## Naming

- **Format name**: **PSCFG**
- **Recommended extension**: `.pscfg`
- **Current on-disk filename**: still `settings.bin` (per current decision), even though the contents will be PSCFG text.

---

## Core idea

- **Defaults live in code**.
- The on-disk settings file stores **only overrides** (the “diff” from defaults).
- `*` is the explicit “this value is set by the user” marker.

This matches the original intent: quickly skip whole categories / keys that are unchanged.

---

## File format specification (v1)

### Structure

- One file contains:
  - a required `version: <int>;` header
  - optional section headers `[section_name]`
  - zero or more assignments `key[*] = literal;`
- Whitespace is insignificant except inside strings.
- Comments are supported:
  - `# comment`
  - `// comment`

### Grammar (informal)

```text
file        := (ws | comment | stmt)*
stmt        := version_stmt | section_stmt | assign_stmt
version_stmt:= "version" ":" int ";"
section_stmt:= "[" ident "]"
assign_stmt := key ("*")? ws? "=" ws? literal ";"

key         := ident            # snake_case recommended
ident       := [a-zA-Z_][a-zA-Z0-9_]*
int         := ["-"]?[0-9]+
comment     := "#" ... EOL | "//" ... EOL
```

### Canonical persistence convention

- **Persisted file**: overrides-only → every assignment is starred.
- Optional debug mode later: “full dump” (all keys), but only overrides starred.

---

## Literal set (v1) — focus for initial implementation

Keep the literal set intentionally small so the parser is easy to get correct.

### Supported literals

- **bool**: `true` / `false`
- **i32**: `i32(<signed-int>)`
- **str**: `str("...")`
- **f32 (exact)**: `f32(0xXXXXXXXX)` where `XXXXXXXX` are the raw IEEE-754 bits
- **i32x2**: `i32x2(a, b)` (e.g. resolution width/height)

Everything else is out of scope until we have a concrete need.

### Float exactness

`f32(0x...)` must round-trip exactly:

```text
master_volume* = f32(0x3F000000); // 0.5
```

### Strings and escaping (minimal)

Start with minimal escapes:

- `\"` quote
- `\\` backslash
- optionally `\n`, `\t`

Unicode escaping can be deferred until there’s a real need.

---

## Example: overrides-only settings file

```text
version: 5;

[audio]
master_volume* = f32(0x3F000000); // 0.5
music_volume*  = f32(0x3E99999A); // 0.3

[video]
is_fullscreen* = true;
resolution*    = i32x2(1920, 1080);
vsync_enabled* = true;

[ui]
lang_name* = str("en_us");
ui_theme*  = str("pharmasea");
```

---

## Load / write semantics

### Load algorithm

1. If the file does not exist / cannot be read → **use defaults**.
2. Parse `version`.
3. If `version != CURRENT_SETTINGS_VERSION` → `log_warn`, discard file → **use defaults**.
4. Parse statements in order:
   - Section headers only affect grouping/debug logging (optional).
   - Assignments:
     - If key is unknown → `log_warn` and ignore.
     - If key is known but not valid for this version (deprecated) → `log_warn` and ignore.
     - If key appears multiple times → `log_warn`, **last one wins**.
     - If assignment is not starred:
       - In overrides-only mode: ignore it (or warn once). (We can choose strictness later.)
     - If starred: parse literal, validate type, apply override.

### Write algorithm

- Emit deterministic ordering (e.g. by section then key).
- Emit **only overrides** (starred assignments).
- Always write `version: CURRENT_SETTINGS_VERSION;` at the top.

---

## Versioning + lifecycle annotations (schema-side)

We want the `@vN` / `@vA-B` concept from the notes, but we don’t want those tags in the user’s saved file.

Instead, store lifecycle metadata alongside the schema (in code now; in a DSL later):

- `since`: first version the key exists
- `until` (optional): last version the key is accepted

Loader uses this metadata to ignore deprecated keys for the active version.

---

## Schema/DSL + codegen (optional, later)

Once the format is stable, introduce a single source of truth schema file, e.g. `resources/config/settings.schema`, that contains:

- `version: N`
- section, key, type, default literal
- lifecycle annotation (`@vN` / `@vA-B`)

Then generate C++ tables from it so defaults/types/lifecycle aren’t duplicated.

Illustrative schema DSL:

```text
version: 5;

[audio]
master_volume : f32 = f32(0x3F000000); @v1
music_volume  : f32 = f32(0x3F000000); @v1

[video]
is_fullscreen : bool = false;            @v1
resolution    : i32x2 = i32x2(1280,720); @v1
vsync_enabled : bool = true;             @v1

[ui]
lang_name : str = str("en_us");          @v1
ui_theme  : str = str("");               @v2
```

---

## Multi-phase implementation roadmap

### Phase 0 — Lock the contract (small PR)

- Freeze:
  - filename stays `settings.bin` (for now)
  - literal set v1 (bool, i32, str, f32-hex, i32x2)
  - duplicate/unknown handling (warn + last-one-wins)
  - comment syntax (`#` and `//`)

**Done when**: this doc matches the intended contract.

### Phase 1 — Minimal runtime parser/writer (manual mapping OK)

- Implement parsing for:
  - comments, `version:`, sections, assignments, `*`
  - v1 literals + float bit-cast helpers
- Implement deterministic writer (overrides-only).

**Done when**: write → read round-trip reproduces in-memory settings.

### Phase 2 — Switch settings persistence to text (break old files)

- Update `Settings::load_save_file()` / `write_save_file()` to use the text format.
- Keep filename as `settings.bin`.
- Enforce version rule (mismatch → defaults).

**Done when**: game starts cleanly with no file / good file / bad file / wrong version.

### Phase 3 — Add lifecycle metadata + stricter validation

- Add per-key metadata in code: type + default + `since/until`.
- Enforce type checking per key; warn and ignore mismatches.

**Done when**: loader behavior is predictable for typos, duplicates, wrong literals.

### Phase 4 — Schema/DSL + codegen (optional)

- Add `resources/config/settings.schema`.
- Add generator to produce C++ schema tables.
- Move defaults/types/lifecycle to the generated source of truth.

**Done when**: adding a setting is “edit schema → regenerate”.

---

## Out of scope (for now)

- Replacing `resources/config/*.json` with this format (would require arrays/objects, etc.)

