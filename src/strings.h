
#pragma once

/*

                                      ooo
                               ooo$$$$$$$$$$$oo
                            o$$$$$$$$$$$$$$$$$$$ooo
                          o$$$$$$$$$$$$$$$$$"$$$$$$$oo
                       o$$$$$$$$$$$$$$$$$$$  o $$$$$$$$$$$$$$oooo
                      o$$$$"""$$$$$$$$$$$$$oooo$$$$$$$$$$$$$"""
                    oo$""      "$$$$$$$$$$$$$$$$$$$$$$$$"
                   o$$          $$$$$$$$$$$$$$$$$$$$$$"
                  o$$            $$$$$$$$$$$$$$$$$$$$
                o$$$$             $$$$$$$$$$$$$$$$$"
               o$$$$$$oooooooo "                $"
              $$$$$$$$$$$$$$"
             $$$$$$$$$$$$$$
            o$$$$$$$$$$$$$                         o
           o$$$$$$$$$$$$$                           o
          o$$$$$$$$$$$$$$                            "o
         o$$$$$$$$$$$$$$$                             "o
        o$$$$$$$$$$$$$$$$                              $
       o$$$$$$$$$$$$$$$$"                              "
       $$$$$$$$$$$$$$$$$                                $
      o$$$$$$$$$$$$$$$$$                                $
      $$$$$$$$$$$$$$$$$                                 $
     $$$$$$$$$$$$$$$$$$                                 "
     $$$$$$$$$$$$$$$$$                                   $
    $$$$$$$$$$$$$$$$$                                    $
    $$$$$$$$$$$$$$$$"                                    $$
   $$$$$$$$$$$$$$$$$                                      $o
   $$$$$$$$$$$$$$$$$                                      $$
  $$$$$$$$$$$$$$$$$$                                       $
  $$$$$$$$$$$$$$$$$$o                                      $$
 $$$$$$$$$$$$$$$$$$$$o                                     $$
 $$$$$$$$$$$$$$$$$$$$$$o                                   "$
 $$$$$$$$$$$$$$$$$$$$$$$$o                                  $
$$$$$$$$$$$$$$$$$$$$$$$$$$$                                 $$
$$$$$$$$$$$$$$$$$$$$$$$$$$$$                                $$
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$                               $$
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$o                              $$
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$o                             $$
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$                             $$
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$                             $"
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$                            $$
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$                            $"
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$                            $
 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$                            o$
 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$                            $"
 $$$$$$$$$$$$$$$$$$$$$$$$$$$$"                            $
 $$$$$$$$$$$$$$$$$$$$$$$$$$$"                             $
  $$$$$$$$$$$$$$$$$$$$$$$$$"                             $$
  $$$$$$$$$$$$$$$$$$$$$$$$"                              $$
  $$$$$$$$$$$$$$$$$$$$$$$                                $$
   $$$$$$$$$$$$$$$$$$$$$                                o$$
   $$$$$$$$$$$$$$$$$$$$                                 $$"
   "$$$$$$$$$$$$$$$$$$                                  $$
    $$$$$$$$$$$$$$$$$                                  o$$
    "$$$$$$$$$$$$$$$"                                  $$
     $$$$$$$$$$$$$$$                                   $$
     "$$$$$$$$$$$$$"                                  o$
      $$$$$$$$$$$$"                                   $$
      $$$$$$$$$$$"                                    $$
       $$$$$$$$$"                                    $$"
       $$$$$$$$$                                     $$
       "$$$$$$$"                                    $$
        $$$$$$$o                                    $"
       o$$$$$$$$                                   $$
       $$$$$$$$$                                   $$
      o$$$$$$$$$                                   $"
      $$$$$$$$$$                                  $$
      "$$$$$$$$$                                o$""
       "$$$$$$$$                          ooooo$$oo
          ""$$$$$o                oooo$$$$$$$$$$$$$$ooo
             "$$$$$$ooooooo     """""""""$$$""""$$o   ""


 This file is for all strings

 */

#include <string>

namespace strings {

constexpr const char* GAME_NAME = "Pub Panic!";
constexpr const char* GAME_FOLDER = "pharmasea";

///

namespace textures {

constexpr const char* FACE = "face";

}  // namespace textures

namespace model {

constexpr const char* RUM = "rum";
constexpr const char* VODKA = "vodka";
constexpr const char* TEQUILA = "tequila";
constexpr const char* WHISKEY = "whiskey";
constexpr const char* GIN = "gin";
constexpr const char* BITTERS = "bitters";
constexpr const char* TRIPLESEC = "triplesec";
constexpr const char* COINTREAU = "cointreau";

constexpr const char* LEMON = "lemon";
constexpr const char* LEMON_HALF = "lemon_half";

constexpr const char* CHARACTER_BEAR = "character_bear";
constexpr const char* CHARACTER_DOG = "character_dog";
constexpr const char* CHARACTER_DUCK = "character_duck";
constexpr const char* CHARACTER_ROGUE = "character_rogue";
constexpr const char* CHARACTER_GABE = "character_gabe";
constexpr const char* CHARACTER_GABE2 = "character_gabe2";
constexpr const char* CHARACTER_RUTH = "character_ruth";

}  // namespace model

const std::array<std::string, 7> character_models = {
    strings::model::CHARACTER_BEAR, strings::model::CHARACTER_DOG,
    strings::model::CHARACTER_DUCK, strings::model::CHARACTER_ROGUE,
    strings::model::CHARACTER_GABE, strings::model::CHARACTER_GABE2,
    strings::model::CHARACTER_RUTH,
};

namespace sounds {

constexpr const char* ROBLOX = "roblox";
constexpr const char* VOMIT = "vom";
constexpr const char* SELECT = "select";
constexpr const char* CLICK = "click";
constexpr const char* WATER = "water";
constexpr const char* BLENDER = "blender";
constexpr const char* SOLID = "solid";
constexpr const char* ICE = "ice";
constexpr const char* PICKUP = "pickup";
constexpr const char* PLACE = "place";

}  // namespace sounds

namespace settings {

constexpr const char* TRANSLATIONS = "translations";
constexpr const char* IMAGES = "images";
constexpr const char* MODELS = "models";
constexpr const char* SOUNDS = "sounds";
constexpr const char* MUSIC = "music";
constexpr const char* SHADERS = "shaders";
constexpr const char* CONFIG = "config";
constexpr const char* FONTS = "fonts";

constexpr const char* KEYMAP_FILE = "keymap.json";

}  // namespace settings

namespace music {

constexpr const char* SUPERMARKET = "supermarket";
constexpr const char* THEME = "theme";

}  // namespace music

namespace globals {

constexpr const char* GAME_CAM = "game_cam";
constexpr const char* CAM_TARGET = "active_camera_target";
constexpr const char* MAP = "map";

}  // namespace globals

namespace menu {

constexpr const char* FPS = "FPS";
constexpr const char* GAME = "Game";
constexpr const char* MENU = "Menu";
constexpr const char* ABOUT = "About";

// TODO :IMPACT: translate
}  // namespace menu

// This is not aligned on purpose
// TODO :IMPACT: add to po file
constexpr const char* ABOUT_INFO = R"(
A game
Choice Honey 
Gabe 
    )";

namespace i18n {

// DO NOT CHANGE THESE WITHOUT CHANGING THE ONES IN THE .PO FILES
// DO NOT CHANGE THESE WITHOUT CHANGING THE ONES IN THE .PO FILES
// DO NOT CHANGE THESE WITHOUT CHANGING THE ONES IN THE .PO FILES
// DO NOT CHANGE THESE WITHOUT CHANGING THE ONES IN THE .PO FILES
// DO NOT CHANGE THESE WITHOUT CHANGING THE ONES IN THE .PO FILES
// DO NOT CHANGE THESE WITHOUT CHANGING THE ONES IN THE .PO FILES
// DO NOT CHANGE THESE WITHOUT CHANGING THE ONES IN THE .PO FILES
// DO NOT CHANGE THESE WITHOUT CHANGING THE ONES IN THE .PO FILES

// Menu Buttons
constexpr const char* ABOUT = "About";
constexpr const char* PLAY = "Play";
constexpr const char* SETTINGS = "Settings";
constexpr const char* EXIT = "Exit";

constexpr const char* BACK_BUTTON = "Back";

// Pause Menu
constexpr const char* CONTINUE = "Continue";
constexpr const char* QUIT = "Quit";

// Network Stuff
constexpr const char* JOIN = "Join";
constexpr const char* HOST = "Host";
constexpr const char* EDIT = "Edit";
constexpr const char* START = "Start";
constexpr const char* DISCONNECT = "Disconnect";
constexpr const char* USERNAME = "Username";
constexpr const char* LOCK_IN = "Lock-In";
constexpr const char* ENTER_IP = "Enter IP Address";
constexpr const char* LOAD_LAST_IP = "Load Last Used IP";
constexpr const char* CONNECT = "Connect";

constexpr const char* HIDE_IP = "Hide";
constexpr const char* SHOW_IP = "Show";
constexpr const char* COPY_IP = "Copy";

// Settings Page
constexpr const char* SAFE_ZONE = "safe zone";
constexpr const char* SHOW_SAFE_BOX = "Show Streamer Safe Box";
constexpr const char* ENABLE_PPS = "Enable Post-Processing Shaders";
constexpr const char* SNAP_CAMERA = "Snap Camera";
constexpr const char* MASTER_VOLUME = "Master Volume";
constexpr const char* MUSIC_VOLUME = "Music Volume";
constexpr const char* SOUND_VOLUME = "SFX Volume";
constexpr const char* RESOLUTION = "Resolution";
constexpr const char* THEME = "Theme";
constexpr const char* LANGUAGE = "Language";

constexpr const char* EXIT_AND_SAVE = "Save and Exit";
constexpr const char* EXIT_NO_SAVE = "Exit without Saving";

constexpr const char* GENERAL = "General";
constexpr const char* CONTROLS = "Controls";
constexpr const char* KEYBOARD = "Keyboard";
constexpr const char* GAMEPAD = "Gamepad";

// In Game
constexpr const char* START_GAME = "Start Game";
constexpr const char* CUSTOMERS_IN_STORE =
    "Can't close until all customers leave";
constexpr const char* HOLDING_FURNITURE =
    "Can't start game until all players drop furniture";
constexpr const char* NO_PATH_TO_REGISTER =
    "Can't start game until there is a path to a register";
constexpr const char* BAR_NOT_CLEAN =
    "Can't start game until your bar is all clean";
constexpr const char* FURNITURE_OVERLAPPING =
    "Can't start game if you have furniture overlapping";
constexpr const char* ITEMS_IN_SPAWN_AREA =
    "Can't start game if you have furniture still in the spawn area";
constexpr const char* DELETING_NEEDED_ITEM =
    "Can't trash machines you need for recipies...";
constexpr const char* LOADING = "Loading...";
constexpr const char* OPEN = "OPEN";
constexpr const char* CLOSING = "CLOSING";
constexpr const char* CLOSED = "CLOSED";
constexpr const char* NEXT_ROUND_COUNTDOWN = "Next Round Starting In";
constexpr const char* CHARACTER_SWITCHER = "Character Switcher";

constexpr const char* TRIGGERAREA_PURCHASE_FINISH = "Submit and Return";

constexpr const char* FLOORMARKER_TRASH = "Trash";
constexpr const char* FLOORMARKER_NEW_ITEMS = "New Items";
constexpr const char* FLOORMARKER_STORE_PURCHASE = "Place to Purchase";

constexpr const char* FAKESTRING_CAPS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
constexpr const char* FAKESTRING_NAPS = "abcdefghijklmnopqrstuvwxyz ";
constexpr const char* FAKESTRING_NUMS = "0123456789_-+=/[];,.<>?:";

}  // namespace i18n

namespace urls {

constexpr const char* DISCORD = "https://ochoag.com/discord.html";
constexpr const char* ITCH = "https://ochoag.com/pp-download.html";

}  // namespace urls

}  // namespace strings

// localization comes from engine/global.h
inline const char* text_lookup(const char* s) {
    if (!localization->mo_data) return s;

    int target_index = get_target_index(localization, s);
    if (target_index == -1) return s;  // Maybe we want to log an error?

    return get_translated_string(localization, target_index);
}
