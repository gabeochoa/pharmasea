#pragma once

#include "components/is_round_settings_manager.h"
#include "dataclass/settings.h"
#include "engine/library.h"
#include "engine/singleton.h"

SINGLETON_FWD(UpgradeLibrary)
struct UpgradeLibrary {
    SINGLETON(UpgradeLibrary)

    [[nodiscard]] auto size() { return impl.size(); }

    [[nodiscard]] const Upgrade& get(const std::string& name) const {
        return impl.get(name);
    }

    [[nodiscard]] Upgrade& get(const std::string& name) {
        return impl.get(name);
    }
    void load(const Upgrade& r, const char* filename, const char* name) {
        impl.load(filename, name);

        Upgrade& recipe = impl.get(name);
        recipe = r;
    }

    void unload_all() { impl.unload_all(); }

    [[nodiscard]] auto begin() { return impl.begin(); }
    [[nodiscard]] auto end() { return impl.end(); }
    [[nodiscard]] auto begin() const { return impl.begin(); }
    [[nodiscard]] auto end() const { return impl.end(); }

   private:
    struct UpgradeLibraryImpl : Library<Upgrade> {
        virtual Upgrade convert_filename_to_object(const char*,
                                                   const char*) override {
            return Upgrade{};
        }

        virtual void unload(Upgrade) override {}
    } impl;
};
