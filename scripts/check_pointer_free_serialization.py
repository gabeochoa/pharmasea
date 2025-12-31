#!/usr/bin/env python3
"""
Phase 0 guardrail: detect pointer-based serialization in snapshot paths.

This is intentionally a *narrow* audit that focuses on files that define
network/save snapshot formats for PharmaSea.

It fails fast if it finds:
- bitsery pointer-linking context in network/save serializer contexts
- use of StdSmartPtr / pointer extensions in snapshot-critical files
"""

from __future__ import annotations

import os
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class Rule:
    name: str
    rel_paths: tuple[str, ...]
    needles: tuple[str, ...]


ROOT = Path(__file__).resolve().parents[1]


RULES: tuple[Rule, ...] = (
    Rule(
        name="no PointerLinkingContext in network/save contexts",
        rel_paths=(
            "src/network/serialization.h",
            "src/save_game/save_game.cpp",
        ),
        needles=("bitsery::ext::PointerLinkingContext",),
    ),
    Rule(
        name="no StdSmartPtr in world snapshot paths",
        # These files define the serialized world snapshot structures.
        rel_paths=(
            "src/level_info.h",
            "src/entity.h",
        ),
        needles=("bitsery::ext::StdSmartPtr", "StdSmartPtr{"),
    ),
    Rule(
        name="no bitsery pointer extension includes in world snapshot paths",
        rel_paths=("src/entity.h",),
        needles=("#include <bitsery/ext/pointer.h>", "PointerObserver", "PointerOwner"),
    ),
)


def read_text(path: Path) -> str:
    with path.open("r", encoding="utf-8") as f:
        return f.read()


def iter_violations(rules: Iterable[Rule]) -> list[str]:
    failures: list[str] = []
    for rule in rules:
        for rel in rule.rel_paths:
            abs_path = ROOT / rel
            if not abs_path.exists():
                failures.append(f"[{rule.name}] missing file: {rel}")
                continue
            content = read_text(abs_path)
            for needle in rule.needles:
                if needle in content:
                    failures.append(f"[{rule.name}] {rel} contains '{needle}'")
    return failures


def main() -> int:
    failures = iter_violations(RULES)
    if failures:
        print("Pointer-free serialization audit: FAIL\n")
        for f in failures:
            print(f"- {f}")
        print("\nNext step: remove the flagged usage from snapshot paths.")
        return 1

    print("Pointer-free serialization audit: PASS")
    return 0


if __name__ == "__main__":
    # Ensure stable relative behavior even if invoked from elsewhere.
    os.chdir(ROOT)
    raise SystemExit(main())

