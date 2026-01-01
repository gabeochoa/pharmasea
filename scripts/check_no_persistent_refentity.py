#!/usr/bin/env python3
"""
Guardrail: flag long-lived storage of afterhours reference-wrapper types.

Why:
  afterhours::RefEntity and afterhours::OptEntity are convenient, but they
  encode direct references. Storing them in long-lived structs/components is a
  common source of dangling-ref bugs once entity lifetime changes (handles,
  slot reuse, cleanup compaction).

What this does:
  A lightweight regex scan over headers in src/ that flags struct/class fields
  that appear to store RefEntity/OptEntity as members.

Notes:
  - This is intentionally conservative and may miss some cases.
  - It can also false-positive (e.g. in comments). Keep patterns narrow.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

# Heuristic: lines that look like member declarations. We intentionally avoid
# matching function parameters and return types by requiring a trailing ';'.
PATTERNS: tuple[tuple[str, re.Pattern[str]], ...] = (
    ("RefEntity member", re.compile(r"\bRefEntity\b\s+\w+\s*;")),
    ("OptEntity member", re.compile(r"\bOptEntity\b\s+\w+\s*;")),
)


def iter_headers() -> list[Path]:
    out: list[Path] = []
    for p in SRC.rglob("*.h"):
        out.append(p)
    return out


def main() -> int:
    violations: list[str] = []
    for path in iter_headers():
        rel = path.relative_to(ROOT)
        txt = path.read_text(encoding="utf-8", errors="replace")
        for name, pat in PATTERNS:
            for m in pat.finditer(txt):
                # Provide a little context: line number.
                line_no = txt.count("\n", 0, m.start()) + 1
                violations.append(f"{rel}:{line_no}: {name}")

    if violations:
        print("Persistent RefEntity/OptEntity guardrail: FAIL\n")
        for v in sorted(set(violations)):
            print(f"- {v}")
        print(
            "\nRecommended fix: store an EntityHandle/EntityID (persistent key), "
            "and resolve to RefEntity/OptEntity only at point-of-use."
        )
        return 1

    print("Persistent RefEntity/OptEntity guardrail: PASS")
    return 0


if __name__ == "__main__":
    os.chdir(ROOT)
    raise SystemExit(main())

