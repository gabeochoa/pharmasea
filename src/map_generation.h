

#include <string>
#include <vector>

#include "entityhelper.h"

namespace generation {

const char WALL = '#';
const char WALL2 = 'w';
const char EMPTY = '.';
const char ORIGIN = '0';
const char CUSTOMER = 'c';
const char CUST_SPAWNER = 'C';
const char REGISTER = 'R';
const char TABLE = 't';

const char GRABBERu = '^';
const char GRABBERl = '<';
const char GRABBERr = '>';
const char GRABBERd = 'v';

const char MED_CAB = 'M';
const char FRUIT = 'F';
const char BLENDER = 'b';
const char SODA_MACHINE = 'S';

const char CUPBOARD = 'd';
const char LEMON = 'l';
const char SIMPLE_SYRUP = 'y';
const char SQUIRTER = 'q';
const char TRASH = 'T';
const char FILTERED_GRABBER = 'G';
const char PIPE = 'p';
const char MOP_HOLDER = 'm';
const char FAST_FORWARD = 'f';

const char SOPHIE = 's';

struct helper {
    std::vector<std::string> lines;

    // For testing only
    vec2 x = {0, 0};
    vec2 z = {0, 0};

    helper(const std::vector<std::string>& l) : lines(l) {}

    template<typename Func = std::function<Entity&()>>
    void generate(Func&& add_to_map = nullptr) {
        vec2 origin = find_origin();

        const auto default_create = []() -> Entity& {
            return EntityHelper::createEntity();
        };

        for (int i = 0; i < (int) lines.size(); i++) {
            auto line = lines[i];
            for (int j = 0; j < (int) line.size(); j++) {
                vec2 raw_location = vec2{i * TILESIZE, j * TILESIZE};
                vec2 location = raw_location - origin;
                auto ch = get_char(i, j);
                if (add_to_map) {
                    generate_entity_from_character(add_to_map, ch, location);
                } else {
                    generate_entity_from_character(default_create, ch,
                                                   location);
                }
            }
        }
    }

    template<typename Func = std::function<Entity&()>>
    void generate_entity_from_character(Func&& create, char ch, vec2 location) {
        switch (ch) {
            case 'x':
                x = location;
                make_entity(create(), DebugOptions{.name = "x"},
                            vec::to3(location));
                return;
            case 'z':
                z = location;
                make_entity(create(), DebugOptions{.name = "z"},
                            vec::to3(location));
                return;
            case EMPTY:
                return;
            case SOPHIE: {
                (furniture::make_sophie(create(), vec::to3(location)));
                return;
            } break;
            case REGISTER: {
                (furniture::make_register(create(), location));
                return;
            } break;
            case WALL2:
            case WALL: {
                const auto d_color = Color{155, 75, 0, 255};
                (furniture::make_wall(create(), location, d_color));
                return;
            } break;
            case CUSTOMER: {
                make_customer(create(), location, true);
                return;
            } break;
            case CUST_SPAWNER: {
                furniture::make_customer_spawner(create(), vec::to3(location));
                return;
            } break;
            case GRABBERu: {
                (furniture::make_grabber(create(), location));
                return;
            } break;
            case GRABBERl: {
                Entity& grabber = create();
                (furniture::make_grabber(grabber, location));
                grabber.get<Transform>().rotate_facing_clockwise(270);
                return;
            } break;
            case GRABBERr: {
                Entity& grabber = create();
                (furniture::make_grabber(grabber, location));
                grabber.get<Transform>().rotate_facing_clockwise(90);
                return;
            } break;
            case GRABBERd: {
                Entity& grabber = create();
                (furniture::make_grabber(grabber, location));
                grabber.get<Transform>().rotate_facing_clockwise(180);
                return;
            } break;
            case TABLE: {
                (furniture::make_table(create(), location));
                return;
            } break;
            case MED_CAB: {
                (furniture::make_medicine_cabinet(create(), location));
                return;
            } break;
            case FRUIT: {
                (furniture::make_fruit_basket(create(), location));
                return;
            } break;
            case BLENDER: {
                (furniture::make_blender(create(), location));
                return;
            } break;
            case SODA_MACHINE: {
                (furniture::make_soda_machine(create(), location));
                return;
            } break;
            case CUPBOARD: {
                (furniture::make_cupboard(create(), location));
                return;
            } break;
            case LEMON: {
                (items::make_lemon(create(), location, 0));
                return;
            } break;
            case SIMPLE_SYRUP: {
                (items::make_simple_syrup(create(), location));
                return;
            } break;
            case SQUIRTER: {
                (furniture::make_squirter(create(), location));
                return;
            } break;
            case TRASH: {
                (furniture::make_trash(create(), location));
                return;
            } break;
            case FILTERED_GRABBER: {
                (furniture::make_filtered_grabber(create(), location));
                return;
            } break;
            case PIPE: {
                (furniture::make_pnumatic_pipe(create(), location));
                return;
            } break;
            case MOP_HOLDER: {
                (furniture::make_mop_holder(create(), location));
                return;
            } break;
            case FAST_FORWARD: {
                (furniture::make_fast_forward(create(), location));
                return;
            } break;
            case 32: {
                // space
            } break;
            default: {
                log_warn("Found character we dont parse in string '{}'({})", ch,
                         (int) ch);
            } break;
        };
        return;
    }

    inline vec2 find_origin() {
        vec2 origin{0.0, 0.0};
        for (int i = 0; i < (int) lines.size(); i++) {
            auto line = lines[i];
            for (int j = 0; j < (int) line.size(); j++) {
                auto ch = line[j];
                if (ch == '0') {
                    origin = vec2{j * TILESIZE, i * TILESIZE};
                    break;
                }
            }
        }
        return origin;
    }

    inline char get_char(int i, int j) {
        if (i < 0) return '.';
        if (j < 0) return '.';
        if (i >= (int) lines.size()) return '.';
        if (j >= (int) lines[i].size()) return '.';
        return lines[i][j];
    }

    void validate() {
        auto soph = EntityHelper::getFirstMatching([](const Entity& e) {
            return check_name(e, strings::entity::SOPHIE);
        });
        VALIDATE(soph, "sophie needs to be there ");

        // find register,
        auto reg_opt = EntityHelper::getFirstMatching([](const Entity& e) {
            return check_name(e, strings::entity::REGISTER);
        });
        VALIDATE(valid(reg_opt), "map needs to have at least one register");
        const auto& reg = asE(reg_opt);

        // find customer
        auto customer_opt =
            EntityHelper::getFirstMatching([](const Entity& e) -> bool {
                return check_name(e, strings::entity::CUSTOMER_SPAWNER);
            });
        VALIDATE(valid(customer_opt),
                 "map needs to have at least one customer spawn point");
        const auto& customer = asE(customer_opt);

        // ensure customers can make it to the register

        // TODO need a better way to do this
        // 0 makes sense but is the position of the entity, when its infront?
        auto reg_pos = reg.get<Transform>().tile_infront(1);

        log_info(" reg{} rep{} c{}", reg.get<Transform>().as2(), reg_pos,
                 customer.get<Transform>().as2());

        auto new_path = astar::find_path(
            customer.get<Transform>().as2(), reg_pos,
            std::bind(EntityHelper::isWalkable, std::placeholders::_1));
        VALIDATE(new_path.size(),
                 "customer should be able to generate a path to the register");

        // TODO add fastfowarder
    }
};

}  // namespace generation
