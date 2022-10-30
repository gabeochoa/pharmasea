
#pragma once

#include <HTTPRequest.hpp>
#include <iostream>
#include <optional>

namespace network {

static std::optional<std::string> get_remote_ip_address() {
    try {
        http::Request request{"http://api.ipify.org/"};
        const auto response = request.send("GET");
        return std::string{response.body.begin(), response.body.end()};
    } catch (const std::exception& e) {
        return {};
    }
}

}  // namespace network
