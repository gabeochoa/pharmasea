
#include "game.h"

#include "engine/assert.h"
#include "engine/ui/svg.h"
#include "map_generation.h"

// TODO add more cleaning bots in the store
// TODO cleaning bots place some shiny on the floor so its harder to get dirtyu
// TODO fix bug where "place ghost" green box kept showing
// TODO add a auto-mop where you can just hold it over the dirt (you can already
// do this with the roomba)

namespace network {
long long total_ping = 0;
long long there_ping = 0;
long long return_ping = 0;
}  // namespace network
std::shared_ptr<network::Info> network_info;

int main(int argc, char* argv[]) {
    process_dev_flags(argv);

    // log nothing during the test
    LOG_LEVEL = 6;
    tests::run_all();
    // back to default , preload will set it right
    LOG_LEVEL = 2;

    if (TESTS_ONLY) {
        log_info("All tests ran ");
        return 0;
    }

    log_info("Executable Path: {}", fs::current_path());
    log_info("Canon Path: {}", fs::canonical(fs::current_path()));

    startup();

    // wfc::WaveCollapse wc(static_cast<unsigned
    // int>(hashString("WVZ_ORYYVAV"))); wc.run(); wc.get_lines(); return 0;
    //
    if (argc > 1) {
        bool is_test = strcmp(argv[1], "test") == 0;
        if (is_test) {
            bool is_host = strcmp(argv[2], "host") == 0;
            int a = setup_multiplayer_test(is_host);
            if (a < 0) {
                return -1;
            }
        }
    }

    try {
        App::get().run();
    } catch (const std::exception& e) {
        std::cout << "App run exception: " << e.what() << std::endl;
    }
    return 0;
}
