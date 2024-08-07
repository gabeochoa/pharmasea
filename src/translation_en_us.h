

#pragma once

#include "engine/constexpr_containers.h"
#include "strings.h"

namespace strings {
namespace en_us {
constexpr CEMap<i18n, const char*, magic_enum::enum_count<i18n>()>
    pre_translation = {{{
        //
        {i18n::ExampleFormattedString, "Your username is {username}"},
        // Menu Buttons
        {i18n::Play, "Play"},
        {i18n::Settings, "Settings"},
        {i18n::About, "About"},
        {i18n::Exit, "Exit"},
        //
        {i18n::RoundEndLayer_Countdown, "Next Round Starting In {time_left}"},

        {i18n::StoreNotEnoughCoins, "Not enough coins"},
        {i18n::StoreMissingRequired, "Missing required machine"},
        {i18n::StoreStealingMachine, "Please put that machine back"},
        {i18n::StoreTip, "{transaction_extra} tip"},
        {i18n::StoreBalance, "Balance: {balance_amount}"},
        {i18n::StoreInCart, "In Cart: {cart_amount}"},
        {i18n::StoreReroll, "Reroll shop for {reroll_cost} coins"},

        // In Game
        {i18n::START_GAME, "Start Game"},
        {i18n::CUSTOMERS_IN_STORE, "Can't close until all customers leave"},
        {i18n::HOLDING_FURNITURE,
         "Can't start game until all players drop furniture"},
        {i18n::NO_PATH_TO_REGISTER,
         "Can't start game until there is a path to a register"},
        {i18n::BAR_NOT_CLEAN, "Can't start game until your bar is all clean"},
        {i18n::FURNITURE_OVERLAPPING,
         "Can't start game if you have furniture overlapping"},
        {i18n::ITEMS_IN_SPAWN_AREA,
         "Can't start game if you have furniture still in the spawn area"},
        {i18n::DELETING_NEEDED_ITEM,
         "Can't trash machines you need for recipies..."},
        {i18n::LOADING, "Loading..."},
        {i18n::NEXT_ROUND_COUNTDOWN, "Next Round Starting In"},
        {i18n::CHARACTER_SWITCHER, "Character Switcher"},

        {i18n::PLANNING_CUSTOMERS_COMING, "Customers Coming: {customer_count}"},

        //
        {i18n::OPEN, "OPEN"},
        {i18n::CLOSING, "CLOSING"},
        {i18n::CLOSED, "CLOSED"},
        {i18n::RoundDayWithStatusText, "{opening_status} Day {day_count}"},

        {i18n::TRIGGERAREA_PURCHASE_FINISH, "Submit and Return"},

        {i18n::FLOORMARKER_TRASH, "Trash"},
        {i18n::FLOORMARKER_NEW_ITEMS, "New Items"},
        {i18n::FLOORMARKER_STORE_PURCHASE, "Place to Purchase"},
        {i18n::FLOORMARKER_STORE_LOCK, "Place to Save until Tomorrow"},

        //
        {i18n::BACK_BUTTON, "Back"},

        // Pause Menu
        {i18n::CONTINUE, "Continue"},
        {i18n::QUIT, "Quit"},

        // Network Stuff
        {i18n::JOIN, "Join"},
        {i18n::HOST, "Host"},
        {i18n::EDIT, "Edit"},
        {i18n::START, "Start"},
        {i18n::DISCONNECT, "Disconnect"},
        {i18n::USERNAME, "Username"},
        {i18n::LOCK_IN, "Lock-In"},
        {i18n::ENTER_IP, "Enter IP Address"},
        {i18n::LOAD_LAST_IP, "Load Last Used IP"},
        {i18n::CONNECT, "Connect"},

        {i18n::HIDE_IP, "Hide"},
        {i18n::SHOW_IP, "Show"},
        {i18n::COPY_IP, "Copy"},

        // Settings Page
        {i18n::SAFE_ZONE, "safe zone"},
        {i18n::SHOW_SAFE_BOX, "Show Streamer Safe Box"},
        {i18n::ENABLE_PPS, "Enable Post-Processing Shaders"},
        {i18n::SNAP_CAMERA, "Snap Camera"},
        {i18n::MASTER_VOLUME, "Master Volume"},
        {i18n::MUSIC_VOLUME, "Music Volume"},
        {i18n::SOUND_VOLUME, "SFX Volume"},
        {i18n::RESOLUTION, "Resolution"},
        {i18n::THEME, "Theme"},
        {i18n::LANGUAGE, "Language"},
        {i18n::FULLSCREEN, "Fullscreen?"},

        {i18n::EXIT_AND_SAVE, "Save and exit"},
        {i18n::EXIT_NO_SAVE, "Exit without Saving"},
        {i18n::RESET_ALL_SETTINGS, "Reset all settings"},

        {i18n::GENERAL, "General"},
        {i18n::CONTROLS, "Controls"},
        {i18n::KEYBOARD, "Keyboard"},
        {i18n::GAMEPAD, "Gamepad"},

        {i18n::FAKESTRING_CAPS, "ABCDEFGHIJKLMNOPQRSTUVWXYZ "},
        {i18n::FAKESTRING_NAPS, "abcdefghijklmnopqrstuvwxyz "},
        {i18n::FAKESTRING_NUMS, "0123456789_-+=/[];,.<>?:"},

        {i18n::Empty, ""},
        {i18n::InternalError, "Internal Error"},
    }}};

}  // namespace en_us
}  // namespace strings
