#!/bin/sh
# Capture day/night lighting screenshots and send them to Claude
# (via the user's `cc` alias) for a lighting style review.
# Usage: sh tests/e2e/run_lighting_review.sh [--no-send]
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
EXE=${PHARMASEA_EXE:-$ROOT/pharmasea.exe}
DAY=$ROOT/tests/e2e/lighting_day.png
NIGHT=$ROOT/tests/e2e/lighting_night.png
SEND=1
if [ "${1:-}" = "--no-send" ]; then SEND=0; fi

rm -f "$DAY" "$NIGHT"
timeout "${E2E_TIMEOUT:-240}" "$EXE" --local --e2e tests/e2e/15_lighting_day_night.e2e

test -s "$DAY" || { echo "missing day screenshot: $DAY" >&2; exit 1; }
test -s "$NIGHT" || { echo "missing night screenshot: $NIGHT" >&2; exit 1; }
echo "[PASS] captured $DAY"
echo "[PASS] captured $NIGHT"

if [ "$SEND" -eq 0 ]; then
    echo "skipping send (--no-send)"
    exit 0
fi

command -v zsh >/dev/null 2>&1 || {
    echo "zsh is required so the cc alias expands" >&2
    exit 1
}

PROMPT="Use the Read tool to inspect these two game screenshots, in order: $DAY (day) and $NIGHT (night). They show the same overlook with the shader lighting enabled. The current lighting is considered ugly and has been disabled by default. Recommend a concrete visual style direction for day vs night (palette, ambient/directional balance, indoor vs outdoor treatment, bar light spill) and list specific, implementable tweaks for a Half-Lambert + Blinn-Phong shader with indoor point lights. Be concise and actionable; do not edit any files."

export LIGHTING_PROMPT="$PROMPT"
export LIGHTING_ROOT="$ROOT"
timeout "${CC_TIMEOUT:-600}" zsh -ic '
    cc -p --permission-mode bypassPermissions \
        --add-dir "$LIGHTING_ROOT" \
        -n lighting-review \
        "$LIGHTING_PROMPT"
'
