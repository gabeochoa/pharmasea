

#pragma once

#include "external_include.h"
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
            return "";
        }
        storage[name] = item;
        return name;
    }

    T& get(const std::string& name) {
        if (!this->contains(name)) {
            std::cout << "asking for item: " << name
                      << " but nothing has been loaded with that name yet"
                      << std::endl;
        }
        return storage[name];
    }
    bool contains(const std::string& name) {
        return (storage.find(name) != storage.end());
    }

    virtual void load(const char* filename, const char* name) = 0;
};
