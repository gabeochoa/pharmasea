
#pragma once

#include <ostream>
#include <sstream>
#include <string>

namespace ui {

// From cppreference.com
// For two different parameters k1 and k2 that are not equal, the probability
// that std::hash<Key>()(k1) == std::hash<Key>()(k2) should be very small,
// approaching 1.0/std::numeric_limits<std::size_t>::max().
//
// so basically dont have more than numeric_limits::max() ui items per layer
//
// thx

struct uuid {
    std::size_t ownerLayer;
    std::size_t hash;

    uuid() : uuid(-99, 0, "__MAGIC__STRING__", -1) {}
    uuid(const std::string& s1, std::size_t i1) : uuid(-1, 0, s1, i1) {}

    uuid(std::size_t layer, std::size_t ownerHash, const std::string& s1,
         std::size_t i1) {
        ownerLayer = layer;
        auto h0 = std::hash<std::size_t>{}(static_cast<std::size_t>(ownerHash));
        auto h1 = std::hash<std::string>{}(s1);
        auto h2 = std::hash<std::size_t>{}(i1);
        hash = h0 ^ (h1 << 1) ^ (h2 << 2);
    }

    uuid(std::size_t o, std::size_t ownerHash, const std::string& s1,
         std::size_t i1, std::size_t index) {
        ownerLayer = o;
        auto h0 = std::hash<std::size_t>{}(static_cast<std::size_t>(ownerHash));
        auto h1 = std::hash<std::string>{}(s1);
        auto h2 = std::hash<std::size_t>{}(i1);
        auto h3 = std::hash<std::size_t>{}(index);
        hash = h0 ^ (h1 << 1) ^ (h2 << 2) ^ (h3 << 3);
    }

    uuid(const uuid& other) { this->operator=(other); }

    uuid& operator=(const uuid& other) {
        this->ownerLayer = other.ownerLayer;
        this->hash = other.hash;
        return *this;
    }

    bool operator==(const uuid& other) const {
        return ownerLayer == other.ownerLayer && hash == other.hash;
    }

    bool operator<(const uuid& other) const {
        if (ownerLayer < other.ownerLayer) return true;
        if (ownerLayer > other.ownerLayer) return false;
        if (hash < other.hash) return true;
        if (hash > other.hash) return false;
        return false;
    }

    operator std::size_t() const { return this->hash; }

    operator std::string() const {
        std::stringstream ss;
        ss << "layer: ";
        ss << this->ownerLayer;
        ss << " hash: ";
        ss << this->hash;
        return ss.str();
    }
};

inline std::ostream& operator<<(std::ostream& os, const uuid& obj) {
    os << std::string(obj);
    return os;
}

#define MK_UUID(x, parent) uuid(x, parent, __FILE__, __LINE__)
#define MK_UUID_LOOP(x, parent, index) \
    uuid(x, parent, __FILE__, __LINE__, index)

static uuid ROOT_ID = MK_UUID(-1, -1);
static uuid FAKE_ID = MK_UUID(-2, -1);

}  // namespace ui
