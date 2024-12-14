

#pragma once

#include "../engine/constexpr_containers.h"
#include "../strings.h"

namespace strings {
namespace en_rev {
constexpr CEMap<i18n, const char*, magic_enum::enum_count<i18n>()>
    pre_translation = {{{
        //
        {i18n::ExampleFormattedString, "ruoY emanresu si{username}"},
        // Menu Buttons
        {i18n::Play, "yalP"},
        {i18n::Settings, "sgnitteS"},
        {i18n::About, "tuobA"},
        {i18n::Exit, "tixE"},
        //
        {i18n::RoundEndLayer_Countdown, "txeN dnuoR gnitratS nI{time_left}"},

        {i18n::StoreNotEnoughCoins, "toN hguone snioc"},
        {i18n::StoreMissingRequired, "gnissiM deriuqer enihcam"},
        {i18n::StoreCartEmpty, "gnihtoN ni ruoy trac tey ):"},
        {i18n::StoreStealingMachine,
         "oN !gnilaetS esaelP tup taht enihcam kcab"},
        {i18n::StoreHasGarbage, "esaelP tnod evael ruoy hsart ni ereh"},
        {i18n::StoreTip, "{transaction_extra}pit"},
        {i18n::StoreBalance, ":ecnalaB{balance_amount}"},
        {i18n::StoreRentDue, "tneR :euD{rent_due}"},
        {i18n::StoreRentDaysRemaining, "syaD :gniniameR{days_until_rent}"},
        {i18n::StoreInCart, "nI :traC{cart_amount}"},
        {i18n::StoreReroll, "lloreR pohs rof{reroll_cost}snioc"},

        // In Game
        {i18n::START_GAME, "tratS emaG"},
        {i18n::CUSTOMERS_IN_STORE, "t'naC esolc litnu lla sremotsuc evael"},
        {i18n::HOLDING_FURNITURE,
         "t'naC trats emag litnu lla sreyalp pord erutinruf"},
        {i18n::NO_PATH_TO_REGISTER,
         "t'naC trats emag litnu ereht si a htap ot a retsiger"},
        {i18n::REGISTER_NOT_INSIDE,
         "t'naC trats emag litnu uoy evah a retsiger edisni eht rab aera"},
        {i18n::BAR_NOT_CLEAN, "t'naC trats emag litnu ruoy rab si lla naelc"},
        {i18n::FURNITURE_OVERLAPPING,
         "t'naC trats emag fi uoy evah erutinruf gnippalrevo"},
        {i18n::ITEMS_IN_SPAWN_AREA,
         "t'naC trats emag fi uoy evah erutinruf llits ni eht nwaps aera"},
        {i18n::DELETING_NEEDED_ITEM,
         "t'naC hsart senihcam uoy deen rof ...seipicer"},
        {i18n::LOADING, "...gnidaoL"},
        {i18n::NEXT_ROUND_COUNTDOWN, "txeN dnuoR gnitratS nI"},
        {i18n::CHARACTER_SWITCHER, "retcarahC rehctiwS"},

        {i18n::PLANNING_CUSTOMERS_COMING, "sremotsuC :gnimoC{customer_count}"},

        //
        {i18n::OPEN, "NEPO"},
        {i18n::CLOSING, "GNISOLC"},
        {i18n::CLOSED, "DESOLC"},
        {i18n::RoundDayWithStatusText, "{opening_status}yaD }tnuoc_yad{"},

        {i18n::TRIGGERAREA_PURCHASE_FINISH, "timbuS dna nruteR"},

        {i18n::FLOORMARKER_TRASH, "hsarT"},
        {i18n::FLOORMARKER_NEW_ITEMS, "weN smetI"},
        {i18n::FLOORMARKER_STORE_PURCHASE, "ecalP ot esahcruP"},
        {i18n::FLOORMARKER_STORE_LOCK, "ecalP ot evaS litnu worromoT"},

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
        {i18n::ENTER_IP, "retnE PI sserddA"},
        {i18n::LOAD_LAST_IP, "daoL tsaL desU PI"},
        {i18n::CONNECT, "tcennoC"},

        {i18n::HIDE_IP, "ediH"},
        {i18n::SHOW_IP, "wohS"},
        {i18n::COPY_IP, "ypoC"},

        // Settings Page
        {i18n::SAFE_ZONE, "efas enoz"},
        {i18n::SHOW_SAFE_BOX, "wohS remaertS efaS xoB"},
        {i18n::ENABLE_PPS, "elbanE gnissecorP-tsoP sredahS"},
        {i18n::SNAP_CAMERA, "panS aremaC"},
        {i18n::MASTER_VOLUME, "retsaM emuloV"},
        {i18n::MUSIC_VOLUME, "cisuM emuloV"},
        {i18n::SOUND_VOLUME, "XFS emuloV"},
        {i18n::RESOLUTION, "noituloseR"},
        {i18n::THEME, "emehT"},
        {i18n::LANGUAGE, "egaugnaL"},
        {i18n::FULLSCREEN, "?neercslluF"},

        {i18n::EXIT_AND_SAVE, "evaS dna tixe"},
        {i18n::EXIT_NO_SAVE, "tixE tuohtiw gnivaS"},
        {i18n::RESET_ALL_SETTINGS, "teseR lla sgnittes"},

        {i18n::GENERAL, "lareneG"},
        {i18n::CONTROLS, "slortnoC"},
        {i18n::KEYBOARD, "draobyeK"},
        {i18n::GAMEPAD, "dapemaG"},

        {i18n::FAKESTRING_CAPS, "ZYXWVUTSRQPONMLKJIHGFEDCBA"},
        {i18n::FAKESTRING_NAPS, "zyxwvutsrqponmlkjihgfedcba"},
        {i18n::FAKESTRING_NUMS, ":?><.,;][/=+-_9876543210"},

        {i18n::Empty, ""},
        {i18n::InternalError, "lanretnI rorrE"},
    }}};

}  // namespace en_rev
}  // namespace strings
