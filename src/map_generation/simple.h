#pragma once

#include <algorithm>
#include <cstddef>
#include <queue>
#include <vector>

#include "../engine/random_engine.h"
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
    // Note: entrance is now created by makeEntrance() on the east side
}

// Ensure there's always a 1-tile walkable corridor inside the boundary wall.
// This prevents interior walls from being placed directly against the outer
// wall.
void ensureInnerCorridor(std::vector<char>& grid, int width, int height) {
    // Clear row 1 (just inside top wall) - except corners
    for (int j = 1; j < width - 1; j++) {
        if (grid[1 * width + j] == generation::WALL) {
            grid[1 * width + j] = generation::EMPTY;
        }
    }
    // Clear row height-2 (just inside bottom wall) - except corners
    for (int j = 1; j < width - 1; j++) {
        if (grid[(height - 2) * width + j] == generation::WALL) {
            grid[(height - 2) * width + j] = generation::EMPTY;
        }
    }
    // Clear column 1 (just inside left wall) - except corners
    for (int i = 1; i < height - 1; i++) {
        if (grid[i * width + 1] == generation::WALL) {
            grid[i * width + 1] = generation::EMPTY;
        }
    }
    // Clear column width-2 (just inside right wall) - except corners
    for (int i = 1; i < height - 1; i++) {
        if (grid[i * width + (width - 2)] == generation::WALL) {
            grid[i * width + (width - 2)] = generation::EMPTY;
        }
    }
}

// Remove interior walls that touch the boundary and create awkward pockets.
// Valid patterns: room completely interior, or room uses boundary as one wall.
// Invalid: walls that jut out from boundary creating small notches.
void removeAwkwardBoundaryWalls(std::vector<char>& grid, int width, int height) {
    // Check for interior walls adjacent to boundary that create small pockets
    // Remove wall segments that touch boundary but don't extend far enough
    const int min_wall_extension = 3;  // Walls must extend at least this far

    bool changed = true;
    while (changed) {
        changed = false;

        // Check walls touching top boundary (row 1)
        for (int j = 1; j < width - 1; j++) {
            if (grid[1 * width + j] != generation::WALL) continue;
            int extension = 0;
            for (int i = 1; i < height - 1; i++) {
                if (grid[i * width + j] == generation::WALL)
                    extension++;
                else
                    break;
            }
            if (extension > 0 && extension < min_wall_extension) {
                for (int i = 1; i < 1 + extension && i < height - 1; i++) {
                    grid[i * width + j] = generation::EMPTY;
                    changed = true;
                }
            }
        }

        // Check walls touching bottom boundary (row height-2)
        for (int j = 1; j < width - 1; j++) {
            if (grid[(height - 2) * width + j] != generation::WALL) continue;
            int extension = 0;
            for (int i = height - 2; i > 0; i--) {
                if (grid[i * width + j] == generation::WALL)
                    extension++;
                else
                    break;
            }
            if (extension > 0 && extension < min_wall_extension) {
                for (int i = height - 2; i > height - 2 - extension && i > 0;
                     i--) {
                    grid[i * width + j] = generation::EMPTY;
                    changed = true;
                }
            }
        }

        // Check walls touching left boundary (col 1)
        for (int i = 1; i < height - 1; i++) {
            if (grid[i * width + 1] != generation::WALL) continue;
            int extension = 0;
            for (int j = 1; j < width - 1; j++) {
                if (grid[i * width + j] == generation::WALL)
                    extension++;
                else
                    break;
            }
            if (extension > 0 && extension < min_wall_extension) {
                for (int j = 1; j < 1 + extension && j < width - 1; j++) {
                    grid[i * width + j] = generation::EMPTY;
                    changed = true;
                }
            }
        }

        // Check walls touching right boundary (col width-2)
        for (int i = 1; i < height - 1; i++) {
            if (grid[i * width + (width - 2)] != generation::WALL) continue;
            int extension = 0;
            for (int j = width - 2; j > 0; j--) {
                if (grid[i * width + j] == generation::WALL)
                    extension++;
                else
                    break;
            }
            if (extension > 0 && extension < min_wall_extension) {
                for (int j = width - 2; j > width - 2 - extension && j > 0;
                     j--) {
                    grid[i * width + j] = generation::EMPTY;
                    changed = true;
                }
            }
        }
    }
}

// Remove small walkable regions that are too small to be useful rooms.
// A hallway connecting larger areas is fine, but isolated small rooms are not.
// Uses flood fill to find connected walkable regions and fills small ones with
// walls.
void removeSmallRoomRegions(std::vector<char>& grid, int width, int height) {
    // Track which cells we've visited
    std::vector<bool> visited(grid.size(), false);

    // Minimum walkable region size to keep (smaller regions become walls)
    const int min_room_size = 15;

    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            int idx = i * width + j;
            if (grid[idx] != generation::EMPTY || visited[idx]) continue;

            // Flood fill to find this walkable region
            std::vector<int> region;
            std::queue<int> q;
            q.push(idx);
            visited[idx] = true;

            while (!q.empty()) {
                int cur = q.front();
                q.pop();
                region.push_back(cur);

                int ci = cur / width;
                int cj = cur % width;

                // Check 4 neighbors
                int di[] = {-1, 1, 0, 0};
                int dj[] = {0, 0, -1, 1};
                for (int k = 0; k < 4; k++) {
                    int ni = ci + di[k];
                    int nj = cj + dj[k];
                    if (ni < 0 || ni >= height || nj < 0 || nj >= width) continue;
                    int nidx = ni * width + nj;
                    if (visited[nidx]) continue;
                    if (grid[nidx] != generation::EMPTY) continue;
                    visited[nidx] = true;
                    q.push(nidx);
                }
            }

            // Fill this region with walls if it's too small
            if ((int) region.size() < min_room_size) {
                for (int idx2 : region) {
                    grid[idx2] = generation::WALL;
                }
            }
        }
    }
}

// Remove walls that create 1-wide corridors. Hallways should be at least 2 wide.
// Detect pattern: wall-empty-wall (horizontal or vertical) and remove the wall.
void widenNarrowCorridors(std::vector<char>& grid, int width, int height) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 1; i < height - 1; i++) {
            for (int j = 1; j < width - 1; j++) {
                if (grid[i * width + j] != generation::EMPTY) continue;

                // Check for vertical 1-wide corridor: wall-empty-wall horizontally
                bool wallLeft = (grid[i * width + (j - 1)] == generation::WALL);
                bool wallRight = (grid[i * width + (j + 1)] == generation::WALL);

                if (wallLeft && wallRight) {
                    // This empty tile is in a 1-wide vertical corridor
                    // Remove one of the walls (prefer interior wall over boundary)
                    if (j - 1 > 0) {
                        grid[i * width + (j - 1)] = generation::EMPTY;
                        changed = true;
                    } else if (j + 1 < width - 1) {
                        grid[i * width + (j + 1)] = generation::EMPTY;
                        changed = true;
                    }
                    continue;
                }

                // Check for horizontal 1-wide corridor: wall-empty-wall vertically
                bool wallUp = (grid[(i - 1) * width + j] == generation::WALL);
                bool wallDown = (grid[(i + 1) * width + j] == generation::WALL);

                if (wallUp && wallDown) {
                    // This empty tile is in a 1-wide horizontal corridor
                    if (i - 1 > 0) {
                        grid[(i - 1) * width + j] = generation::EMPTY;
                        changed = true;
                    } else if (i + 1 < height - 1) {
                        grid[(i + 1) * width + j] = generation::EMPTY;
                        changed = true;
                    }
                    continue;
                }
            }
        }
    }
}

// Thin out thick wall sections - walls should be 1 tile thick, not solid blobs.
void thinWallBlobs(std::vector<char>& grid, int width, int height) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 1; i < height - 1; i++) {
            for (int j = 1; j < width - 1; j++) {
                if (grid[i * width + j] != generation::WALL) continue;

                bool wallUp =
                    (i > 0 && grid[(i - 1) * width + j] == generation::WALL);
                bool wallDown =
                    (i < height - 1 &&
                     grid[(i + 1) * width + j] == generation::WALL);
                bool wallLeft =
                    (j > 0 && grid[i * width + (j - 1)] == generation::WALL);
                bool wallRight =
                    (j < width - 1 &&
                     grid[i * width + (j + 1)] == generation::WALL);

                int wallNeighbors = (wallUp ? 1 : 0) + (wallDown ? 1 : 0) +
                                    (wallLeft ? 1 : 0) + (wallRight ? 1 : 0);

                // Remove if surrounded by 3+ walls (inside a blob)
                if (wallNeighbors >= 3) {
                    grid[i * width + j] = generation::EMPTY;
                    changed = true;
                    continue;
                }

                // Check for 2x2 wall blocks and remove
                if (wallUp && wallLeft &&
                    grid[(i - 1) * width + (j - 1)] == generation::WALL) {
                    grid[i * width + j] = generation::EMPTY;
                    changed = true;
                    continue;
                }
                if (wallUp && wallRight &&
                    grid[(i - 1) * width + (j + 1)] == generation::WALL) {
                    grid[i * width + j] = generation::EMPTY;
                    changed = true;
                    continue;
                }
                if (wallDown && wallLeft &&
                    grid[(i + 1) * width + (j - 1)] == generation::WALL) {
                    grid[i * width + j] = generation::EMPTY;
                    changed = true;
                    continue;
                }
                if (wallDown && wallRight &&
                    grid[(i + 1) * width + (j + 1)] == generation::WALL) {
                    grid[i * width + j] = generation::EMPTY;
                    changed = true;
                    continue;
                }

            }
        }
    }
}

// Remove interior wall clusters that don't connect to the boundary.
// These are "floating" walls that create awkward blocks in the middle of rooms.
void removeFloatingWallClusters(std::vector<char>& grid, int width, int height) {
    std::vector<bool> visited(grid.size(), false);

    // First, mark all boundary walls as visited (they're connected by definition)
    for (int j = 0; j < width; j++) {
        if (grid[0 * width + j] == generation::WALL)
            visited[0 * width + j] = true;
        if (grid[(height - 1) * width + j] == generation::WALL)
            visited[(height - 1) * width + j] = true;
    }
    for (int i = 0; i < height; i++) {
        if (grid[i * width + 0] == generation::WALL) visited[i * width + 0] = true;
        if (grid[i * width + (width - 1)] == generation::WALL)
            visited[i * width + (width - 1)] = true;
    }

    // Now flood fill from boundary walls to find all connected walls
    std::queue<int> boundaryQ;
    for (int j = 0; j < width; j++) {
        if (grid[0 * width + j] == generation::WALL) boundaryQ.push(0 * width + j);
        if (grid[(height - 1) * width + j] == generation::WALL)
            boundaryQ.push((height - 1) * width + j);
    }
    for (int i = 1; i < height - 1; i++) {
        if (grid[i * width + 0] == generation::WALL) boundaryQ.push(i * width + 0);
        if (grid[i * width + (width - 1)] == generation::WALL)
            boundaryQ.push(i * width + (width - 1));
    }

    while (!boundaryQ.empty()) {
        int cur = boundaryQ.front();
        boundaryQ.pop();

        int ci = cur / width;
        int cj = cur % width;

        // Check 4 neighbors for more walls
        int di[] = {-1, 1, 0, 0};
        int dj[] = {0, 0, -1, 1};
        for (int k = 0; k < 4; k++) {
            int ni = ci + di[k];
            int nj = cj + dj[k];
            if (ni < 0 || ni >= height || nj < 0 || nj >= width) continue;
            int nidx = ni * width + nj;
            if (visited[nidx]) continue;
            if (grid[nidx] != generation::WALL) continue;
            visited[nidx] = true;
            boundaryQ.push(nidx);
        }
    }

    // Any unvisited wall is floating - remove it
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            int idx = i * width + j;
            if (grid[idx] == generation::WALL && !visited[idx]) {
                grid[idx] = generation::EMPTY;
            }
        }
    }
}

// Helper to count how many walkable (EMPTY) neighbors a cell has (4-connected)
int countWalkableNeighbors(const std::vector<char>& grid, int width, int height,
                           int i, int j) {
    int count = 0;
    // Up
    if (i > 0 && grid[(i - 1) * width + j] == generation::EMPTY) count++;
    // Down
    if (i < height - 1 && grid[(i + 1) * width + j] == generation::EMPTY)
        count++;
    // Left
    if (j > 0 && grid[i * width + (j - 1)] == generation::EMPTY) count++;
    // Right
    if (j < width - 1 && grid[i * width + (j + 1)] == generation::EMPTY)
        count++;
    return count;
}

// Remove 1x1 enclosed walkable areas by opening a wall to merge them with
// adjacent spaces. A 1x1 room is a walkable cell with 0 walkable neighbors.
void remove1x1Rooms(std::vector<char>& grid, int width, int height) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 1; i < height - 1; i++) {
            for (int j = 1; j < width - 1; j++) {
                if (grid[i * width + j] != generation::EMPTY) continue;

                int neighbors = countWalkableNeighbors(grid, width, height, i, j);
                if (neighbors == 0) {
                    // This is a 1x1 room - try to open a wall to connect it
                    // Prefer opening toward the interior (away from boundary)
                    // Try down first (toward interior usually)
                    if (i + 1 < height - 1 &&
                        grid[(i + 1) * width + j] == generation::WALL) {
                        grid[(i + 1) * width + j] = generation::EMPTY;
                        changed = true;
                    }
                    // Try right
                    else if (j + 1 < width - 1 &&
                             grid[i * width + (j + 1)] == generation::WALL) {
                        grid[i * width + (j + 1)] = generation::EMPTY;
                        changed = true;
                    }
                    // Try up
                    else if (i > 1 &&
                             grid[(i - 1) * width + j] == generation::WALL) {
                        grid[(i - 1) * width + j] = generation::EMPTY;
                        changed = true;
                    }
                    // Try left
                    else if (j > 1 &&
                             grid[i * width + (j - 1)] == generation::WALL) {
                        grid[i * width + (j - 1)] = generation::EMPTY;
                        changed = true;
                    }
                }
            }
        }
    }
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
        if (i >= height) return 0;
        if (j < 0) return 0;
        if (j >= width) return 0;
        return grid[i * width + j];
    };

    // Place entrance on the south (bottom) edge of the ASCII map.
    // Due to coordinate mapping (row->X, col->Y), this appears on the
    // visual "east/right" side in the 3D view.
    // Find a wall on the bottom edge where the cell above is walkable.
    int i = height - 1;  // Bottom edge
    for (int j = 1; j < width - 1; j++) {
        if (px(i, j) == generation::WALL && px(i - 1, j) == generation::EMPTY) {
                grid[i * width + j] = generation::EMPTY;
            return;
        }
    }
}

inline void addOrigin(std::vector<char>& grid, int width, int height) {
    int midx = (int) (width / 2);
    int midy = (int) (height / 2);
    grid[midx * width + midy] = generation::ORIGIN;
}

// Generate a map by directly placing walls extending from boundaries
inline std::vector<char> something(int width, int height, int num_walls,
                                   [[maybe_unused]] bool should_print) {
    std::vector<char> grid;
    grid.reserve(width * height);
    for (int i = 0; i < width * height; i++) {
        grid.push_back(generation::EMPTY);
    }

    // Add boundary walls first
    addBoundaryWall(grid, width, height);

    // Add random walls extending from boundaries
    for (int w = 0; w < num_walls; w++) {
        // Pick a random edge to extend from (0=top, 1=bottom, 2=left, 3=right)
        int edge = RandomEngine::get().get_int(0, 3);
        // Random length (3 to half the dimension)
        int minLen = 3;
        int maxLen = (edge < 2 ? height : width) / 2;
        int length = RandomEngine::get().get_int(minLen, maxLen);

        if (edge == 0) {
            // Extend down from top
            int col = RandomEngine::get().get_int(2, width - 3);
            for (int i = 1; i < 1 + length && i < height - 1; i++) {
                grid[i * width + col] = generation::WALL;
            }
        } else if (edge == 1) {
            // Extend up from bottom
            int col = RandomEngine::get().get_int(2, width - 3);
            for (int i = height - 2; i > height - 2 - length && i > 0; i--) {
                grid[i * width + col] = generation::WALL;
            }
        } else if (edge == 2) {
            // Extend right from left
            int row = RandomEngine::get().get_int(2, height - 3);
            for (int j = 1; j < 1 + length && j < width - 1; j++) {
                grid[row * width + j] = generation::WALL;
            }
        } else {
            // Extend left from right
            int row = RandomEngine::get().get_int(2, height - 3);
            for (int j = width - 2; j > width - 2 - length && j > 0; j--) {
                grid[row * width + j] = generation::WALL;
            }
        }
    }

    // Thin out any thick wall sections
    thinWallBlobs(grid, width, height);

    // Widen 1-wide corridors to at least 2 wide
    widenNarrowCorridors(grid, width, height);

    // Remove floating walls
    removeFloatingWallClusters(grid, width, height);

    // Fill small isolated regions
    removeSmallRoomRegions(grid, width, height);

    // Remove any 1x1 enclosed areas
    remove1x1Rooms(grid, width, height);

    // Ensure all areas are connected
    char label = 'A';
    do {
        label = labelConnectedSections(grid, width, height);
        mergeConnectedSections(grid, width, height);
        removeLabelSections(grid, width, height);
    } while (label > 'B');

    makeEntrance(grid, width, height);
    addOrigin(grid, width, height);

    return grid;
}

inline std::vector<char> something(int width, int height) {
    // Number of walls to generate. For 20x20, this gives ~4-6 walls.
    int default_num_walls = (int) ((width + height) / 8);
    return something(width, height, default_num_walls, false);
}
