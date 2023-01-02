
#pragma once
#include <expected.hpp>
#include <string>

namespace network {

enum struct NetworkRequestError {
    UNKNOWN_ERROR = 0,
};

extern tl::expected<std::string, NetworkRequestError> get_remote_ip_address();

}  // namespace network
