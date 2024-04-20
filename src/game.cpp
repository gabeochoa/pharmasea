
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

    // Replace the original room with empty space
    for (int i = minY; i <= maxY; i++) {
        for (int j = minX; j <= maxX; j++) {
            grid[i * width + j] = ' ';
        }
    }

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

void unifyWalls(std::vector<char>& grid, int width, int height) {
    // Replace all walls with '#'
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (grid[i * width + j] != ' ') {
                grid[i * width + j] = '#';
            }
        }
    }
}

void addDoorsBetweenRooms(std::vector<char>& grid, int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (grid[i * width + j] != ' ' && grid[i * width + j] != '#') {
                // Check if there is a room above with a different value
                if (i > 0 && grid[(i - 1) * width + j] != ' ' &&
                    grid[(i - 1) * width + j] != '#' &&
                    grid[i * width + j] != grid[(i - 1) * width + j]) {
                    grid[i * width + j - width] = ' ';  // Add door
                }
                // Check if there is a room below with a different value
                if (i < height - 1 && grid[(i + 1) * width + j] != ' ' &&
                    grid[(i + 1) * width + j] != '#' &&
                    grid[i * width + j] != grid[(i + 1) * width + j]) {
                    grid[i * width + j + width] = ' ';  // Add door
                }
                // Check if there is a room to the left with a different value
                if (j > 0 && grid[i * width + j - 1] != ' ' &&
                    grid[i * width + j - 1] != '#' &&
                    grid[i * width + j] != grid[i * width + j - 1]) {
                    grid[i * width + j - 1] = ' ';  // Add door
                }
                // Check if there is a room to the right with a different value
                if (j < width - 1 && grid[i * width + j + 1] != ' ' &&
                    grid[i * width + j + 1] != '#' &&
                    grid[i * width + j] != grid[i * width + j + 1]) {
                    grid[i * width + j + 1] = ' ';  // Add door
                }
            }
        }
    }
}

void removeSmallRooms(std::vector<char>& grid, int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (grid[i * width + j] != ' ' && grid[i * width + j] != '#') {
                bool isolated = true;
                for (int k = -1; k <= 1; k++) {
                    for (int l = -1; l <= 1; l++) {
                        int x = i + k;
                        int y = j + l;
                        if (x >= 0 && x < height && y >= 0 && y < width &&
                            grid[x * width + y] == grid[i * width + j]) {
                            isolated = false;
                            break;
                        }
                    }
                    if (!isolated) break;
                }
                if (isolated) {
                    grid[i * width + j] = ' ';  // Remove 1x1 room
                }
            }
        }
    }
}

void addBoundaryWall(std::vector<char>& grid, int width, int height) {
    // Add a wall around the grid with a single door to enter
    for (int i = 0; i < height; i++) {
        grid[i * width] = '#';              // Left wall
        grid[i * width + width - 1] = '#';  // Right wall
    }
    for (int j = 0; j < width; j++) {
        grid[j] = '#';                         // Top wall
        grid[(height - 1) * width + j] = '#';  // Bottom wall
    }
    grid[width + 1] = ' ';  // Door to enter
}

void labelRecursive(std::vector<char>& grid, int width, int height, int i,
                    int j, char label) {
    char value = grid[i * width + j];
    if (value != ' ') {
        return;
    }
    grid[i * width + j] = label;

    labelRecursive(grid, width, height, i - 1, j, label);  // Up
    labelRecursive(grid, width, height, i + 1, j, label);  // Down
    labelRecursive(grid, width, height, i, j - 1, label);  // Left
    labelRecursive(grid, width, height, i, j + 1, label);  // Right
}

char labelConnectedSections(std::vector<char>& grid, int width, int height) {
    char label = 'A';
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // we found something empty...
            if (grid[i * width + j] == ' ') {
                labelRecursive(grid, width, height, i, j, label);
                label++;
            }
        }
    }
    return label;
}

void mergeConnectedSections(std::vector<char>& grid, int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            char value = grid[i * width + j];
            if (value != 'A') continue;

            if (j + 2 < width) {
                char right = grid[(i + 0) * width + (j + 1)];
                char right2 = grid[(i + 0) * width + (j + 2)];
                if (right == '#' && right2 == 'B') {
                    grid[(i + 0) * width + (j + 1)] = 'A';
                    return;
                }
            }

            if (j - 1 >= 0) {
                char left = grid[(i + 0) * width + (j - 1)];
                char left2 = grid[(i + 0) * width + (j - 2)];

                if (left == '#' && left2 == 'B') {
                    grid[(i + 0) * width + (j - 1)] = 'A';
                    return;
                }
            }

            if (i - 2 >= 0) {
                char up = grid[(i - 1) * width + (j + 0)];
                char up2 = grid[(i - 2) * width + (j + 0)];

                if (up == '#' && up2 == 'B') {
                    grid[(i - 1) * width + (j + 0)] = 'A';
                    return;
                }
            }

            if (i + 2 < height) {
                char down = grid[(i + 1) * width + (j + 0)];
                char down2 = grid[(i + 2) * width + (j + 0)];

                if (down == '#' && down2 == 'B') {
                    grid[(i + 1) * width + (j + 0)] = 'A';
                    return;
                }
            }
        }
    }
}
void removeLabelSections(std::vector<char>& grid, int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            char value = grid[i * width + j];
            if (value == '#') continue;
            if (value == ' ') continue;
            grid[i * width + j] = ' ';
        }
    }
}

void something() {
    int width = 20;
    int height = 20;

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
    int num_seeds = (int) ((width * height) / 100);
    for (int i = 0; i < num_seeds; i++) {
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
            break;
        }
        /*
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
        */
    }

    removeSmallRooms(grid, width, height);
    print_g();
    addDoorsBetweenRooms(grid, width, height);
    print_g();
    unifyWalls(grid, width, height);
    print_g();
    addBoundaryWall(grid, width, height);
    print_g();

    char label = 'A';
    do {
        label = labelConnectedSections(grid, width, height);
        mergeConnectedSections(grid, width, height);
        removeLabelSections(grid, width, height);
    } while (label > 'B');
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
