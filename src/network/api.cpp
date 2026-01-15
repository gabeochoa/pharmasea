#include "api.h"

#include "network.h"

namespace network {

void init_connections() { Info::init_connections(); }
void reset_connections() { Info::reset_connections(); }
void shutdown_connections() { Info::shutdown_connections(); }

}  // namespace network