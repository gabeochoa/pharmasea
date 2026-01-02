#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "../bitsery_include.h"
#include "../globals.h"
#include "../map.h"

namespace save_game {

namespace fs = std::filesystem;

// Keep this small; it should be quick to read without deserializing the world.
struct SaveGameHeader {
    // Versioning / compatibility
    uint32_t save_version = 1;
    uint64_t hashed_build_version = HASHED_VERSION;

    // Metadata (best-effort; may be 0 / empty)
    int64_t timestamp_epoch_seconds = 0;
    std::string display_name;

    std::string seed;
    int32_t day_count = 0;
    int32_t coins = 0;
    int32_t cart = 0;
    int32_t playtime_seconds = 0;

    // File format marker (kept inside header for now).
    std::string magic = "PHARMSAVE";

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        // NOTE: we serialize magic first so quick header reads can fail fast.
        s.text1b(magic, 16);
        s.value4b(save_version);
        s.value8b(hashed_build_version);
        s.value8b(timestamp_epoch_seconds);

        s.text1b(display_name, 64);
        s.text1b(seed, MAX_SEED_LENGTH);
        s.value4b(day_count);
        s.value4b(coins);
        s.value4b(cart);
        s.value4b(playtime_seconds);
    }
};

struct SaveGameFile {
    SaveGameHeader header;
    Map map_snapshot;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.object(header);
        s.object(map_snapshot);
    }
};

struct SlotInfo {
    int slot = 0;  // 1-based
    bool exists = false;
    std::optional<SaveGameHeader> header;
    fs::path path;
};

struct SaveGameManager {
    static constexpr int kDefaultNumSlots = 8;

    [[nodiscard]] static fs::path saves_folder();
    [[nodiscard]] static bool ensure_saves_folder_exists();

    [[nodiscard]] static fs::path slot_path(int slot);
    [[nodiscard]] static bool delete_slot(int slot);

    // Snapshot-based save/load.
    [[nodiscard]] static bool save_slot(int slot, const Map& authoritative_map);
    [[nodiscard]] static bool load_slot(int slot, SaveGameFile& out);
    [[nodiscard]] static bool load_file(const fs::path& path,
                                        SaveGameFile& out);

    // Read only the header (fast).
    [[nodiscard]] static std::optional<SaveGameHeader> read_header(
        const fs::path& path);

    [[nodiscard]] static std::vector<SlotInfo> enumerate_slots(
        int num_slots = kDefaultNumSlots);
};

}  // namespace save_game
