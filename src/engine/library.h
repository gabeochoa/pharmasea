

#pragma once

#include "../external_include.h"
#include "expected.hpp"
#include "log.h"
#include "random.h"
//
template<typename T>
struct Library {
    enum struct Error {
        DUPLICATE_NAME,
        NO_MATCH,
    };

    using Storage = std::map<std::string, T>;
    typedef typename Storage::iterator iterator;
    typedef typename Storage::const_iterator const_iterator;

    Storage storage;

    [[nodiscard]] auto size() { return storage.size(); }
    [[nodiscard]] auto begin() { return storage.begin(); }
    [[nodiscard]] auto end() { return storage.end(); }
    [[nodiscard]] auto begin() const { return storage.begin(); }
    [[nodiscard]] auto end() const { return storage.end(); }
    [[nodiscard]] auto rbegin() const { return storage.rbegin(); }
    [[nodiscard]] auto rend() const { return storage.rend(); }
    [[nodiscard]] auto rbegin() { return storage.rbegin(); }
    [[nodiscard]] auto rend() { return storage.rend(); }
    [[nodiscard]] auto empty() const { return storage.empty(); }

    const tl::expected<std::string, Error> add(const char* name,
                                               const T& item) {
        if (storage.find(name) != storage.end()) {
            return tl::unexpected(Error::DUPLICATE_NAME);
        }
        storage[name] = item;
        return name;
    }

    [[nodiscard]] T& get(const std::string& name) {
        if (!this->contains(name)) {
            log_warn(
                "asking for item: {} but nothing has been loaded with that "
                "name yet",
                name);
        }
        return storage.at(name);
    }

    [[nodiscard]] const T& get(const std::string& name) const {
        if (!this->contains(name)) {
            log_warn(
                "asking for item: {} but nothing has been loaded with that "
                "name yet",
                name);
        }
        return storage.at(name);
    }

    [[nodiscard]] bool contains(const std::string& name) const {
        return (storage.find(name) != storage.end());
    }

    virtual void load(const char* filename, const char* name) = 0;

    void unload_all() {
        for (auto kv : storage) {
            unload(kv.second);
        }
    }

    virtual void unload(T) = 0;

    [[nodiscard]] tl::expected<T, Error> get_random_match(
        const std::string& key) const {
        auto matches = lookup(key);
        size_t num_matches = std::distance(matches.first, matches.second);
        if (num_matches == 0) {
            log_warn("got no matches for your prefix search: {}", key);
            return tl::unexpected(Error::NO_MATCH);
        }

        log_info("number of matches: {}", num_matches);

        int idx = randIn(0, static_cast<int>(num_matches) - 1);
        const_iterator begin(matches.first);
        std::advance(begin, idx);
        return begin->second;
    }

    [[nodiscard]] auto lookup(const std::string& key) const
        -> std::pair<const_iterator, const_iterator> {
        auto p = storage.lower_bound(key);
        auto q = storage.end();
        if (p != q && p->first == key) {
            return std::make_pair(p, std::next(p));
        } else {
            auto r = p;
            while (r != q && r->first.compare(0, key.size(), key) == 0) {
                ++r;
            }
            return std::make_pair(p, r);
        }
    }
};
