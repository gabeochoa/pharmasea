

#pragma once

#include "../external_include.h"
#include "log.h"
//
template<typename T>
struct Library {
    std::map<std::string, T> storage;

    auto size() { return storage.size(); }
    auto begin() { return storage.begin(); }
    auto end() { return storage.end(); }
    auto begin() const { return storage.begin(); }
    auto end() const { return storage.end(); }
    auto rbegin() const { return storage.rbegin(); }
    auto rend() const { return storage.rend(); }
    auto rbegin() { return storage.rbegin(); }
    auto rend() { return storage.rend(); }
    auto empty() const { return storage.empty(); }

    const std::string add(const char* name, const T& item) {
        if (storage.find(name) != storage.end()) {
            // TODO can we throw or something? should we return Optional? are we
            // ever using the returned string?
            return "";
        }
        storage[name] = item;
        return name;
    }

    T& get(const std::string& name) {
        if (!this->contains(name)) {
            log_warn(
                "asking for item: {} but nothing has been loaded with that "
                "name yet",
                name);
        }
        return storage[name];
    }

    bool contains(const std::string& name) const {
        return (storage.find(name) != storage.end());
    }

    virtual void load(const char* filename, const char* name) = 0;

    void unload_all() {
        for (auto kv : storage) {
            unload(kv.second);
        }
    }

    virtual void unload(T) = 0;
};
