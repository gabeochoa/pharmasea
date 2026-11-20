#pragma once

#include "../ah.h"
#include "../engine/singleton.h"

struct FontLoadingInfo {
    const char* filename;
    int size = 400;
    int* font_chars = 0;
    int glyph_count = 0;
};

SINGLETON_FWD(FontLibrary)
struct FontLibrary {
    SINGLETON(FontLibrary)

    [[nodiscard]] auto size() { return impl.size(); }
    void unload_all() { impl.unload_all(); }

    [[nodiscard]] raylib::Font& get(const std::string& name) {
        return impl.get(name);
    }

    [[nodiscard]] bool contains(const std::string& name) const {
        return impl.contains(name);
    }

    [[nodiscard]] const raylib::Font& get(const std::string& name) const {
        return impl.get(name);
    }

    void load(const FontLoadingInfo& fli, const char* name) {
        impl.store_fli(fli);
        impl.load(fli.filename, name);
    }

   private:
    struct FontLibraryImpl : afterhours::Library<raylib::Font> {
        FontLoadingInfo fli;

        void store_fli(const FontLoadingInfo& _fli) { fli = _fli; }

        virtual raylib::Font convert_filename_to_object(
            const char*, const char* filename) override {
            raylib::Font font = raylib::LoadFontEx(
                filename, fli.size, fli.font_chars, fli.glyph_count);
            // raylib only uploads the glyph atlas when a window
            // exists; headless runs redo the load manually (G-P10).
            // (LoadFontEx already consumed the glyph images, so the
            // atlas cannot be regenerated from its result.)
            if (font.texture.id == 0 && font.glyphs != nullptr) {
                int data_size = 0;
                unsigned char* data =
                    raylib::LoadFileData(filename, &data_size);
                if (data != nullptr) {
                    raylib::UnloadFont(font);
                    font = raylib::Font{};
                    font.baseSize = fli.size;
                    font.glyphPadding = 4;
                    font.glyphCount =
                        fli.glyph_count > 0 ? fli.glyph_count : 95;
                    font.glyphs = raylib::LoadFontData(
                        data, data_size, fli.size, fli.font_chars,
                        fli.glyph_count, raylib::FONT_DEFAULT);
                    if (font.glyphs != nullptr) {
                        raylib::Image atlas = raylib::GenImageFontAtlas(
                            font.glyphs, &font.recs, font.glyphCount,
                            fli.size, 4, 0);
                        font.texture = raylib::LoadTextureFromImage(atlas);
                        raylib::UnloadImage(atlas);
                    }
                    raylib::UnloadFileData(data);
                }
            }
            return font;
        }

        virtual void unload(raylib::Font font) override {
            raylib::UnloadFont(font);
        }
    } impl;
};
