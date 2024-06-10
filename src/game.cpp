

#include "game.h"

#include "engine/assert.h"
#include "engine/ui/svg.h"
#include "map_generation.h"

// TODO cleaning bots place some shiny on the floor so its harder to get dirtyu
// TODO fix bug where "place ghost" green box kept showing
// TODO conveyerbelt speed
// TODO tutorial should only apply when host has it on
// TODO save tutorial in save file
// TODO fastforward should not work in shop mode
//  => this is not a problem except when the state gets messed up
// TODO affect tip based on how much vomit is around?
// => how can we communicate this to the player

namespace network {
long long total_ping = 0;
long long there_ping = 0;
long long return_ping = 0;
}  // namespace network
std::shared_ptr<network::Info> network_info;

int main(int, char* argv[]) {
    process_dev_flags(argv);

    tests::run_all();

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
    try {
        App::get().run();
    } catch (const std::exception& e) {
        std::cout << "App run exception: " << e.what() << std::endl;
    }
    return 0;
}
