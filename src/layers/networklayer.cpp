
#include "networklayer.h"

#include "../components/has_name.h"
#include "../engine/toastmanager.h"
#include "../local_ui.h"
#include "../network/network.h"

std::unique_ptr<network::Info> network_info;

using namespace ui;

bool validate_ip(const std::string& ip) {
    // TODO this should fail for 00000.00000.0000 but shouldnt
    //      this too 000000000000
    //
    // NOTE: this is const static cause its faster :)
    const static std::regex ip_regex(
        R"(^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    return std::regex_match(ip, ip_regex);
}

NetworkLayer::NetworkLayer()
    : Layer("Network"),
      ui_context(std::make_shared<ui::UIContext>()),
      lobby_screen("lobby_screen"),
      join_lobby_screen("join_lobby_screen"),
      username_screen("username_screen"),
      network_selection_screen("network_selection_screen") {}

void NetworkLayer::NetworkLayer::onStartup() {
    // Note: init_connections() is already called from game.cpp during startup
    // so we don't need to call it again here

    if (network_info) {
        if (!Settings::get().data.username.empty()) {
            network_info->lock_in_username();
        }
    }
}

NetworkLayer::~NetworkLayer() { network::Info::shutdown_connections(); }

bool NetworkLayer::onCharPressedEvent(CharPressedEvent& event) {
    if (MenuState::get().is_not(menu::State::Network)) return false;
    return ui_context->process_char_press_event(event);
}

bool NetworkLayer::onGamepadAxisMoved(GamepadAxisMovedEvent& event) {
    if (MenuState::get().is_not(menu::State::Network)) return false;
    return ui_context->process_gamepad_axis_event(event);
}

bool NetworkLayer::onKeyPressed(KeyPressedEvent& event) {
    if (MenuState::get().is_not(menu::State::Network)) return false;
    return ui_context->process_keyevent(event);
}

bool NetworkLayer::onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
    if (MenuState::get().is_not(menu::State::Network)) return false;

    if (KeyMap::get_button(menu::State::UI, InputName::MenuBack) ==
        event.button) {
        MenuState::get().go_back();
        return true;
    }

    return ui_context->process_gamepad_button_event(event);
}

void NetworkLayer::onUpdate(float dt) {
    // NOTE: this has to go above the checks since it always has to run
    network_info->tick(dt);

    if (MenuState::get().is_not(menu::State::Network)) return;
    // if we get here, then user clicked "join"
}

void NetworkLayer::handle_announcements() {
    if (!network_info->client) return;

    for (const auto& info : network_info->client->announcements) {
        log_info("Announcement: {}", info.message);
        // TODO add support for other annoucement types
        TOASTS.push_back({.msg = info.message, .timeToShow = 5});
    }
    network_info->client->announcements.clear();
}

void NetworkLayer::draw_username_picker(float) {
    username_screen.draw_background();

    username_screen.text("UsernameText",
                         TranslatableString(strings::i18n::USERNAME));

    auto name = username_screen.rect("PlayerUsernameInputText");
    if (auto result = textfield(
            Widget{name},
            TextfieldData{
                Settings::get().data.username,
                [](const std::string& content) {
                    if (content.size() >= network::MAX_NAME_LENGTH)
                        return TextfieldValidationDecisionFlag::StopNewInput;
                    return TextfieldValidationDecisionFlag::Valid;
                }});
        result) {
        Settings::get().data.username = result.as<std::string>();
    }

    if (username_screen.button("SaveButton",
                               TranslatableString(strings::i18n::LOCK_IN))) {
        network_info->lock_in_username();
        Settings::get().write_save_file();
    }

    if (username_screen.button(
            "BackButton", TranslatableString(strings::i18n::BACK_BUTTON))) {
        MenuState::get().go_back();
    }
}

void NetworkLayer::draw_username_with_edit(Rectangle username, float) {
    username = rect::tpad(username, 20);
    username = rect::hpad(username, 20);

    auto [label, uname, edit] = rect::hsplit<3>(username);

    text(Widget{rect::hpad(label, 5)},
         TranslatableString(strings::i18n::USERNAME));

    text(Widget{uname}, NO_TRANSLATE(Settings::get().data.username));

    bool show_edit_button =
        network_info->client ? network_info->client->remote_players.size() <= 1
                             : true;

    if (show_edit_button) {
        edit = rect::hpad(edit, 20);
        edit = rect::vpad(edit, 5);
        if (ps::button(Widget{edit}, TranslatableString(strings::i18n::EDIT))) {
            network_info->unlock_username();
        }
    }
}

void NetworkLayer::draw_role_selector_screen(float) {
    network_selection_screen.draw_background();

    if (network_selection_screen.button(
            "HostButton", TranslatableString(strings::i18n::HOST))) {
        log_info("Host button clicked - attempting to set role to Host");
        network_info->set_role(network::Info::Role::s_Host);
        log_info("Host button clicked - set_role call completed");
    }
    if (network_selection_screen.button(
            "JoinButton", TranslatableString(strings::i18n::JOIN))) {
        network_info->set_role(network::Info::Role::s_Client);
    }

    if (network_selection_screen.button(
            "BackButton", TranslatableString(strings::i18n::BACK_BUTTON))) {
        MenuState::get().clear_history();
        MenuState::get().set(menu::State::Root);
    }

    // draw lobby username
    {
        network_selection_screen.text(
            "UsernameText", TranslatableString(strings::i18n::USERNAME));

        network_selection_screen.text(
            "PlayerUsernameText", NO_TRANSLATE(Settings::get().data.username));

        bool show_edit_button =
            network_info->client
                ? network_info->client->remote_players.size() <= 1
                : true;

        if (show_edit_button) {
            if (network_selection_screen.button(
                    "EditButton", TranslatableString(strings::i18n::EDIT))) {
                network_info->unlock_username();
            }
        }
    }
}

void NetworkLayer::draw_connected_screen(float) {
    lobby_screen.draw_background();

    // draw lobby
    {
        lobby_screen.text("LobbyText",
                          TODO_TRANSLATE("Lobby", TodoReason::SubjectToChange));

        Rectangle lobby_area = lobby_screen.rect("LobbyPlayersText");

        int num_players =
            std::max(1, (int) network_info->client->remote_players.size());

        float min_name_height = lobby_screen.scale_to_resolution(32.f);
        float max_name_height = lobby_screen.scale_to_resolution(64.f);
        float height_per = std::min(
            max_name_height,
            std::max(min_name_height, lobby_area.height / num_players));

        Rectangle player_name{lobby_area.x, lobby_area.y, lobby_area.width,
                              height_per};

        auto remote_players = network_info->client->remote_players;

        // when you are joining a lobby and there is no host yet,
        // you will see no players,
        //
        // force the current viewers name to show up in the list
        if (!network_info->is_host() && remote_players.empty()) {
            text(player_name, NO_TRANSLATE(Settings::get().data.username));
            player_name.y += height_per;
        }

        for (const auto& kv : network_info->client->remote_players) {
            text(player_name,
                 NO_TRANSLATE(fmt::format(
                     "{}({})", kv.second->get<HasName>().name(), kv.first)));

            player_name.y += height_per;
        }
    }

    // Buttons
    {
        // TODO because only the host can control this
        // need a way to their hide it from the screen
        // or to put some text that says "Host only" or somethign
        if (network_info->is_host()) {
            if (lobby_screen.button("StartButton",
                                    TranslatableString(strings::i18n::START))) {
                MenuState::get().set(menu::State::Game);
                GameState::get().set(game::State::Lobby);
                network_info->send_current_menu_state();
            }
        }

        if (lobby_screen.button(
                "DisconnectButton",
                TranslatableString(strings::i18n::DISCONNECT))) {
            network::Info::reset_connections();
        }
    }

    // draw ip address
    {
        lobby_screen.text("IPAddrText",
                          TODO_TRANSLATE("IP Address", TodoReason::Format));

        auto ip = should_show_host_ip ? network::my_remote_ip_address
                                      : "***.***.***.***";
        lobby_screen.text("IPAddrValueText", NO_TRANSLATE(ip));

        strings::i18n show_hide_host_ip_text = should_show_host_ip
                                                   ? strings::i18n::HIDE_IP
                                                   : strings::i18n::SHOW_IP;
        if (lobby_screen.checkbox(
                "ShowHideButton",
                ui::CheckboxData{
                    .selected = should_show_host_ip,
                    .content = TranslatableString(show_hide_host_ip_text)
                                   .str(TodoReason::UILibrary)})) {
            should_show_host_ip = !should_show_host_ip;
        }

        if (lobby_screen.button("CopyButton",
                                TranslatableString(strings::i18n::COPY_IP))) {
            ext::set_clipboard_text(my_ip_address.c_str());
        }
    }

    // draw lobby username
    {
        lobby_screen.text("UsernameText",
                          TranslatableString(strings::i18n::USERNAME));

        lobby_screen.text("PlayerUsernameText",
                          NO_TRANSLATE(Settings::get().data.username));

        // TODO add a way to edit your username without disconnecting
        bool show_edit_button =
            network_info->client
                ? network_info->client->remote_players.size() <= 1
                : true;

        if (show_edit_button) {
            if (lobby_screen.button("EditButton",
                                    TranslatableString(strings::i18n::EDIT))) {
                network_info->unlock_username();
            }
        }
    }
}

void NetworkLayer::draw_ip_input_screen(float) {
    join_lobby_screen.draw_background();

    // TODO add showhide button
    // IP addr input
    {
        join_lobby_screen.text("IPAddrText",
                               TranslatableString(strings::i18n::ENTER_IP));

        auto control = join_lobby_screen.rect("IPAddrInput");
        if (auto result = textfield(
                Widget{control},
                TextfieldData{
                    network_info->host_ip_address(),
                    [](const std::string& content) {
                        // xxx.xxx.xxx.xxx
                        if (content.size() > 15) {
                            return TextfieldValidationDecisionFlag::
                                StopNewInput;
                        }
                        if (validate_ip(content)) {
                            return TextfieldValidationDecisionFlag::Valid;
                        }
                        return TextfieldValidationDecisionFlag::Invalid;
                    }});
            result) {
            network_info->host_ip_address() = result.as<std::string>();
        }
    }

    // Buttons
    {
        if (join_lobby_screen.button(
                "LoadLastUsedButton",
                TranslatableString(strings::i18n::LOAD_LAST_IP))) {
            network_info->host_ip_address() = Settings::get().last_used_ip();
        }

        if (join_lobby_screen.button(
                "ConnectButton", TranslatableString(strings::i18n::START))) {
            Settings::get().update_last_used_ip_address(
                network_info->host_ip_address());
            network_info->lock_in_ip();

            // TODO so this saves the "last used" but maybe we want to
            // save the "last successful"
            Settings::get().write_save_file();
        }

        if (join_lobby_screen.button(
                "BackButton", TranslatableString(strings::i18n::BACK_BUTTON))) {
            network_info->unlock_username();
        }
    }

    // Even though the ui shows up at the top
    // we dont want the tabbing to be first, so
    // we put it here
    {
        join_lobby_screen.text("UsernameText",
                               TranslatableString(strings::i18n::USERNAME));

        join_lobby_screen.text("PlayerUsernameText",
                               NO_TRANSLATE(Settings::get().data.username));

        if (join_lobby_screen.button("EditButton",
                                     TranslatableString(strings::i18n::EDIT))) {
            network_info->unlock_username();
        }
    }
}

void NetworkLayer::draw_screen(float dt) {
    if (network_info->missing_username()) {
        draw_username_picker(dt);
        return;
    }

    if (network_info->missing_role()) {
        draw_role_selector_screen(dt);
        return;
    }

    if (network_info->has_set_ip()) {
        draw_connected_screen(dt);
        return;
    }

    draw_ip_input_screen(dt);
}

void NetworkLayer::NetworkLayer::onDraw(float dt) {
    if (MenuState::get().is_not(menu::State::Network)) return;

    ext::clear_background(ui::UI_THEME.background);

    using namespace ui;

    begin(ui_context, dt);
    draw_screen(dt);
    end();

    handle_announcements();
}
