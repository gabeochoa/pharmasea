

#include "../external_include.h"
#include "../entity.h"
#include "../network/shared.h"

namespace tests {

    struct SizeInfo {
        std::string name; 
        int size;
    };

    inline std::vector<SizeInfo> size_all_sorted(){
        using namespace entities;
        vec3 zero = {0,0,0};
        vec2 z2 = {0,0};

        std::vector<SizeInfo> results;

    Entity* entity2 = make_entity({.name="entity"}, zero);
    network::serialize_to_entity(entity2);

        std::function<Entity*(void)> fns[] = {
            // std::bind(&make_entity, {.name="entity"}, zero),
            // std::bind(&make_aiperson, {.name="aiperson"}, zero),
            // std::bind(&make_furniture, {.name="furniture"}, z2),

            // [&](){return make_remote_player(zero);},
            // [&](){return make_player(zero);},

            // std::bind(&make_customer, z2, true),
            std::bind(&make_table, z2),
            // std::bind(&make_character_switcher, z2),
            // std::bind(&make_wall, z2, ui::color::brown),
            // std::bind(&make_conveyer, z2),
            // std::bind(&make_grabber, z2),
            // std::bind(&make_register, z2),
            // std::bind(&make_bagbox, z2),
            // std::bind(&make_medicine_cabinet, z2),
            // std::bind(&make_pill_dispenser, z2),
            // std::bind(&make_trigger_area, zero, 0, 0, "test"),
            // std::bind(&make_customer_spawner, zero),
            // std::bind(&make_sophie, zero),
        };

        for(int i = 0; i < 15; i++){
            auto fn = fns[i];
            Entity* e = fn();
            std::cout << "Attempting to serialize: " << i << " " << e->get<DebugName>().name() << std::endl;
            network::Buffer buff = network::serialize_to_entity(e);
            results.push_back({e->get<DebugName>().name(), (int) buff.size() });
            // delete e;
        }

        return results;
    }

    inline void size_test(){
        // auto r = size_all_sorted();
        // for(auto si : r){
            // std::cout << si.name << " " << si.size << std::endl;
        // }

    }

}

