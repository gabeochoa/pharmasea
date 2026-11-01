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
  - inline comments after statements (e.g. `...; // comment`)

### `version:` meaning (schema/versioning)

`version: N;` is the **PSCFG file-format/schema version**.

- When we introduce new keys, change literal rules, or otherwise change what the file can express, we bump this `version`.
- Each key/value can also carry lifecycle metadata:
  - version added
  - version deprecated (optional)
  - version removed (optional; not supported/accepted at/after this)

#### Removal semantics (important)

To avoid off-by-one confusion, treat “removed” as **exclusive**:

- A key with `removed_in_version = 5` is valid only for PSCFG versions \(< 5\).
- In other words: it is valid up through **version 4**, and invalid starting at **version 5**.

Because this field is really “the first version where it stops working”, a clearer name than `version_removed` is:

- `removed_in_version` (recommended), or equivalently
- `until_version_exclusive`

### Grammar (informal)

```text
file        := (ws | comment | stmt)*
stmt        := version_stmt | section_stmt | assign_stmt
version_stmt:= "version" ":" int ";"
section_stmt:= "[" section_name "]"
assign_stmt := key ("*")? ws? "=" ws? literal ";"

key         := ident            # snake_case recommended
ident       := [a-zA-Z_][a-zA-Z0-9_]*
section_name:= any chars except ']' (single-line)
int         := ["-"]?[0-9]+
comment     := "#" ... EOL | "//" ... EOL
```

### Sections

- Sections are **case-sensitive**.
- Section names may include **spaces** and **dashes**.
- If the loader sees multiple section names that differ only by case (e.g. `[Video]` vs `[video]`), it should `log_warn`.
- If the same section name appears multiple times, treat it as **one logical section** (statements still apply top-to-bottom in file order).

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

Strings may contain comment-like tokens (e.g. `str("//")`); comment parsing must not trigger inside strings.

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
   - If multiple `version:` statements are present → `log_error` and treat the file as invalid → **use defaults**.
3. If `version != CURRENT_SETTINGS_VERSION` → `log_warn`, discard file → **use defaults**.
4. Parse statements in order:
   - Section headers only affect grouping/debug logging (optional).
   - Assignments:
     - If key is unknown → `log_warn` and ignore.
     - Lifecycle handling:
       - If key is deprecated in this version → `log_warn`, but **accept** it.
       - If key is removed in this version → `log_error` and ignore.
     - If key appears multiple times → `log_warn`, **last one wins**.
     - If a line is malformed (syntax error, bad literal, etc.) → `log_warn`, **skip the line** and continue.
     - If assignment is not starred (a “non-star assignment”):
       - This means the line is a plain `key = literal;` without the `*`.
       - In PSCFG, `*` is what marks “user override”. So non-star assignments should be treated as **non-overrides** and ignored by default (optionally `log_warn` once to help catch user mistakes).
     - If starred:
       - Parse the literal.
       - If the literal type does not match the expected key type → `log_warn` and **keep the default** (skip applying the override).
       - Otherwise apply the override.

### Write algorithm

- Goal: **preserve user formatting** as much as practical.
- Always ensure `version: CURRENT_SETTINGS_VERSION;` exists at the top (update in place if present).
- Update/insert **only the lines that changed**; avoid rewriting unrelated lines.
- Keep inline and full-line comments intact when possible.
- When writing a brand-new file, emit a canonical, deterministic ordering.

---

## Versioning + lifecycle annotations (schema-side)

We want the `@vN` / `@vA-B` concept from the notes, but we don’t want those tags in the user’s saved file.

Instead, store lifecycle metadata alongside the schema (in code now; in a DSL later):

- `since`: first PSCFG version the key exists (inclusive)
- `deprecated_since` (optional): first PSCFG version the key is deprecated (inclusive; still accepted with `log_warn`)
- `removed_in_version` (optional): first PSCFG version the key is removed (exclusive end; not accepted at/after this)

Loader uses this metadata to ignore deprecated keys for the active version.

### Renames

To keep the system simple, we do not support aliases in v1.

- Rename is modeled as: **removed old key + added new key**.
- Old key (if present in a file) will warn (and error once removed), and the new key is the supported replacement.

---

## Schema representation in code (near-term)

Instead of a separate DSL file immediately, we can represent the schema in C++ with a small set of structs and use it for:

- key lookup
- lifecycle checks (added/deprecated/removed)
- type validation
- defaults (near-term)

Concept sketch (shape):

- `SettingsValue { name, version_added, version_deprecated, removed_in_version }`
- `SettingsSection { name, values[] }`

Example entry:

- `{ "is_fullscreen", v1, null, null }` (never removed)
- `{ "old_value", v2, v3, v5 }` (deprecated in v3, removed starting v5)

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

