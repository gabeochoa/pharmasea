

#pragma once

#include <yojimbo/yojimbo.h>

namespace network {

static const uint8_t DEFAULT_PRIVATE_KEY[yojimbo::KeyBytes] = {0};
static const int MAX_PLAYERS = 4;

// a simple test message
enum class PharmaSeaMessageType { TEST, COUNT };

// two channels, one for each type that Yojimbo supports
enum class PharmaSeaChannel { RELIABLE, UNRELIABLE, COUNT };

class TestMessage : public yojimbo::Message {
   public:
    int data;

    TestMessage() : data(0) {}

    template<typename Stream>
    bool Serialize(Stream& stream) {
        serialize_int(stream, data, 0, 512);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

// the message factory
YOJIMBO_MESSAGE_FACTORY_START(PharmaSeaMessageFactory,
                              (int) PharmaSeaMessageType::COUNT)
YOJIMBO_DECLARE_MESSAGE_TYPE((int) PharmaSeaMessageType::TEST, TestMessage)
YOJIMBO_MESSAGE_FACTORY_FINISH()

}  // namespace network
