#pragma once

#include "../engine/assert.h"
#include "../network/network.h"

namespace tests {

inline void test_network_session_state() {
    using SessionState = network::RoleInfoMixin::SessionState;

    network::RoleInfoMixin info;
    VALIDATE(info.session_state() == SessionState::NoRole,
             "default role mixin should have no role");

    info.desired_role = network::RoleInfoMixin::Role::s_Host;
    VALIDATE(info.session_state() == SessionState::Hosting,
             "host role without a joined client should be hosting");

    info.desired_role = network::RoleInfoMixin::Role::s_Client;
    VALIDATE(info.session_state() == SessionState::Joining,
             "client role without a joined id should be joining");

    info.desired_role = static_cast<network::RoleInfoMixin::Role>(
        network::RoleInfoMixin::Role::s_Host |
        network::RoleInfoMixin::Role::s_Client);
    VALIDATE(info.session_state() == SessionState::NoRole,
             "both role bits should collapse to no role");
}

}  // namespace tests
