

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

    Entities entities() const { return game_info.entities; }

    void onUpdate(float dt) {  //
        _onUpdate(remote_players_NOT_SERIALIZED, dt);
    }

    void onUpdateLocalPlayers(float dt);

    void _onUpdate(const std::vector<std::shared_ptr<Entity>>& players,
                   float dt);

    void onDraw(float dt) const;
    void onDrawUI(float dt);

    // These are called before every "send_map_state" when server
    // sends everything over to clients
    void grab_things() { game_info.grab_things(); }

   public:
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.object(game_info);
        s.value1b(showMinimap);
    }
};
