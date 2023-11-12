#pragma once

#include "components/is_round_settings_manager.h"
#include "engine/library.h"
#include "engine/singleton.h"

enum struct Operation { Multiplier, Set };

struct UpgradeEffect {
    IsRoundSettingsManager::Config::Key name;
    Operation operation;
    std::variant<int, float, bool> value;
};

struct UpgradeRequirement {
    IsRoundSettingsManager::Config::Key name;
    std::variant<int, float, bool> value;
};

struct Upgrade {
    std::string name;
    std::string flavor_text;
    std::string description;
    std::vector<UpgradeEffect> effects;
};

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
