#include "save_game.h"

#include <chrono>
#include <fstream>
#include <sstream>

#include "../components/all_components.h"
#include "../components/has_day_night_timer.h"
#include "../components/is_bank.h"
#include "../engine/files.h"
#include "../engine/log.h"
#include "../external_include.h"

// Acknowledgement gate:
// If you change `snapshot_blob::ComponentTypes` (add/reorder/delete), you MUST
// update this expected value to acknowledge the save/replay/network format
// impact.
static constexpr std::uint64_t kExpectedComponentTypesChecksum =
    0xde64e3f368746217ull;
static_assert(snapshot_blob::kComponentTypesChecksum ==
                  kExpectedComponentTypesChecksum,
              "ComponentTypes checksum changed. Update "
              "kExpectedComponentTypesChecksum to acknowledge.");

namespace save_game {

namespace {
using Buffer = std::string;

[[nodiscard]] std::optional<std::string> read_file_to_string(
    const fs::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) return std::nullopt;
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

[[nodiscard]] bool write_string_to_file_atomic(const fs::path& path,
                                               const std::string& data) {
    // Ensure folder exists.
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    if (ec) {
        log_warn("save_game: failed to create dirs for {} err={}",
                 path.parent_path().string(), ec.message());
        return false;
    }

    fs::path tmp = path;
    tmp += ".tmp";

    {
        std::ofstream ofs(tmp, std::ios::binary);
        if (!ofs.is_open()) {
            log_warn("save_game: failed to open temp file {}", tmp.string());
            return false;
        }
        ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
        ofs.flush();
        if (!ofs.good()) {
            log_warn("save_game: write failed to {}", tmp.string());
            return false;
        }
    }

    fs::rename(tmp, path, ec);
    if (ec) {
        log_warn("save_game: rename {} -> {} failed: {}", tmp.string(),
                 path.string(), ec.message());
        // Best-effort cleanup.
        fs::remove(tmp, ec);
        return false;
    }
    return true;
}

template<typename T>
[[nodiscard]] std::optional<T> deserialize_one_object_prefix(
    const std::string& buf) {
    T obj{};
    zpp::bits::in in{buf};
    if (auto result = in(  //
            obj  //
            );
        zpp::bits::failure(result)) {
        return std::nullopt;
    }
    return obj;
}

[[nodiscard]] int64_t now_epoch_seconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch())
        .count();
}
}  // namespace

fs::path SaveGameManager::saves_folder() {
    return Files::get().game_folder() / fs::path("saves");
}

bool SaveGameManager::ensure_saves_folder_exists() {
    std::error_code ec;
    fs::create_directories(saves_folder(), ec);
    if (ec) {
        log_warn("save_game: failed creating saves folder {} err={}",
                 saves_folder().string(), ec.message());
        return false;
    }
    return true;
}

fs::path SaveGameManager::slot_path(int slot) {
    // 1-based.
    if (slot < 1) slot = 1;
    // Format: slot_01.bin, slot_02.bin, ...
    return saves_folder() / fs::path(fmt::format("slot_{:02d}.bin", slot));
}

bool SaveGameManager::delete_slot(int slot) {
    std::error_code ec;
    fs::path path = slot_path(slot);
    bool existed = fs::exists(path, ec);
    if (ec) {
        log_warn("save_game: exists() failed for {} err={}", path.string(),
                 ec.message());
    }
    if (!existed) return true;

    fs::remove(path, ec);
    if (ec) {
        log_warn("save_game: delete failed {} err={}", path.string(),
                 ec.message());
        return false;
    }
    return true;
}

std::optional<SaveGameHeader> SaveGameManager::read_header(
    const fs::path& path) {
    auto data = read_file_to_string(path);
    if (!data.has_value()) return std::nullopt;
    auto header_opt = deserialize_one_object_prefix<SaveGameHeader>(*data);
    if (!header_opt.has_value()) return std::nullopt;
    if (header_opt->magic != "PHARMSAVE") return std::nullopt;
    return header_opt;
}

bool SaveGameManager::save_slot(int slot, const Map& authoritative_map) {
    if (!ensure_saves_folder_exists()) return false;

    SaveGameFile file;
    file.map_snapshot = authoritative_map;

    // Header (best-effort fields).
    file.header.timestamp_epoch_seconds = now_epoch_seconds();
    file.header.seed = file.map_snapshot.seed;
    file.header.hashed_build_version = HASHED_VERSION;

    // Fill a few commonly useful preview fields from the snapshot.
    for (const auto& ent_ptr : EntityHelper::get_entities()) {
        if (!ent_ptr) continue;
        const Entity& e = *ent_ptr;
        if (e.has<HasDayNightTimer>()) {
            file.header.day_count = e.get<HasDayNightTimer>().days_passed();
        }
        if (e.has<IsBank>()) {
            const IsBank& bank = e.get<IsBank>();
            file.header.coins = bank.balance();
            file.header.cart = bank.cart();
        }
    }

    // NOTE: display_name/playtime are optional; leaving default for Phase 1.

    // Serialize.
    Buffer buffer;
    zpp::bits::out out{buffer};
    if (auto result = out(  //
            file  //
            );
        zpp::bits::failure(result)) {
        log_warn("save_game: serialize failed slot={} err={}", slot, (int) result);
        return false;
    }

    fs::path path = slot_path(slot);
    return write_string_to_file_atomic(path, buffer);
}

bool SaveGameManager::load_slot(int slot, SaveGameFile& out) {
    fs::path path = slot_path(slot);
    return load_file(path, out);
}

bool SaveGameManager::load_file(const fs::path& path, SaveGameFile& out) {
    auto data = read_file_to_string(path);
    if (!data.has_value()) return false;

    zpp::bits::in in{*data};
    if (auto result = in(  //
            out  //
            );
        zpp::bits::failure(result)) {
        log_warn("save_game: load_file {} decode_err={}", path.string(),
                 (int) result);
        return false;
    }

    if (out.header.magic != "PHARMSAVE") {
        log_warn("save_game: bad magic in {}", path.string());
        return false;
    }
    return true;
}

std::vector<SlotInfo> SaveGameManager::enumerate_slots(int num_slots) {
    if (num_slots < 1) num_slots = 1;

    std::vector<SlotInfo> out;
    out.reserve(num_slots);
    for (int i = 1; i <= num_slots; ++i) {
        SlotInfo info;
        info.slot = i;
        info.path = slot_path(i);
        std::error_code ec;
        info.exists = fs::exists(info.path, ec);
        if (!ec && info.exists) {
            info.header = read_header(info.path);
        }
        out.push_back(std::move(info));
    }
    return out;
}

}  // namespace save_game
