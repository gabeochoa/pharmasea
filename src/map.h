

#pragma once

#include "external_include.h"
#include "level_info.h"

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
    LevelInfo game_info;

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

   public:
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        // Keep wire/save compatibility: this is the exact historical ordering:
        // - entities list (full snapshot)
        // - LevelInfo metadata (was_generated, seed, hashed_seed)
        // - showMinimap

        constexpr bool kIsReader = requires { s.adapter().error(); };

        Entities::size_type num_entities = 0;
        if constexpr (kIsReader) {
            Entities tmp;
            s.value8b(num_entities);
            s.container(tmp, num_entities,
                        [](S& s2, std::shared_ptr<Entity>& entity) {
                            s2.ext(entity, bitsery::ext::StdSmartPtr{});
                        });
            EntityHelper::get_current_collection().replace_all_entities(
                std::move(tmp));
        } else {
            Entities tmp = EntityHelper::get_entities();
            num_entities = tmp.size();
            s.value8b(num_entities);
            s.container(tmp, num_entities,
                        [](S& s2, std::shared_ptr<Entity>& entity) {
                            s2.ext(entity, bitsery::ext::StdSmartPtr{});
                        });
        }

        s.value1b(game_info.was_generated);
        s.text1b(game_info.seed, MAX_SEED_LENGTH);
        s.value8b(game_info.hashed_seed);

        s.value1b(showMinimap);
    }
};
