
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


 This file is for all strings that dont need to be translated;

 */

// TODO write a python script that extracts any new strings from here and adds
// them as empty in the .po files

#include <string>

namespace strings {
namespace entity {

constexpr const char* REMOTE_PLAYER = "remote player";
constexpr const char* PLAYER = "player";
constexpr const char* CUSTOMER = "customer";
constexpr const char* TABLE = "table";
constexpr const char* CHARACTER_SWITCHER = "character switcher";
constexpr const char* WALL = "wall";
constexpr const char* CONVEYER = "conveyer";
constexpr const char* GRABBER = "grabber";
constexpr const char* REGISTER = "register";
constexpr const char* MEDICINE_CABINET = "medicine cabinet";
constexpr const char* PILL_DISPENSER = "pill dispenser";
constexpr const char* TRIGGER_AREA = "trigger area";
constexpr const char* CUSTOMER_SPAWNER = "customer spawner";
constexpr const char* SOPHIE = "sophie";
constexpr const char* BLENDER = "blender";
constexpr const char* SODA_MACHINE = "soda machine";
constexpr const char* CUPBOARD = "cupboard";
constexpr const char* SQUIRTER = "squirter";
constexpr const char* TRASH = "trash";

constexpr const char* DEFAULT_TRIGGER = "DEFAULT TRIGGER";

}  // namespace entity

namespace item {

constexpr const char* SODA_SPOUT = "SODA_SPOUT";
constexpr const char* DRINK = "DRINK";
constexpr const char* ALCOHOL = "ALCOHOL";
constexpr const char* LEMON = "LEMON";
constexpr const char* SIMPLE_SYRUP = "SIMPLE_SYRUP";

}  // namespace item

namespace textures {

constexpr const char* FACE = "face";

}  // namespace textures

namespace model {

constexpr const char* CHARACTER_BEAR = "character_bear";
constexpr const char* CHARACTER_DOG = "character_dog";
constexpr const char* CHARACTER_DUCK = "character_duck";
constexpr const char* CHARACTER_ROGUE = "character_rogue";

}  // namespace model

namespace sounds {

constexpr const char* ROBLOX = "roblox";

}

namespace settings {

constexpr const char* TRANSLATIONS = "translations";
constexpr const char* IMAGES = "images";
constexpr const char* MODELS = "models";
constexpr const char* SOUNDS = "sounds";
constexpr const char* MUSIC = "music";
constexpr const char* SHADERS = "shaders";
constexpr const char* CONFIG = "config";

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

constexpr const char* PHARMASEA = "Pharmasea";

// TODO translate
}  // namespace menu

// This is not aligned on purpose
// TODO add to po file
constexpr const char* ABOUT_INFO = R"(
A game by: 
    Gabe
    Brett
    Alice)";

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
constexpr const char* MASTER_VOLUME = "Master Volume";
constexpr const char* MUSIC_VOLUME = "Music Volume";
constexpr const char* RESOLUTION = "Resolution";
constexpr const char* LANGUAGE = "Language";

// In Game
constexpr const char* START_GAME = "Start Game";
constexpr const char* CUSTOMERS_IN_STORE =
    "Can't close until all customers leave";
constexpr const char* HOLDING_FURNITURE =
    "Can't start game until all players drop furniture";
constexpr const char* NO_PATH_TO_REGISTER =
    "Can't start game until there is a path to a register";
constexpr const char* LOADING = "Loading...";
constexpr const char* OPEN = "OPEN";
constexpr const char* CLOSING = "CLOSING";
constexpr const char* CLOSED = "CLOSED";
constexpr const char* NEXT_ROUND_COUNTDOWN = "Next Round Starting In";

}  // namespace i18n

}  // namespace strings

// localization comes from engine/global.h
inline const char* text_lookup(const char* s) {
    if (!localization->mo_data) return s;

    int target_index = get_target_index(localization, s);
    if (target_index == -1) return s;  // Maybe we want to log an error?

    return get_translated_string(localization, target_index);
}
