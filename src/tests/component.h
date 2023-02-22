

#include "../entity.h"
//
#include "../engine/defer.h"
#include "../network/shared.h"

namespace network {
using Buffer = std::string;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;
using TContext =
    std::tuple<bitsery::ext::PointerLinkingContext,
               bitsery::ext::PolymorphicContext<bitsery::ext::StandardRTTI>>;
using BitserySerializer = bitsery::Serializer<OutputAdapter, TContext>;
using BitseryDeserializer = bitsery::Deserializer<InputAdapter, TContext>;

Buffer serialize(Entity* entity) {
    Buffer buffer;
    TContext ctx{};

    std::get<1>(ctx).registerBasesList<BitserySerializer>(
        MyPolymorphicClasses{});
    BitserySerializer ser{ctx, buffer};
    ser.object(*entity);
    ser.adapter().flush();

    return buffer;
}

void deserialize(Entity* entity, std::string msg) {
    TContext ctx{};
    std::get<1>(ctx).registerBasesList<BitseryDeserializer>(
        MyPolymorphicClasses{});

    BitseryDeserializer des{ctx, msg.begin(), msg.size()};
    des.object(*entity);
}

}  // namespace network

void compare_and_validate_components(Entity* a, Entity* b) {
    M_ASSERT(a->componentSet == b->componentSet, "component sets should match");

    int i = 0;
    while (i < max_num_components) {
        auto eC = a->componentArray[i];
        auto e2C = b->componentArray[i];

        if ((eC == nullptr && e2C != nullptr) ||
            (eC != nullptr && e2C == nullptr)) {
            M_ASSERT(false, "one was null but other wasnt");
        }

        i++;
    }
}

void entity_components() {
    Entity* entity = new Entity({0, 0, 0}, WHITE, WHITE);

    network::Buffer buff = network::serialize(entity);

    Entity* entity2 = new Entity({0, 0, 0}, WHITE, WHITE);
    network::deserialize(entity2, buff);

    compare_and_validate_components(entity, entity2);

    delete entity;
    delete entity2;
}

void remote_player_components() {
    Entity* entity = make_remote_player({0, 0, 0});

    network::Buffer buff = network::serialize(entity);

    Entity* entity2 = make_remote_player({0, 0, 0});
    network::deserialize(entity2, buff);

    compare_and_validate_components(entity, entity2);

    delete entity;
    delete entity2;
}

void customer_components() {
    Entity* entity = make_customer({0, 0, 0});

    network::Buffer buff = network::serialize(entity);

    Entity* entity2 = make_customer({0, 0, 0});
    network::deserialize(entity2, buff);

    compare_and_validate_components(entity, entity2);

    delete entity;
    delete entity2;
}

void all_tests() {
    entity_components();
    // remote_player_components();
    customer_components();
}
