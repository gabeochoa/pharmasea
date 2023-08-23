#pragma once

#include "library.h"
#include "singleton.h"

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

    [[nodiscard]] const raylib::Font& get(const std::string& name) const {
        return impl.get(name);
    }

    void load(const FontLoadingInfo& fli, const char* name) {
        impl.store_fli(fli);
        impl.load(fli.filename, name);
    }

   private:
    struct FontLibraryImpl : Library<raylib::Font> {
        FontLoadingInfo fli;

        void store_fli(const FontLoadingInfo& _fli) { fli = _fli; }

        virtual raylib::Font convert_filename_to_object(
            const char*, const char* filename) override {
            return raylib::LoadFontEx(filename, fli.size, fli.font_chars,
                                      fli.glyph_count);
        }

        virtual void unload(raylib::Font font) override {
            raylib::UnloadFont(font);
        }
    } impl;
};
