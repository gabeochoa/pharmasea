

#pragma once
#include "engine/constexpr_containers.h"
#include "strings.h"

namespace strings {
namespace en_rev {
constexpr CEMap<i18n, const char*, magic_enum::enum_count<i18n>()>
    pre_translation = {{{
        //
        {i18n::ExampleFormattedString, "}emanresu{ si emanresu ruoY"},
        // Menu Buttons
        {i18n::Play, "yalP"},
        {i18n::Settings, "sgnitteS"},
        {i18n::About, "tuobA"},
        {i18n::Exit, "tixE"},
        //
        {i18n::RoundEndLayer_Countdown, "}tfel_emit{ nI gnitratS dnuoR txeN"},

        {i18n::StoreNotEnoughCoins, "snioc hguone toN"},
        {i18n::StoreMissingRequired, "enihcam deriuqer gnissiM"},
        {i18n::StoreStealingMachine, "kcab enihcam taht tup esaelP"},
        {i18n::StoreBalance, "ecnalaB"},
        {i18n::StoreTip, "pit"},

        // In Game
        {i18n::START_GAME, "emaG tratS"},
        {i18n::CUSTOMERS_IN_STORE, "evael sremotsuc lla litnu esolc t'naC"},
        {i18n::HOLDING_FURNITURE,
         "erutinruf pord sreyalp lla litnu emag trats t'naC"},
        {i18n::NO_PATH_TO_REGISTER,
         "retsiger a ot htap a si ereht litnu emag trats t'naC"},
        {i18n::BAR_NOT_CLEAN, "naelc lla si rab ruoy litnu emag trats t'naC"},
        {i18n::FURNITURE_OVERLAPPING,
         "gnippalrevo erutinruf evah uoy fi emag trats t'naC"},
        {i18n::ITEMS_IN_SPAWN_AREA,
         "aera nwaps eht ni llits erutinruf evah uoy fi emag trats t'naC"},
        {i18n::DELETING_NEEDED_ITEM,
         "...seipicer rof deen uoy senihcam hsart t'naC"},
        {i18n::LOADING, "...gnidaoL"},
        {i18n::OPEN, "NEPO"},
        {i18n::CLOSING, "GNISOLC"},
        {i18n::CLOSED, "DESOLC"},
        {i18n::NEXT_ROUND_COUNTDOWN, "nI gnitratS dnuoR txeN"},
        {i18n::CHARACTER_SWITCHER, "rehctiwS retcarahC"},

        {i18n::PLANNING_CUSTOMERS_COMING, "gnimoC sremotsuC"},
        {i18n::ROUND_DAY, "yaD"},

        {i18n::TRIGGERAREA_PURCHASE_FINISH, "nruteR dna timbuS"},

        {i18n::FLOORMARKER_TRASH, "hsarT"},
        {i18n::FLOORMARKER_NEW_ITEMS, "smetI weN"},
        {i18n::FLOORMARKER_STORE_PURCHASE, "esahcruP ot ecalP"},

        //
        {i18n::BACK_BUTTON, "kcaB"},

        // Pause Menu
        {i18n::CONTINUE, "eunitnoC"},
        {i18n::QUIT, "tiuQ"},

        // Network Stuff
        {i18n::JOIN, "nioJ"},
        {i18n::HOST, "tsoH"},
        {i18n::EDIT, "tidE"},
        {i18n::START, "tratS"},
        {i18n::DISCONNECT, "tcennocsiD"},
        {i18n::USERNAME, "emanresU"},
        {i18n::LOCK_IN, "nI-kcoL"},
        {i18n::ENTER_IP, "sserddA PI retnE"},
        {i18n::LOAD_LAST_IP, "PI desU tsaL daoL"},
        {i18n::CONNECT, "tcennoC"},

        {i18n::HIDE_IP, "ediH"},
        {i18n::SHOW_IP, "wohS"},
        {i18n::COPY_IP, "ypoC"},

        // Settings Page
        {i18n::SAFE_ZONE, "enoz efas"},
        {i18n::SHOW_SAFE_BOX, "xoB efaS remaertS wohS"},
        {i18n::ENABLE_PPS, "sredahS gnissecorP-tsoP elbanE"},
        {i18n::SNAP_CAMERA, "aremaC panS"},
        {i18n::MASTER_VOLUME, "emuloV retsaM"},
        {i18n::MUSIC_VOLUME, "emuloV cisuM"},
        {i18n::SOUND_VOLUME, "emuloV XFS"},
        {i18n::RESOLUTION, "noituloseR"},
        {i18n::THEME, "emehT"},
        {i18n::LANGUAGE, "egaugnaL"},
        {i18n::FULLSCREEN, "?neercslluF"},

        {i18n::EXIT_AND_SAVE, "tixe dna evaS"},
        {i18n::EXIT_NO_SAVE, "gnivaS tuohtiw tixE"},
        {i18n::RESET_ALL_SETTINGS, "sgnittes lla teseR"},

        {i18n::GENERAL, "lareneG"},
        {i18n::CONTROLS, "slortnoC"},
        {i18n::KEYBOARD, "draobyeK"},
        {i18n::GAMEPAD, "dapemaG"},

        {i18n::FAKESTRING_CAPS, " ZYXWVUTSRQPONMLKJIHGFEDCBA"},
        {i18n::FAKESTRING_NAPS, " zyxwvutsrqponmlkjihgfedcba"},
        {i18n::FAKESTRING_NUMS, ":?><.,;][/=+-_9876543210"},

        {i18n::Empty, ""},
        {i18n::InternalError, "rorrE lanretnI"},
    }}};

}  // namespace en_rev
}  // namespace strings
