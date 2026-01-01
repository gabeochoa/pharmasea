#include <cassert>
#include <cstddef>
#include <optional>
#include <string>

#include "bitsery/ext/pointer.h"

#include "bitsery_include.h"
#include "network/polymorphic_components.h"
#include "save_game/save_game.h"

namespace {
using Buffer = std::string;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

// NOTE: This matches current runtime contexts used by network/save_game.
using TContext =
    std::tuple<bitsery::ext::PointerLinkingContext,
               bitsery::ext::PolymorphicContext<bitsery::ext::StandardRTTI>>;
using Serializer = bitsery::Serializer<OutputAdapter, TContext>;
using Deserializer = bitsery::Deserializer<InputAdapter, TContext>;

template<typename T>
Buffer serialize_to_string(const T& obj) {
    Buffer buffer;
    TContext ctx{};
    std::get<1>(ctx).registerBasesList<Serializer>(MyPolymorphicClasses{});
    Serializer ser{ctx, buffer};
    // bitsery doesn't take const here; it serializes by reading, so const-cast is safe.
    auto& mut = const_cast<T&>(obj);
    ser.object(mut);
    ser.adapter().flush();
    return buffer;
}

template<typename T>
std::optional<T> deserialize_from_string(const Buffer& buf) {
    TContext ctx{};
    std::get<1>(ctx).registerBasesList<Deserializer>(MyPolymorphicClasses{});
    Deserializer des{ctx, buf.begin(), buf.size()};
    T out{};
    des.object(out);
    if (des.adapter().error() != bitsery::ReaderError::NoError) return std::nullopt;
    return out;
}
}  // namespace

int main() {
    using save_game::SaveGameFile;

    SaveGameFile in{};
    // Keep this minimal and deterministic: an empty world snapshot.
    in.header.magic = "PHARMSAVE";
    in.header.save_version = 1;
    in.map_snapshot.showMinimap = false;

    // IMPORTANT: LevelInfo serialization uses num_entities as the container size.
    in.map_snapshot.game_info.entities.clear();
    in.map_snapshot.game_info.num_entities = in.map_snapshot.game_info.entities.size();
    in.map_snapshot.game_info.seed = "";
    in.map_snapshot.game_info.hashed_seed = 0;
    in.map_snapshot.game_info.was_generated = false;

    Buffer blob = serialize_to_string(in);
    assert(!blob.empty());

    auto out_opt = deserialize_from_string<SaveGameFile>(blob);
    assert(out_opt.has_value());
    const SaveGameFile& out = *out_opt;

    // Basic invariants.
    assert(out.header.magic == "PHARMSAVE");
    assert(out.map_snapshot.game_info.entities.size() == 0);
    assert(out.map_snapshot.game_info.num_entities == 0);

    return 0;
}

