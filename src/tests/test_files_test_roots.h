#pragma once

#include <fstream>

#include <afterhours/src/plugins/files.h>

#include "../engine/assert.h"
#include "../engine/files.h"

namespace tests {

inline void test_files_test_roots() {
    const fs::path real_folder = Files::get().game_folder();
    fs::path temp_root;
    {
        afterhours::files::TestRoots roots;
        temp_root = roots.root;

        VALIDATE(Files::get().game_folder() == roots.save,
                 "test roots should redirect the game folder");
        VALIDATE(Files::get().settings_filepath() ==
                     roots.save / Files::get().settings_file,
                 "settings should resolve under the test root");

        std::ofstream out(Files::get().settings_filepath());
        out << "test";
        out.close();
        VALIDATE(fs::exists(Files::get().settings_filepath()),
                 "writes should land under the test root");
    }
    VALIDATE(Files::get().game_folder() == real_folder,
             "game folder should restore after test roots");
    VALIDATE(!fs::exists(temp_root), "test root should be removed");
}

}  // namespace tests
