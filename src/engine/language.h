

#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "../vendor_include.h"
// TODO dont reach out of engine
#include "../strings.h"
#include "files.h"
#include "singleton.h"

namespace lang {
constexpr int MAX_STRING_LENGTH = 20;

struct LanguageInfo {
    std::string name;
    std::string filename;

    bool operator<(const LanguageInfo& r) const { return name < r.name; }
    bool operator==(const LanguageInfo& r) const { return name == r.name; }

    // LanguageInfo(const LanguageInfo& other) {
    // this->name = other.name;
    // this->filename = other.filename;
    // }
    //
    // LanguageInfo& operator=(const LanguageInfo other) {
    // this->name = other.name;
    // this->filename = other.filename;
    // return *this;
    // }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.text1b(name, MAX_STRING_LENGTH);
        s.text1b(filename, MAX_STRING_LENGTH);
    }
};

inline std::ostream& operator<<(std::ostream& os, const LanguageInfo& info) {
    os << "Language: " << info.name << " fn: " << info.filename << std::endl;
    return os;
}

SINGLETON_FWD(LanguageExplorer)
struct LanguageExplorer {
    SINGLETON(LanguageExplorer)

    std::vector<LanguageInfo> options;
    std::vector<std::string> options_str;

    [[nodiscard]] const LanguageInfo& fetch(int index) const {
        return options[index];
    }

    void load_language_options() {
        Files::get().for_resources_in_group(
            strings::settings::TRANSLATIONS,
            [&](const std::string& name, const std::string& filename,
                const std::string& extension) {
                if (extension != ".mo") return;
                log_info("adding {} {} {}", name, filename, extension);

                this->options.push_back(
                    LanguageInfo{.name = name, .filename = filename});
            });

        if (options.empty()) {
            this->options.push_back(LanguageInfo{.name = "English Reverse",
                                                 .filename = "en_rev.mo"});
        }

        log_info("loaded {} language options", options.size());

        for (auto li : options) {
            this->options_str.push_back(li.name);
        }
    }

    // Saving them is just an optimization so we dont have to fetch them all the
    // time it also means that the index is relatively stable at least once per
    // launch
    [[nodiscard]] std::vector<std::string> fetch_options() {
        if (options.empty()) load_language_options();
        return options_str;
    }

    [[nodiscard]] int index(const LanguageInfo& info) const {
        int i = 0;
        for (const auto& res : options) {
            if (res == info) return i;
            i++;
        }
        return -1;
    }

    [[nodiscard]] int index(const std::string& name) const {
        int i = 0;
        for (const auto& res : options) {
            if (res.name == name) return i;
            i++;
        }
        return -1;
    }
};

}  // namespace lang
