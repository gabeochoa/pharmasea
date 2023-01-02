
#include "webrequest.h"

#include "../log.h"
#include <HTTPRequest.hpp>


namespace network {
tl::expected<std::string, NetworkRequestError> get_remote_ip_address() {
    try {
        http::Request request{"http://api.ipify.org/"};
        const auto response = request.send("GET");
        return std::string{response.body.begin(), response.body.end()};
    } catch (const std::exception& e) {
        log_warn("Got an error while fetching remote ip add: {}", e.what());
        return tl::unexpected(NetworkRequestError::UNKNOWN_ERROR);
    }
}
}  // namespace network
