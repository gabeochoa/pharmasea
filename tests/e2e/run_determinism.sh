#!/bin/sh
# Seed determinism + map-gen sweep (script 07 and the sweep item).
# --generate-map is headless and deterministic per seed, so two runs
# of the same seed must produce byte-identical output, and every
# seed in the sweep must generate without crashing.
# Usage: sh tests/e2e/run_determinism.sh [seed ...]
set -u
EXE=${PHARMASEA_EXE:-./pharmasea.exe}
fail=0

check_seed() {
    a=$("$EXE" --generate-map "$1" 2>/dev/null)
    b=$("$EXE" --generate-map "$1" 2>/dev/null)
    if [ -z "$a" ]; then
        echo "[FAIL] seed $1 produced no map"
        fail=1
    elif [ "$a" != "$b" ]; then
        echo "[FAIL] seed $1 is not deterministic"
        fail=1
    else
        echo "[PASS] seed $1"
    fi
}

if [ $# -gt 0 ]; then
    for seed in "$@"; do check_seed "$seed"; done
else
    for seed in e2e-alpha e2e-beta e2e-serve seed-01 seed-02 seed-03 seed-04 seed-05 seed-06 seed-07; do
        check_seed "$seed"
    done
fi

exit $fail
