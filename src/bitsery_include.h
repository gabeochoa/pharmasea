
#pragma once

#define CEREAL_EXCEPTIONS

#undef CEREAL_THREAD_SAFE
#define CEREAL_THREAD_SAFE 1
#include <cereal/types/polymorphic.hpp>
//
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/bitset.hpp>
#include <cereal/types/deque.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>

CEREAL_REGISTER_ARCHIVE(cereal::JSONInputArchive)
CEREAL_REGISTER_ARCHIVE(cereal::JSONOutputArchive)
CEREAL_REGISTER_ARCHIVE(cereal::PortableBinaryInputArchive)
CEREAL_REGISTER_ARCHIVE(cereal::PortableBinaryOutputArchive)
