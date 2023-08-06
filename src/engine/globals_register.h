
#pragma once

#include <map>
#include <string>

// #include "log.h"

struct GlobalValueRegister {
    std::map<std::string, void*> globals;

    template<typename T>
    [[nodiscard]] T* get_ptr(const std::string& name) const {
        // TODO do we want to catch the exception here?
        try {
            return static_cast<T*>(globals.at(name));
        } catch (const std::exception& e) {
            // TODO turn this back on
            // log_warn("Trying to fetch global of name: {} but it doesnt
            // exist", name);
            return nullptr;
        }
    }

    // TODO better error message here please
    template<typename T>
    [[nodiscard]] T get(const std::string& name) const {
        return *(get_ptr<T>(name));
    }

    template<typename T>
    [[nodiscard]] T get_or_default(const std::string& name,
                                   T default_value) const {
        if (!contains(name)) return default_value;

        T* t = get_ptr<T>(name);
        if (t) {
            return *t;
        }
        return default_value;
    }

    template<typename T>
    void set(const std::string& name, T* value) {
        globals[name] = static_cast<void*>(value);
    }

    template<typename T>
    T update(const std::string& name, T value) {
        T* t = get_ptr<T>(name);
        (*t) = value;
        return value;
    }

    // TODO need an api to remove a global for example when we unload some
    // loaded info

    // TODO what we can we do to make this api more valuable to callers
    // it seems like today that no one uses this but people seem to like
    // get_or_default
    [[nodiscard]] bool contains(const std::string& name) const {
        return globals.find(name) != globals.end();
    }
};

extern GlobalValueRegister GLOBALS;
