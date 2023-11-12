
#pragma once

#include "dataclass/settings.h"
#include "engine/library.h"
#include "engine/singleton.h"

SINGLETON_FWD(ConfigValueLibrary)
struct ConfigValueLibrary {
    SINGLETON(ConfigValueLibrary)

    [[nodiscard]] auto size() { return impl.size(); }

    [[nodiscard]] const ConfigValue& get(const std::string& name) const {
        return impl.get(name);
    }

    [[nodiscard]] ConfigValue& get(const std::string& name) {
        return impl.get(name);
    }
    void load(const ConfigValue& r, const char* filename, const char* name) {
        impl.load(filename, name);

        ConfigValue& recipe = impl.get(name);
        recipe = r;

        log_info("Loaded config value named {}",
                 magic_enum::enum_name<ConfigKey>(recipe.key));
    }

    void unload_all() { impl.unload_all(); }

    [[nodiscard]] auto begin() { return impl.begin(); }
    [[nodiscard]] auto end() { return impl.end(); }
    [[nodiscard]] auto begin() const { return impl.begin(); }
    [[nodiscard]] auto end() const { return impl.end(); }

   private:
    struct ConfigValueLibraryImpl : Library<ConfigValue> {
        virtual ConfigValue convert_filename_to_object(const char*,
                                                       const char*) override {
            return ConfigValue{};
        }

        virtual void unload(ConfigValue) override {}
    } impl;
};
