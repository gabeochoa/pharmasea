
#pragma once

#include <any>
#include <cassert>
#include <functional>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <memory>

// #include "log.h"

struct GlobalValueRegister {
    // A process-wide "registry" for cross-cutting values and service pointers.
    //
    // This used to store `void*` and required callers to register pointers to
    // live storage. That pattern caused lifetime hazards (dangling pointers to
    // members/stack vars) and made it hard to store simple values like bools.
    //
    // This version stores values using `std::any`. You can still register
    // service pointers via `set_ptr()`, but value-like entries should use
    // `set_value()` so they don't depend on external lifetime.
    //
    // NOTE(threading): This container is guarded by a mutex, but that does NOT
    // magically make the *pointed-to objects* thread-safe. Avoid cross-thread
    // access to complex structures via `GLOBALS`.
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::any> globals;

    template<typename T>
    [[nodiscard]] T* get_ptr(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = globals.find(name);
        if (it == globals.end()) return nullptr;

        // Stored as a raw pointer: `T*`.
        if (auto pp = std::any_cast<T*>(&it->second)) {
            return *pp;
        }

        // Stored as a shared_ptr<T>.
        if (auto sp = std::any_cast<std::shared_ptr<T>>(&it->second)) {
            return sp->get();
        }

        // Stored as a reference_wrapper<T>.
        if (auto rw = std::any_cast<std::reference_wrapper<T>>(&it->second)) {
            return &rw->get();
        }

        return nullptr;
    }

    // TODO better error message here please
    template<typename T>
    [[nodiscard]] T get(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = globals.find(name);
        if (it == globals.end()) {
            assert(false && "GLOBALS missing key");
            static_assert(std::is_default_constructible_v<T>,
                          "GLOBALS.get<T> requires T be default constructible "
                          "when key is missing");
            return T{};
        }

        // Stored as a direct value: `T`.
        if (auto pv = std::any_cast<T>(&it->second)) {
            return *pv;
        }

        // Stored as a pointer: `T*` (legacy pattern).
        if (auto pp = std::any_cast<T*>(&it->second)) {
            if (!*pp) return T{};
            return **pp;  // Note: caller must ensure pointed-to lifetime.
        }

        // Stored as a shared_ptr<T>.
        if (auto sp = std::any_cast<std::shared_ptr<T>>(&it->second)) {
            if (!(*sp)) return T{};
            return **sp;
        }

        // Stored as a reference_wrapper<T>.
        if (auto rw = std::any_cast<std::reference_wrapper<T>>(&it->second)) {
            return rw->get();
        }

        // Type mismatch (programmer error).
        assert(false && "GLOBALS key has unexpected type");
        return T{};
    }

    template<typename T>
    [[nodiscard]] T get_or_default(const std::string& name,
                                   T default_value) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = globals.find(name);
        if (it == globals.end()) return default_value;

        // Value stored directly.
        if (auto pv = std::any_cast<T>(&it->second)) {
            return *pv;
        }

        // Legacy: value stored indirectly via pointer.
        if (auto pp = std::any_cast<T*>(&it->second)) {
            if (!*pp) return default_value;
            return **pp;
        }

        // Stored as a reference_wrapper<T>.
        if (auto rw = std::any_cast<std::reference_wrapper<T>>(&it->second)) {
            return rw->get();
        }

        return default_value;
    }

    template<typename T>
    void set_ptr(const std::string& name, T* value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        globals[name] = value;
    }

    template<typename T>
    void set_value(const std::string& name, T value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        globals[name] = std::move(value);
    }

    template<typename T>
    T update_value(const std::string& name, T value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        globals[name] = value;
        return value;
    }

    // TODO need an api to remove a global for example when we unload some
    // loaded info

    // TODO what we can we do to make this api more valuable to callers
    // it seems like today that no one uses this but people seem to like
    // get_or_default
    [[nodiscard]] bool contains(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return globals.find(name) != globals.end();
    }
};

extern GlobalValueRegister GLOBALS;
