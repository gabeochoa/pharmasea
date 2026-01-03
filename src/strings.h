
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

#include <array>
#include <cassert>
#include <iostream>
#include <string>

#include "zpp_bits_include.h"
#include "engine/log.h"
#include "vendor_include.h"

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
constexpr const char* CHARACTER_EFB = "character_efb";

}  // namespace model

constexpr std::array<std::string_view, 8> character_models = {
    strings::model::CHARACTER_BEAR, strings::model::CHARACTER_DOG,
    strings::model::CHARACTER_DUCK, strings::model::CHARACTER_ROGUE,
    strings::model::CHARACTER_GABE, strings::model::CHARACTER_GABE2,
    strings::model::CHARACTER_RUTH, strings::model::CHARACTER_EFB,
};

namespace sounds {

enum struct SoundId : uint8_t {
    None = 0,
    ROBLOX,
    VOMIT,
    SELECT,
    CLICK,
    WATER,
    BLENDER,
    SOLID,
    ICE,
    PICKUP,
    PLACE,
};

inline const char* to_name(SoundId id) {
    switch (id) {
        case SoundId::None:
            return "";
        case SoundId::ROBLOX:
            return "roblox";
        case SoundId::VOMIT:
            return "vom";
        case SoundId::SELECT:
            return "select";
        case SoundId::CLICK:
            return "click";
        case SoundId::WATER:
            return "water";
        case SoundId::BLENDER:
            return "blender";
        case SoundId::SOLID:
            return "solid";
        case SoundId::ICE:
            return "ice";
        case SoundId::PICKUP:
            return "pickup";
        case SoundId::PLACE:
            return "place";
    }
    return "";
}

}  // namespace sounds

namespace settings {

constexpr const char* TRANSLATIONS = "translations";
constexpr const char* IMAGES = "images";
constexpr const char* MODELS = "models";
constexpr const char* SOUNDS = "sounds";
constexpr const char* UI = "ui";
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

namespace urls {

constexpr const char* DISCORD = "https://ochoag.com/discord.html";
constexpr const char* ITCH = "https://ochoag.com/pp-download.html";

}  // namespace urls

enum struct i18nParam {
    ExampleFormattedParam,
    TimeRemaining,
    TransactionExtra,
    DayCount,
    OpeningStatus,
    CustomerCount,
    CartAmount,
    BalanceAmount,
    RerollCost,
    RentDue,
    DaysUntilRent,
    EstimatedProfit,
};
const std::map<i18nParam, std::string> translation_param = {{
    {i18nParam::ExampleFormattedParam, "username"},
    {i18nParam::TimeRemaining, "time_left"},
    {i18nParam::TransactionExtra, "transaction_extra"},
    {i18nParam::DayCount, "day_count"},
    {i18nParam::OpeningStatus, "opening_status"},
    {i18nParam::CustomerCount, "customer_count"},
    {i18nParam::BalanceAmount, "balance_amount"},
    {i18nParam::CartAmount, "cart_amount"},
    {i18nParam::RerollCost, "reroll_cost"},
    {i18nParam::RentDue, "rent_due"},
    {i18nParam::DaysUntilRent, "days_until_rent"},
    {i18nParam::EstimatedProfit, "estimated_profit"},
}};

enum struct i18n {
    ExampleFormattedString,
    //
    Play,
    Settings,
    About,
    Exit,
    //
    RoundEndLayer_Countdown,
    // Store
    StoreNotEnoughCoins,
    StoreMissingRequired,
    StoreCartEmpty,
    StoreStealingMachine,
    StoreHasGarbage,
    StoreBalance,
    StoreTip,
    StoreInCart,
    StoreReroll,
    StoreRentDue,
    StoreRentDaysRemaining,
    StoreEstimatedProfit,

    //
    START_GAME,
    CUSTOMERS_IN_STORE,
    HOLDING_FURNITURE,
    NO_PATH_TO_REGISTER,
    REGISTER_NOT_INSIDE,
    BAR_NOT_CLEAN,
    FURNITURE_OVERLAPPING,
    ITEMS_IN_SPAWN_AREA,
    DELETING_NEEDED_ITEM,
    LOADING,
    OPEN,
    CLOSING,
    CLOSED,
    NEXT_ROUND_COUNTDOWN,
    CHARACTER_SWITCHER,
    PLANNING_CUSTOMERS_COMING,
    TRIGGERAREA_PURCHASE_FINISH,
    FLOORMARKER_TRASH,
    FLOORMARKER_NEW_ITEMS,
    FLOORMARKER_STORE_PURCHASE,
    FLOORMARKER_STORE_LOCK,

    RoundDayWithStatusText,

    BACK_BUTTON,

    // Pause Menu
    CONTINUE,
    QUIT,

    // Network Stuff
    JOIN,
    HOST,
    EDIT,
    START,
    DISCONNECT,
    USERNAME,
    LOCK_IN,
    ENTER_IP,
    LOAD_LAST_IP,
    CONNECT,

    HIDE_IP,
    SHOW_IP,
    COPY_IP,

    // Settings Page
    SAFE_ZONE,
    SHOW_SAFE_BOX,
    ENABLE_PPS,
    ENABLE_LIGHTING,
    SNAP_CAMERA,
    VSYNC_ENABLED,
    MASTER_VOLUME,
    MUSIC_VOLUME,
    SOUND_VOLUME,
    RESOLUTION,
    THEME,
    LANGUAGE,
    FULLSCREEN,

    EXIT_AND_SAVE,
    EXIT_NO_SAVE,
    RESET_ALL_SETTINGS,

    GENERAL,
    CONTROLS,
    KEYBOARD,
    GAMEPAD,

    FAKESTRING_CAPS,
    FAKESTRING_NAPS,
    FAKESTRING_NUMS,

    Empty,
    InternalError,
};

extern std::map<i18n, std::string> pre_translation;
}  // namespace strings

template<typename T>
fmt::detail::named_arg<char, T> create_param(const strings::i18nParam& param,
                                             const T& arg) {
    if (!strings::translation_param.contains(param)) {
        log_error("Missing param {}",
                  magic_enum::enum_name<strings::i18nParam>(param));
    }
    const char* param_name = strings::translation_param.at(param).c_str();
    return fmt::arg(param_name, arg);
}

/*

template<typename... Args>
[[nodiscard]] inline std::string translate_formatted(const i18n_new& key,
                                                     Args&&... args) {
    if (!pre_translation.contains(key)) {
        log_error("Missing translation for {}",
                  magic_enum::enum_name<i18n_new>(key));
    }
    const auto& fmt_string = pre_translation.at(key);
    fmt::format_args fmt_args =
        fmt::make_args_checked<Args...>(fmt_string, args...);
    return fmt::vformat(fmt_string, fmt_args);
}


#include "engine/log.h"
//
#include "strings2.h"

int main(int, char**) {
    auto s = translate_formatted(
        i18n_new::ExampleFormattedString,
        create_param(i18nParam::ExampleFormattedParam, "coolname"));
    log_info("{}", s);
    return 0;
}

*/

// TODO make those constexpr strings above translatablestring :)
//
enum struct TodoReason {
    Format,
    UserFacingError,
    KeyName,
    Recursion,
    ServerString,
    SubjectToChange,
    UILibrary,
};

struct TranslatableString {
    static constexpr int MAX_LENGTH = 100;

    explicit TranslatableString() {}
    explicit TranslatableString(const std::string& s) : content(s) {}
    explicit TranslatableString(const strings::i18n& key) {
        if (!strings::pre_translation.contains(key)) {
            log_error("Missing translation for {}",
                      magic_enum::enum_name<strings::i18n>(key));
        }
        content = strings::pre_translation.at(key);
    }
    explicit TranslatableString(const std::string& s, bool ig)
        : content(s), no_translate(ig) {}

    [[nodiscard]] bool skip_translate() const { return no_translate; }
    [[nodiscard]] bool empty() const { return content.empty(); }
    [[nodiscard]] const char* debug() const { return content.c_str(); }
    [[nodiscard]] const char* underlying_TL_ONLY() const {
        return content.c_str();
    }

    [[nodiscard]] const std::string& str(TodoReason) const { return content; }

    [[nodiscard]] size_t size() const { return content.size(); }
    void resize(size_t len) { content.resize(len); }

    auto& set_param(const strings::i18nParam& param, const std::string& arg) {
        if (!formatted) formatted = true;
        params[param] = arg;
        return *this;
    }

    template<typename T>
    auto& set_param(const strings::i18nParam& param, const T& arg) {
        return set_param(param, fmt::format("{}", arg));
    }

    template<>
    auto& set_param(const strings::i18nParam& param,
                    const TranslatableString& arg) {
        return set_param(param, fmt::format("{}", arg.underlying_TL_ONLY()));
    }

    [[nodiscard]] bool is_formatted() const { return formatted; }

    fmt::dynamic_format_arg_store<fmt::format_context> get_params() const {
        fmt::dynamic_format_arg_store<fmt::format_context> store;
        for (const auto& kv : params) {
            store.push_back(create_param(kv.first, kv.second));
        }
        return store;
    }

   private:
    std::string content;
    std::map<strings::i18nParam, std::string> params;

    bool formatted = false;
    bool no_translate = false;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(    //
            self.content,  //
            self.formatted, //
            self.no_translate, //
            self.params    //
        );
    }
};

[[nodiscard]] inline TranslatableString NO_TRANSLATE(const std::string& s) {
    return TranslatableString{s, true};
}

// TODO fix all of these before launch :)
[[nodiscard]] inline TranslatableString TODO_TRANSLATE(const std::string& s,
                                                       TodoReason) {
    return TranslatableString{s, true};
}

[[nodiscard]] inline std::string translate_formatted(
    const TranslatableString& trs) {
    return fmt::vformat(trs.underlying_TL_ONLY(), trs.get_params());
}

// localization comes from engine/global.h
[[nodiscard]] inline std::string translation_lookup(
    const TranslatableString& s) {
    if (s.skip_translate()) return s.underlying_TL_ONLY();
    if (s.is_formatted()) {
        return translate_formatted(s);
    }
    return s.underlying_TL_ONLY();
}
