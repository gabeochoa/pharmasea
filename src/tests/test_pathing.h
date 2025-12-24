
#include "../engine/pathfinder.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../level_info.h"
#include "../map_generation/map_generation.h"

namespace test {
namespace neighbors {

inline bool is_valid(std::vector<vec2> v, vec2 p) {
    for (auto pos : v)
        if (p == pos) return false;
    return true;
};

inline void test_all_neighbors() {
    std::vector<vec2> invalid;

    auto n = pathfinder::get_neighbors({1, 1}, [](auto&&) { return true; });
    VALIDATE(n.size() == 8, "should have all neighbors");

    invalid.push_back({0, 0});
    auto f = std::bind(&is_valid, invalid, std::placeholders::_1);
    n = pathfinder::get_neighbors({1, 1}, f);
    VALIDATE(n.size() == 7, "should have 7 neighbors");

    invalid.push_back({0, 1});
    f = std::bind(&is_valid, invalid, std::placeholders::_1);
    n = pathfinder::get_neighbors({1, 1}, f);
    VALIDATE(n.size() == 6, "should have 6 neighbors");

    invalid.push_back({0, 2});
    f = std::bind(&is_valid, invalid, std::placeholders::_1);
    n = pathfinder::get_neighbors({1, 1}, f);
    VALIDATE(n.size() == 5, "should have 5 neighbors");

    invalid.push_back({1, 0});
    f = std::bind(&is_valid, invalid, std::placeholders::_1);
    n = pathfinder::get_neighbors({1, 1}, f);
    VALIDATE(n.size() == 4, "should have 4 neighbors");

    invalid.push_back({1, 2});
    f = std::bind(&is_valid, invalid, std::placeholders::_1);
    n = pathfinder::get_neighbors({1, 1}, f);
    VALIDATE(n.size() == 3, "should have 3 neighbors");

    invalid.push_back({2, 0});
    f = std::bind(&is_valid, invalid, std::placeholders::_1);
    n = pathfinder::get_neighbors({1, 1}, f);
    VALIDATE(n.size() == 2, "should have 2 neighbors");

    invalid.push_back({2, 1});
    f = std::bind(&is_valid, invalid, std::placeholders::_1);
    n = pathfinder::get_neighbors({1, 1}, f);
    VALIDATE(n.size() == 1, "should have 1 neighbors");

    invalid.push_back({2, 2});
    f = std::bind(&is_valid, invalid, std::placeholders::_1);
    n = pathfinder::get_neighbors({1, 1}, f);
    VALIDATE(n.size() == 0, "should have 0 neighbors");
}
}  // namespace neighbors

static std::vector<Entity> ents;

inline bool canvisit(const vec2& pos) {
    auto is_collidable_ = [](const Entity& entity) {
        return (entity.has<IsSolid>());
    };
    bool hit_impassible_entity = false;
    for (const auto& entity : ents) {
        if (!is_collidable_(entity)) continue;

        if (vec::distance(entity.template get<Transform>().as2(), pos) <
            TILESIZE / 2.f) {
            hit_impassible_entity = true;
            break;
        }
    }
    return !hit_impassible_entity;
}

inline auto p(vec2 a, vec2 b) {
    return pathfinder::find_path(a, b,
                                 std::bind(&canvisit, std::placeholders::_1));
}

inline auto p(const Entity& a, vec2 b) {
    return p(a.get<Transform>().as2(), b);
}

inline std::pair<vec2, vec2> setup(const std::string& map) {
    auto lines = util::split_string(map, "\n");
    generation::helper helper(lines);
    helper.generate([]() -> Entity& { return ents.emplace_back(); });

    return std::make_pair(helper.z, helper.x);
}

inline void teardown() {
    for (auto& entity : ents) {
        for (size_t i = 0; i < afterhours::max_num_components; ++i) {
            entity.componentArray[i].reset();
        }
        entity.componentSet.reset();
    }

    ents.clear();
}

inline void test_no_obstacles() {
    auto [z, x] = setup("z............x");
    //
    auto path = p(z, x);
    VALIDATE(path.size(), "path should not be empty");
    //
    teardown();
}

inline void test_one_obstacle() {
    auto [z, x] = setup("z..w..x");
    //
    auto path = p(z, x);
    VALIDATE(path.size(), "path should not be empty");
    //
    teardown();
}

inline void test_surround_no_path() {
    auto [z, x] = setup(R"(
www
wzw....x.
www
    )");
    //
    auto path = p(z, x);
    VALIDATE(path.empty(), "path should be empty");
    //
    teardown();
}

inline void test_surround_one_exit() {
    auto [z, x] = setup(R"(
www
wz.....x.
www
    )");
    //
    auto path = p(z, x);
    VALIDATE(path.size(), "path should not be empty");
    //
    teardown();
}

inline void test_clear_path_surround_exit() {
    auto [z, x] = setup(R"(
www
wxw....z.
www
    )");
    //
    auto path = p(z, x);
    VALIDATE(path.empty(), "path should be empty");
    //
    teardown();
}

inline void test_clear_path_surround_one_exit() {
    auto [z, x] = setup(R"(
www
wx.....z.
www
    )");
    //
    auto path = p(z, x);
    VALIDATE(path.size(), "path should not be empty");
    //
    teardown();
}

inline void test_maze_path_exists() {
    auto [z, x] = setup(R"(
wwwwwwwwwwwwwww
w...wwwwwwwwwww
w.x.wwwwwwwwwww
w.............w
wwwwwwwwwwwww.w
w...w...w...w.w
w.w.w.w.w.w.w.w
wzw...w...w...w
wwwwwwwwwwwwwww
)");
    //
    auto path = p(z, x);
    VALIDATE(path.size(), "path should not be empty");
    //
    teardown();
}

inline void test_maze_path_doesnt_exist() {
    auto [z, x] = setup(R"(
wwwwwwwwwwwwwww
w...w...w...wxw
w.w.w.w.w.w.w.w
w.w.w.w.w.w.w.w
w.w.w.w.w.w.w.w
wzw...w.w.w...w 
wwwwwwwwwwwwwww
    )");
    //  ^ the filled in square is that one right there
    //
    auto path = p(z, x);
    VALIDATE(path.empty(), "path should be empty");
    //
    teardown();
}

}  // namespace test
   //
inline void test_all_pathing() {
    using namespace test;
    neighbors::test_all_neighbors();
    test_no_obstacles();
    test_one_obstacle();
    test_surround_no_path();
    test_surround_one_exit();
    test_clear_path_surround_exit();
    test_clear_path_surround_one_exit();
    test_maze_path_exists();
    test_maze_path_doesnt_exist();

    test::ents.clear();
}
