
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
constexpr const char* BAG_BOX = "bag box";
constexpr const char* MEDICINE_CABINET = "medicine cabinet";
constexpr const char* PILL_DISPENSER = "pill dispenser";
constexpr const char* TRIGGER_AREA = "trigger area";
constexpr const char* CUSTOMER_SPAWNER = "customer spawner";
constexpr const char* SOPHIE = "sophie";

constexpr const char* DEFAULT_TRIGGER = "DEFAULT TRIGGER";

}  // namespace entity

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

namespace i18n {

constexpr const char* ABOUT = "About";        // TODO translate
constexpr const char* JOIN = "Join";          // TODO translate
constexpr const char* HOST = "Host";          // TODO translate
constexpr const char* PLAY = "Play";          // TODO translate
constexpr const char* SETTINGS = "Settings";  // TODO translate
constexpr const char* EXIT = "Exit";          // TODO translate
constexpr const char* EDIT = "Edit";          // TODO translate
constexpr const char* BACK_BUTTON = "Back";   // TODO translate
// This is not aligned on purpose
constexpr const char* ABOUT_INFO = R"(
A game by: 
    Gabe
    Brett
    Alice)";  // TODO translate

}  // namespace i18n

}  // namespace strings

// localization comes from engine/global.h
inline const char* text_lookup(const char* s) {
    if (!localization->mo_data) return s;

    int target_index = get_target_index(localization, s);
    if (target_index == -1) return s;  // Maybe we want to log an error?

    return get_translated_string(localization, target_index);
}
