
#pragma once

#include <fstream>
#include <string>
#include <unordered_map>

#include "../ah.h"
#include "../engine/files.h"
#include "../engine/singleton.h"
#include "../strings.h"

#include <nlohmann/json.hpp>

struct TextureAtlasRegion {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    [[nodiscard]] raylib::Rectangle as_rect() const {
        return raylib::Rectangle{
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(w),
            static_cast<float>(h),
        };
    }
};

struct TextureAtlas {
    raylib::Texture2D texture{};
    std::unordered_map<std::string, TextureAtlasRegion> regions;
    int width = 0;
    int height = 0;

    [[nodiscard]] bool contains(const std::string& name) const {
        return regions.contains(name);
    }

    [[nodiscard]] const TextureAtlasRegion& get_region(
        const std::string& name) const {
        return regions.at(name);
    }

    [[nodiscard]] raylib::Rectangle get_source_rect(
        const std::string& name) const {
        return get_region(name).as_rect();
    }

    void draw_region(const std::string& name, raylib::Rectangle dest,
                     raylib::Color tint = raylib::WHITE) const {
        if (!contains(name)) {
            log_warn("TextureAtlas: region '{}' not found", name);
            return;
        }
        raylib::Rectangle src = get_source_rect(name);
        raylib::DrawTexturePro(texture, src, dest, vec2{0, 0}, 0.0f, tint);
    }

    void draw_region(const std::string& name, vec2 pos, float scale = 1.0f,
                     raylib::Color tint = raylib::WHITE) const {
        if (!contains(name)) {
            log_warn("TextureAtlas: region '{}' not found", name);
            return;
        }
        const TextureAtlasRegion& region = get_region(name);
        raylib::Rectangle dest{pos.x, pos.y, region.w * scale,
                               region.h * scale};
        raylib::Rectangle src = region.as_rect();
        raylib::DrawTexturePro(texture, src, dest, vec2{0, 0}, 0.0f, tint);
    }
};

SINGLETON_FWD(TextureAtlasLibrary)
struct TextureAtlasLibrary {
    SINGLETON(TextureAtlasLibrary)

    [[nodiscard]] auto size() const { return impl.size(); }
    void unload_all() { impl.unload_all(); }

    [[nodiscard]] bool contains(const std::string& atlas_name) const {
        return impl.contains(atlas_name);
    }

    [[nodiscard]] const TextureAtlas& get(const std::string& atlas_name) const {
        return impl.get(atlas_name);
    }

    [[nodiscard]] TextureAtlas& get(const std::string& atlas_name) {
        return impl.get(atlas_name);
    }

    void load_from_config(const std::string& atlas_name) {
        // Load atlas using standard paths:
        // texture: resources/images/{atlas_name}.png
        // manifest: resources/config/{atlas_name}.json
        impl.load(atlas_name.c_str(), atlas_name.c_str());
    }

   private:
    struct TextureAtlasLibraryImpl : afterhours::Library<TextureAtlas> {
        virtual TextureAtlas convert_filename_to_object(
            const char* name, const char* /*filename*/) override {
            TextureAtlas atlas;

            const auto texture_path = Files::get().fetch_resource_path(
                strings::settings::IMAGES, fmt::format("{}.png", name));
            const auto manifest_path = Files::get().fetch_resource_path(
                strings::settings::CONFIG, fmt::format("{}.json", name));

            // Load texture
            atlas.texture = raylib::LoadTexture(texture_path.c_str());
            if (atlas.texture.id == 0) {
                log_error("TextureAtlasLibrary: failed to load texture '{}'",
                          texture_path);
                return atlas;
            }

            // Load manifest JSON
            std::ifstream ifs(manifest_path);
            if (!ifs.good()) {
                log_error("TextureAtlasLibrary: failed to open manifest '{}'",
                          manifest_path);
                raylib::UnloadTexture(atlas.texture);
                atlas.texture = {};
                return atlas;
            }

            try {
                nlohmann::json manifest = nlohmann::json::parse(ifs);

                atlas.width = manifest.value("width", 0);
                atlas.height = manifest.value("height", 0);

                const auto& regions = manifest["regions"];
                for (auto it = regions.begin(); it != regions.end(); ++it) {
                    TextureAtlasRegion region;
                    region.x = it.value()["x"].get<int>();
                    region.y = it.value()["y"].get<int>();
                    region.w = it.value()["w"].get<int>();
                    region.h = it.value()["h"].get<int>();
                    atlas.regions[it.key()] = region;
                }

                log_info("Loaded atlas '{}' with {} regions ({}x{})", name,
                         atlas.regions.size(), atlas.width, atlas.height);

            } catch (const std::exception& e) {
                log_error(
                    "TextureAtlasLibrary: failed to parse manifest '{}': {}",
                    manifest_path, e.what());
                raylib::UnloadTexture(atlas.texture);
                atlas.texture = {};
                return atlas;
            }

            return atlas;
        }

        virtual void unload(TextureAtlas atlas) override {
            if (atlas.texture.id > 0) {
                raylib::UnloadTexture(atlas.texture);
            }
        }
    } impl;
};
