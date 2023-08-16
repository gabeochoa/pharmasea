
#include "game.h"

namespace network {
long long total_ping = 0;
long long there_ping = 0;
long long return_ping = 0;
}  // namespace network

#include "engine/globals.h"
#include "engine/layer.h"
#include "engine/ui/ui.h"

int main(int, char* argv[]) {
    process_dev_flags(argv);

    tests::run_all();

    if (TESTS_ONLY) {
        std::cout << "All tests ran " << std::endl;
        return 0;
    }

    std::cout << "Executable Path: " << fs::current_path() << std::endl;
    std::cout << "Canon Path: " << fs::canonical(fs::current_path())
              << std::endl;

    startup();
    defer(Settings::get().write_save_file());

    try {
        App::get().run();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
