#include "../src/world_snapshot_v2.h"

// Compilation-only TU for Cursor builds.
// This file exists so `make -f makefile_cursor cursor-check` can validate that
// snapshot headers stay self-contained and compile.
int cursor_snapshot_compile_smoke() {
    return static_cast<int>(snapshot_v2::kWorldSnapshotVersionV2);
}

