#include "serialization.h"

#include "../engine/log.h"
#include "../entity.h"
#include "../globals.h"
#include "polymorphic_components.h"

namespace network {

Buffer serialize_to_entity(Entity* entity) {
    Buffer buffer;
    TContext ctx{};

    std::get<1>(ctx).registerBasesList<BitserySerializer>(
        MyPolymorphicClasses{});

    // Debug: Print type hashes
    printf("[SERIALIZE DEBUG] afterhours::BaseComponent hash: %zu, name: %s\n",
           typeid(afterhours::BaseComponent).hash_code(),
           typeid(afterhours::BaseComponent).name());
    printf("[SERIALIZE DEBUG] BaseComponent hash: %zu, name: %s\n",
           typeid(BaseComponent).hash_code(), typeid(BaseComponent).name());
    fflush(stdout);

    BitserySerializer ser{ctx, buffer};
    ser.object(*entity);
    ser.adapter().flush();

    return buffer;
}

void deserialize_to_entity(Entity* entity, const std::string& msg) {
    TContext ctx{};
    std::get<1>(ctx).registerBasesList<BitseryDeserializer>(
        MyPolymorphicClasses{});

    BitseryDeserializer des{ctx, msg.begin(), msg.size()};
    des.object(*entity);

    switch (des.adapter().error()) {
        case bitsery::ReaderError::NoError:
            break;
        case bitsery::ReaderError::ReadingError:
            log_error("reading error");
            break;
        case bitsery::ReaderError::DataOverflow:
            log_error("data overflow error");
            break;
        case bitsery::ReaderError::InvalidData:
            log_error("invalid data error");
            break;
        case bitsery::ReaderError::InvalidPointer:
            log_error("invalid pointer error");
            break;
    }
    assert(des.adapter().error() == bitsery::ReaderError::NoError);
}

ClientPacket deserialize_to_packet(const std::string& msg) {
    TContext ctx{};
    std::get<1>(ctx).registerBasesList<BitseryDeserializer>(
        MyPolymorphicClasses{});

    BitseryDeserializer des{ctx, msg.begin(), msg.size()};

    ClientPacket packet;
    des.object(packet);
    // TODO obviously theres a ton of validation we can do here but idk
    // https://github.com/fraillt/bitsery/blob/master/examples/smart_pointers_with_polymorphism.cpp
    return packet;
}

Buffer serialize_to_buffer(ClientPacket packet) {
    Buffer buffer;
    TContext ctx{};

    std::get<1>(ctx).registerBasesList<BitserySerializer>(
        MyPolymorphicClasses{});
    BitserySerializer ser{ctx, buffer};
    ser.object(packet);
    ser.adapter().flush();

    return buffer;
}

}  // namespace network
