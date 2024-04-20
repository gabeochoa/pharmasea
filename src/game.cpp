
#include "game.h"

#include "engine/assert.h"
#include "map_generation.h"

namespace network {
long long total_ping = 0;
long long there_ping = 0;
long long return_ping = 0;
}  // namespace network
std::shared_ptr<network::Info> network_info;

#include <queue>

struct Room {
    int x;
    int y;
    int w;
    int h;
};

bool expandRoom(std::vector<char>& grid, int width, int height, char letter) {
    // Find the room boundaries
    int minX = width, maxX = 0, minY = height, maxY = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (grid[i * width + j] == letter) {
                minX = std::min(minX, j);
                maxX = std::max(maxX, j);
                minY = std::min(minY, i);
                maxY = std::max(maxY, i);
            }
        }
    }

    // Check if expanding the room will overwrite any non-' ' characters
    bool willOverwrite = false;
    for (int i = std::max(0, minY - 1); i <= std::min(height - 1, maxY + 1);
         i++) {
        for (int j = std::max(0, minX - 1); j <= std::min(width - 1, maxX + 1);
             j++) {
            if (grid[i * width + j] != ' ' && grid[i * width + j] != letter) {
                willOverwrite = true;
                break;
            }
        }
        if (willOverwrite) break;
    }

    if (willOverwrite) return false;

    /*
    // Replace the original room with empty space
    for (int i = minY; i <= maxY; i++) {
        for (int j = minX; j <= maxX; j++) {
            grid[i * width + j] = ' ';
        }
    }
    */

    // Expand the room
    for (int i = std::max(0, minY - 1); i <= std::min(height - 1, maxY + 1);
         i++) {
        for (int j = std::max(0, minX - 1); j <= std::min(width - 1, maxX + 1);
             j++) {
            if (i == minY - 1 || i == maxY + 1 || j == minX - 1 ||
                j == maxX + 1) {
                grid[i * width + j] = letter;
            }
        }
    }
    return true;
}

bool any_empty(const std::vector<char>& grid, int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (grid[i * width + j] == ' ') {
                return true;
            }
        }
    }
    return false;
}

void something() {
    int width = 10;
    int height = 10;

    std::vector<char> required = {{
        generation::CUST_SPAWNER,
        generation::SODA_MACHINE,
        generation::TRASH,
        generation::REGISTER,
        generation::ORIGIN,

        generation::TABLE,
        generation::TABLE,
        generation::TABLE,
        generation::TABLE,

    }};

    char a = 48;

    std::vector<char> grid;
    for (int i = 0; i < width * height; i++) {
        grid.push_back(' ');
    }

    // get seeds
    std::vector<vec2> seeds;
    for (int i = 0; i < 3; i++) {
        seeds.push_back(vec2{
            (float) randIn(0, width),
            (float) randIn(0, height),
        });
    }

    for (size_t i = 0; i < seeds.size(); i++) {
        vec2 seed = seeds[i];
        int index = (int) (floor(seed.x) * height + floor(seed.y));
        grid[index] = (char) (a + i);
    }

    auto print_g = [&]() {
        std::cout << grid.size() << std::endl;
        std::cout << "====" << std::endl;
        int i = 0;
        for (auto c : grid) {
            std::cout << c;
            i++;
            if (i >= width) {
                i = 0;
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
    };

    print_g();

    while (any_empty(grid, width, height)) {
        int not_expanded = 0;
        for (size_t i = 0; i < seeds.size(); i++) {
            bool expanded = expandRoom(grid, width, height, (char) (a + i));
            if (!expanded) not_expanded++;
        }
        print_g();
        if (not_expanded >= (int) seeds.size()) {
            int x, y;
            do {
                x = rand() % width;
                y = rand() % height;
            } while (grid[y * width + x] != ' ');
            grid[y * width + x] = (char) (a + seeds.size());
            seeds.push_back(vec2{(float) x, (float) y});
        }
        if (seeds.size() > 255) {
            break;
        }
    }

    print_g();
}

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

    something();
    return 0;

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
