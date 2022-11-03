
#pragma once

#include <map>
#include <string>

struct GlobalValueRegister {
    std::map<std::string, void*> globals;

    template<typename T>
    T* get_ptr(const std::string& name) {
        return (T*) (globals[name]);
    }

    template<typename T>
    T get(const std::string& name) {
        return *(get_ptr<T>(name));
    }

    template<typename T>
    T get_or_default(const std::string& name, T default_value) {
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

    bool contains(const std::string& name) {
        return globals.find(name) != globals.end();
    }
};

static GlobalValueRegister GLOBALS;
