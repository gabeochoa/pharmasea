#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "../globals.h"
#include "../map.h"
#include "../zpp_bits_include.h"

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
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        // NOTE: we serialize magic first so quick header reads can fail fast.
        (void) archive(                   //
            self.magic,                   //
            self.save_version,            //
            self.hashed_build_version,    //
            self.timestamp_epoch_seconds  //
        );

        return archive(            //
            self.display_name,     //
            self.seed,             //
            self.day_count,        //
            self.coins,            //
            self.cart,             //
            self.playtime_seconds  //
        );
    }
};

struct SaveGameFile {
    SaveGameHeader header;
    Map map_snapshot;

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(        //
            self.header,       //
            self.map_snapshot  //
        );
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
