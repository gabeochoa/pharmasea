#include "serialization.h"

#include "../engine/assert.h"
#include "../engine/log.h"
#include "../entity.h"
#include "../globals.h"
#include "../serialization/world_snapshot_blob.h"

namespace network {

Buffer serialize_to_entity(Entity* entity) {
    if (!entity) return {};
    return snapshot_blob::encode_entity(*entity);
}

void deserialize_to_entity(Entity* entity, const std::string& msg) {
    if (!entity) return;
    const bool ok = snapshot_blob::decode_into_entity(*entity, msg);
    if (!ok) {
        log_error("deserialize_to_entity failed (invalid snapshot blob)");
        INVARIANT_MSG(ok, "deserialize_to_entity failed (invalid snapshot blob)");
    }
}

ClientPacket deserialize_to_packet(const std::string& msg) {
    ClientPacket packet;
    zpp::bits::in in{msg};
    (void) in(packet);
    // TODO obviously theres a ton of validation we can do here but idk
    // https://github.com/fraillt/bitsery/blob/master/examples/smart_pointers_with_polymorphism.cpp
    return packet;
}

Buffer serialize_to_buffer(ClientPacket packet) {
    Buffer buffer;
    zpp::bits::out out{buffer};
    (void) out(packet);

    return buffer;
}

}  // namespace network
