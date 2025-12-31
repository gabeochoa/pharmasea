#!/usr/bin/env python3
"""
Phase 0 guardrail: detect pointer-based serialization in snapshot paths.

This is intentionally a *narrow* audit that focuses on files that define
network/save snapshot formats for PharmaSea.

How this is meant to be used:
- Early in the migration (today), the repo still contains pointer-linking and
  smart-ptr graph serialization in snapshot paths. This script should still be
  runnable and useful without breaking developer workflows.
- Therefore this script supports a *baseline lock*:
  - By default, it FAILS only if *new* violations appear compared to the
    checked-in baseline list.
  - If you set STRICT=1, it FAILS if *any* violations exist.

Update the baseline by running:
  UPDATE_BASELINE=1 python3 scripts/check_pointer_free_serialization.py
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
BASELINE_PATH = ROOT / "scripts" / "pointer_free_serialization_audit_baseline.txt"


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


def load_baseline() -> list[str]:
    if not BASELINE_PATH.exists():
        return []
    with BASELINE_PATH.open("r", encoding="utf-8") as f:
        lines = [ln.strip() for ln in f.readlines()]
    return [ln for ln in lines if ln and not ln.startswith("#")]


def write_baseline(violations: list[str]) -> None:
    BASELINE_PATH.parent.mkdir(parents=True, exist_ok=True)
    with BASELINE_PATH.open("w", encoding="utf-8") as f:
        f.write(
            "# Baseline for scripts/check_pointer_free_serialization.py\n"
            "#\n"
            "# This file intentionally lists current known violations so the audit can\n"
            "# be used as a regression detector before the migration is complete.\n"
            "#\n"
            "# To update:\n"
            "#   UPDATE_BASELINE=1 python3 scripts/check_pointer_free_serialization.py\n"
            "\n"
        )
        for v in sorted(set(violations)):
            f.write(v + "\n")


def main() -> int:
    strict = os.environ.get("STRICT", "0") == "1"
    update_baseline = os.environ.get("UPDATE_BASELINE", "0") == "1"

    violations = iter_violations(RULES)

    if update_baseline:
        write_baseline(violations)
        print(f"Pointer-free serialization audit baseline updated: {BASELINE_PATH}")
        return 0

    if strict:
        if violations:
            print("Pointer-free serialization audit: FAIL (STRICT=1)\n")
            for v in violations:
                print(f"- {v}")
            return 1
        print("Pointer-free serialization audit: PASS (STRICT=1)")
        return 0

    # Baseline-locked mode (default): only fail if violations changed.
    baseline = load_baseline()
    cur = sorted(set(violations))
    base = sorted(set(baseline))
    if cur != base:
        print("Pointer-free serialization audit: FAIL (baseline changed)\n")
        print(f"Baseline file: {BASELINE_PATH}\n")
        if base:
            print("Baseline violations:")
            for v in base:
                print(f"- {v}")
        else:
            print("Baseline violations: (none)")
        print("\nCurrent violations:")
        for v in cur:
            print(f"- {v}")
        print(
            "\nIf this change is expected, update the baseline with:\n"
            "  UPDATE_BASELINE=1 python3 scripts/check_pointer_free_serialization.py"
        )
        return 1

    print("Pointer-free serialization audit: PASS (baseline unchanged)")
    return 0


if __name__ == "__main__":
    # Ensure stable relative behavior even if invoked from elsewhere.
    os.chdir(ROOT)
    raise SystemExit(main())

