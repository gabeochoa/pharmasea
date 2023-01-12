
#pragma once

#include <map>
#include <string>

struct GlobalValueRegister {
    std::map<std::string, void*> globals;

    template<typename T>
    [[nodiscard]] T* get_ptr(const std::string& name) const {
        return (T*) (globals.at(name));
    }

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
        globals[name] = (void*) value;
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

static GlobalValueRegister GLOBALS;
