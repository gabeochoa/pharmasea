#!/bin/sh
# Run e2e scripts one process per script (fresh game state each).
# Default: fast set (excludes @slow and @xfail scripts).
# Usage: sh tests/e2e/run_e2e.sh [--slow] [script.e2e ...]
# PHARMASEA_E2E_TIME_SCALE=N multiplies game dt in every run
# (upstream --time-scale); leave unset for real-time runs.
set -u
EXE=${PHARMASEA_EXE:-./pharmasea.exe}
time_scale_args=""
if [ -n "${PHARMASEA_E2E_TIME_SCALE:-}" ]; then
    time_scale_args="--time-scale $PHARMASEA_E2E_TIME_SCALE"
fi
include_slow=0
scripts=""

for arg in "$@"; do
    case "$arg" in
        --slow) include_slow=1 ;;
        *) scripts="$scripts $arg" ;;
    esac
done

if [ -z "$scripts" ]; then
    for f in tests/e2e/*.e2e; do
        if grep -q "@xfail" "$f"; then continue; fi
        if [ "$include_slow" -eq 0 ] && grep -q "@slow" "$f"; then continue; fi
        scripts="$scripts $f"
    done
fi

fail=0
for f in $scripts; do
    echo "=== $f"
    if timeout 900 "$EXE" --local --e2e "$f" $time_scale_args; then
        echo "[PASS] $f"
    else
        echo "[FAIL] $f (exit $?)"
        fail=1
    fi
done
exit $fail
