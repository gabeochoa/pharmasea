#!/usr/bin/env bash
set -euo pipefail

# Runs the recorded input replay and validates end-state assertions.
#
# Expected usage:
#   scripts/run_replay_validate_completed_first_day.sh /path/to/pharmasea_binary
#
# Recommended build configuration:
# - ENABLE_TESTS=1 (so replay specs register)
# - headless-friendly settings if available (disable models/sound, etc.)

BIN="${1:-}"
if [[ -z "${BIN}" ]]; then
  echo "Usage: $0 /path/to/pharmasea_binary" >&2
  exit 2
fi

if [[ ! -x "${BIN}" ]]; then
  echo "Error: binary not found or not executable: ${BIN}" >&2
  exit 2
fi

exec "${BIN}" \
  --disable-all \
  --replay completed_first_day \
  --replay-validate \
  --exit-on-bypass-complete

