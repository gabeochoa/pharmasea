

#pragma once

#include <string>
#include <type_traits>
#include <vector>

#include "entity_helper.h"
#include "external_include.h"
#include "serialization/world_snapshot_blob.h"

constexpr int MIN_MAP_SIZE = 10;
constexpr int MAX_MAP_SIZE = 25;
constexpr int MAX_SEED_LENGTH = 20;

extern std::vector<std::string> EXAMPLE_MAP;

namespace ps::concepts {

template<typename Archive>
concept HasZppBitsKind = requires { std::remove_cvref_t<Archive>::kind(); };

template<typename Archive>
concept ZppBitsOutArchive =
    HasZppBitsKind<Archive> &&
    (std::remove_cvref_t<Archive>::kind() == zpp::bits::kind::out);

template<typename Archive>
concept ZppBitsInArchive =
    HasZppBitsKind<Archive> &&
    (std::remove_cvref_t<Archive>::kind() == zpp::bits::kind::in);

}  // namespace ps::concepts

struct Map {
    // This gets called on every network frame because
    // the serializer uses the default contructor
    Map() {}

    explicit Map(const std::string& _seed) : seed(_seed) {
        // TODO :NOTE: this is needed for the items to be regenerated
        update_seed(seed);
    }

    // Serialized
    bool showMinimap = false;
    bool was_generated = false;
    game::State last_generated = game::State::InMenu;
    size_t hashed_seed = 0;

    // No serialized
    Entities local_players_NOT_SERIALIZED;
    Entities remote_players_NOT_SERIALIZED;
    std::string seed;
    bool showSeedInputBox = false;

    void update_seed(const std::string& s);

    OptEntity get_remote_with_cui();

    void update_map(const Map& new_map);

    void onUpdate(float dt) {  //
        _onUpdate(remote_players_NOT_SERIALIZED, dt);
    }

    void onUpdateLocalPlayers(float dt);
    void onUpdateRemotePlayers(float dt);

    void _onUpdate(const std::vector<std::shared_ptr<Entity>>& players,
                   float dt);

    void onDraw(float dt) const;
    void onDrawUI(float dt);

    void ensure_generated_map(const std::string& new_seed);

    // called by the server sometimes
    void generate_model_test_map();
    // called by the server sometimes
    void generate_load_save_room_map();

   public:
   private:
    void generate_lobby_map();
    void generate_progression_map();
    void generate_store_map();
    void generate_default_seed();

    void generate_in_game_map();
    auto get_rand_walkable();
    auto get_rand_walkable_register();
    void add_outside_triggers(vec2 origin);

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        // Pointer-free snapshot surface:
        // - We serialize the entire world into a byte blob (no pointer linking).
        // - Then we serialize map metadata (was_generated, seed, hashed_seed).
        // - Then showMinimap.
        //
        // NOTE: This is intentionally a breaking change vs. the historical
        // StdSmartPtr-based entity graph serialization.

        std::string world_blob;
        using ArchiveT = decltype(archive);
        if constexpr (ps::concepts::ZppBitsOutArchive<ArchiveT>) {
            world_blob = snapshot_blob::encode_current_world();
        }

        // Serialize as raw bytes: length + 1-byte chars.
        // (Bitsery's `container4b`/`text4b` mean 4-bytes-per-element, which
        // is NOT what we want for a byte blob.)
        auto result = archive(  //
            world_blob,         //
            self.was_generated, //
            self.seed,          //
            self.hashed_seed,   //
            self.showMinimap    //
        );

        if constexpr (ps::concepts::ZppBitsInArchive<ArchiveT>) {
            const bool ok =
                snapshot_blob::decode_into_current_world(world_blob);
            VALIDATE(ok, "failed to decode world snapshot blob");
        }

        return result;
    }
};
