#pragma once

#include <cstddef>
#include <queue>
#include <vector>

#include "engine/random_engine.h"
#include "map_generation.h"

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

    // Check if expanding the room will overwrite any non-generation::EMPTY
    // characters
    bool willOverwrite = false;
    for (int i = std::max(0, minY - 1); i <= std::min(height - 1, maxY + 1);
         i++) {
        for (int j = std::max(0, minX - 1); j <= std::min(width - 1, maxX + 1);
             j++) {
            if (grid[i * width + j] != generation::EMPTY &&
                grid[i * width + j] != letter) {
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
            grid[i * width + j] = generation::EMPTY;
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
            if (grid[i * width + j] == generation::EMPTY) {
                return true;
            }
        }
    }
    return false;
}

void unifyWalls(std::vector<char>& grid, int width, int height) {
    // Replace all walls with generation::WALL
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (grid[i * width + j] != generation::EMPTY) {
                grid[i * width + j] = generation::WALL;
            }
        }
    }
}

void addDoorsBetweenRooms(std::vector<char>& grid, int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (grid[i * width + j] != generation::EMPTY &&
                grid[i * width + j] != generation::WALL) {
                // Check if there is a room above with a different value
                if (i > 0 && grid[(i - 1) * width + j] != generation::EMPTY &&
                    grid[(i - 1) * width + j] != generation::WALL &&
                    grid[i * width + j] != grid[(i - 1) * width + j]) {
                    grid[i * width + j - width] =
                        generation::EMPTY;  // Add door
                }
                // Check if there is a room below with a different value
                if (i < height - 1 &&
                    grid[(i + 1) * width + j] != generation::EMPTY &&
                    grid[(i + 1) * width + j] != generation::WALL &&
                    grid[i * width + j] != grid[(i + 1) * width + j]) {
                    grid[i * width + j + width] =
                        generation::EMPTY;  // Add door
                }
                // Check if there is a room to the left with a different value
                if (j > 0 && grid[i * width + j - 1] != generation::EMPTY &&
                    grid[i * width + j - 1] != generation::WALL &&
                    grid[i * width + j] != grid[i * width + j - 1]) {
                    grid[i * width + j - 1] = generation::EMPTY;  // Add door
                }
                // Check if there is a room to the right with a different value
                if (j < width - 1 &&
                    grid[i * width + j + 1] != generation::EMPTY &&
                    grid[i * width + j + 1] != generation::WALL &&
                    grid[i * width + j] != grid[i * width + j + 1]) {
                    grid[i * width + j + 1] = generation::EMPTY;  // Add door
                }
            }
        }
    }
}

void removeSmallRooms(std::vector<char>& grid, int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (grid[i * width + j] != generation::EMPTY &&
                grid[i * width + j] != generation::WALL) {
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
                    grid[i * width + j] = generation::EMPTY;  // Remove 1x1 room
                }
            }
        }
    }
}

void addBoundaryWall(std::vector<char>& grid, int width, int height) {
    // Add a wall around the grid with a single door to enter
    for (int i = 0; i < height; i++) {
        grid[i * width] = generation::WALL;              // Left wall
        grid[i * width + width - 1] = generation::WALL;  // Right wall
    }
    for (int j = 0; j < width; j++) {
        grid[j] = generation::WALL;                         // Top wall
        grid[(height - 1) * width + j] = generation::WALL;  // Bottom wall
    }
    grid[width + 1] = generation::EMPTY;  // Door to enter
}

void labelRecursive(std::vector<char>& grid, int width, int height, int i,
                    int j, char label) {
    char value = grid[i * width + j];
    if (value != generation::EMPTY) {
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
            if (grid[i * width + j] == generation::EMPTY) {
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
                if (right == generation::WALL && right2 == 'B') {
                    grid[(i + 0) * width + (j + 1)] = 'A';
                    return;
                }
            }

            if (j - 1 >= 0) {
                char left = grid[(i + 0) * width + (j - 1)];
                char left2 = grid[(i + 0) * width + (j - 2)];

                if (left == generation::WALL && left2 == 'B') {
                    grid[(i + 0) * width + (j - 1)] = 'A';
                    return;
                }
            }

            if (i - 2 >= 0) {
                char up = grid[(i - 1) * width + (j + 0)];
                char up2 = grid[(i - 2) * width + (j + 0)];

                if (up == generation::WALL && up2 == 'B') {
                    grid[(i - 1) * width + (j + 0)] = 'A';
                    return;
                }
            }

            if (i + 2 < height) {
                char down = grid[(i + 1) * width + (j + 0)];
                char down2 = grid[(i + 2) * width + (j + 0)];

                if (down == generation::WALL && down2 == 'B') {
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
            if (value == generation::WALL) continue;
            if (value == generation::EMPTY) continue;
            grid[i * width + j] = generation::EMPTY;
        }
    }
}

void makeEntrance(std::vector<char>& grid, int width, int height) {
    const auto px = [&](int i, int j) -> char {
        if (i < 0) return 0;
        if (i > width) return 0;
        if (j < 0) return 0;
        if (j > height) return 0;
        return grid[i * width + j];
    };

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (px(i, j) == generation::WALL && px(i - 1, j) == 0 &&
                px(i + 1, j) == generation::EMPTY) {
                grid[i * width + j] = generation::EMPTY;
                break;
            }
        }
    }
}

inline void addOrigin(std::vector<char>& grid, int width, int height) {
    int midx = (int) (width / 2);
    int midy = (int) (height / 2);
    grid[midx * width + midy] = generation::ORIGIN;
}

inline std::vector<char> something(int width, int height) {
    char a = 48;

    std::vector<char> grid;
    grid.reserve(width * height);
    for (int i = 0; i < width * height; i++) {
        grid.push_back(generation::EMPTY);
    }

    // get seeds
    std::vector<vec2> seeds;
    int num_seeds = (int) ((width * height) / 100);
    seeds.reserve(num_seeds);
    for (int i = 0; i < num_seeds; i++) {
        seeds.push_back(RandomEngine::get().get_vec(0, width, 0, height));
    }
    // std::vector<vec2> seeds = {{vec2{8, 8}, vec2{2, 2}}};

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
        if (not_expanded) {
            break;
        }
    }

    removeSmallRooms(grid, width, height);
    addDoorsBetweenRooms(grid, width, height);
    unifyWalls(grid, width, height);
    addBoundaryWall(grid, width, height);

    char label = 'A';
    do {
        label = labelConnectedSections(grid, width, height);
        mergeConnectedSections(grid, width, height);
        removeLabelSections(grid, width, height);
    } while (label > 'B');

    makeEntrance(grid, width, height);
    addOrigin(grid, width, height);
    print_g();

    return grid;
}
