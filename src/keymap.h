
#pragma once

#include "external_include.h"
#include "menu.h"

struct KeyMap;
static std::shared_ptr<KeyMap> keymap;

typedef std::map<std::string, int> LayerMapping;
typedef std::map<Menu::State, LayerMapping> FullMap;

struct KeyMap {
    FullMap mapping;

    KeyMap() { load_default_keys(); }

    LayerMapping& get_or_create_layer_map(Menu::State state) {
        if (!this->mapping.contains(state)) {
            mapping[state] = LayerMapping();
        }
        return mapping[state];
    }

    void load_game_keys() {
        LayerMapping& game_map =
            this->get_or_create_layer_map(Menu::State::Game);
        game_map["Player Forward"] = KEY_W;
        game_map["Player Back"] = KEY_S;
        game_map["Player Left"] = KEY_A;
        game_map["Player Right"] = KEY_D;

        game_map["Player Pickup"] = KEY_SPACE;

        game_map["Target Forward"] = KEY_UP;
        game_map["Target Back"] = KEY_DOWN;
        game_map["Target Left"] = KEY_LEFT;
        game_map["Target Right"] = KEY_RIGHT;
    }

    void load_ui_keys() {
        LayerMapping& game_map = this->get_or_create_layer_map(Menu::State::UI);
        game_map["Widget Next"] = KEY_TAB;
        game_map["Widget Mod"] = KEY_LEFT_SHIFT;
        game_map["Widget Press"] = KEY_ENTER;
    }

    void load_default_keys() {
        load_game_keys();
        load_ui_keys();
    }

    inline static KeyMap* create() { return new KeyMap(); }
    inline static KeyMap& get() {
        if (!keymap) keymap.reset(KeyMap::create());
        return *keymap;
    }

    static int get_key_code(Menu::State state, const std::string& name) {
        return KeyMap::get().mapping[state][name];
    }

    static bool is_event(Menu::State state, const std::string& name) {
        int left = KeyMap::get_key_code(state, name);
        if (IsKeyDown(left)) return true;
        // TODO also check gamepad
        return false;
    }

    static bool is_event_once_DO_NOT_USE(Menu::State state,
                                         const std::string& name) {
        int left = KeyMap::get_key_code(state, name);
        if (IsKeyPressed(left)) return true;
        // TODO also check gamepad
        return false;
    }

    static bool does_layer_map_contain_key(Menu::State state,
                                         int keycode) {
        // We dont even have this Layer Map 
        if(!KeyMap::get().mapping.contains(state)) return false;
        LayerMapping layermap = KeyMap::get().mapping[state];
        for(auto pair : layermap){
            if(keycode == pair.second) return true;
        }
        return false;
    }
};
