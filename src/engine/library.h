

#pragma once

#include "../external_include.h"
#include "log.h"
//
template<typename T>
struct Library {
    std::map<std::string, T> storage;

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

    const std::string add(const char* name, const T& item) {
        if (storage.find(name) != storage.end()) {
            // TODO can we throw or something? should we return Optional? are we
            // ever using the returned string?
            return "";
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
};
