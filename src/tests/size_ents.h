

#include "../entity.h"
#include "../external_include.h"
#include "../network/shared.h"

/*

namespace tests {

struct SizeInfo {
    std::string name;
    int size;

    bool operator<(const SizeInfo& si) const { return size < si.size; }
};

template<typename A>
inline void size_component_a(std::vector<SizeInfo>& results) {
    Entity* entity = new Entity();
    network::Buffer buff = network::serialize_to_entity(entity);
    results.push_back({std::string(type_name<A>()), (int) buff.size()});
    delete entity;
}

template<typename A>
inline void size_components(auto& results) {
    size_component_a<A>(results);
}

template<typename A, typename B, typename... Rest>
inline std::vector<SizeInfo> size_components(auto& results) {
    size_component_a<A>(results);
    size_components<B, Rest...>(results);
    return results;
}

inline std::vector<SizeInfo> size_all_components_sorted() {
    using namespace entities;

    std::vector<SizeInfo> results;
    size_components<
        Transform, HasName, CanHoldItem, SimpleColoredBoxRenderer,
        CanBeHighlighted, CanHighlightOthers, CanHoldFurniture,
        CanBeGhostPlayer, CanPerformJob, ModelRenderer, CanBePushed,
        CanHaveAilment, CustomHeldItemPosition, HasWork, HasBaseSpeed, IsSolid,
        CanBeHeld, IsRotatable, CanGrabFromOtherFurniture, ConveysHeldItem,
        HasWaitingQueue, CanBeTakenFrom, IsItemContainer<Bag>,
        IsItemContainer<PillBottle>, IsItemContainer<Pill>, UsesCharacterModel,
        ShowsProgressBar, DebugName, HasDynamicModelName, IsTriggerArea,
        HasSpeechBubble, Indexer, IsSpawner, HasTimer, CollectsUserInput>(
        results);

    std::sort(results.begin(), results.end());

    return results;
}

inline std::vector<SizeInfo> size_all_sorted() {
    using namespace entities;
    vec3 zero = {0, 0, 0};
    vec2 z2 = {0, 0};

    std::vector<SizeInfo> results;

    Entity* e = nullptr;

    // e = make_remote_player(zero);
    // results.push_back({e->get<DebugName>().name(), 0});
    // delete e;
    //
    // e = make_player(zero);
    // results.push_back({e->get<DebugName>().name(), 0});
    // delete e;
    //
    // std::bind(&make_customer, z2, true),

    e = make_table(z2);
    network::Buffer buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    e = make_character_switcher(z2);
    buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    e = make_wall(z2, ui::color::brown);
    buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    e = make_conveyer(z2);
    buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    e = make_grabber(z2);
    buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    e = make_register(z2);
    buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    e = make_bagbox(z2);
    buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    e = make_medicine_cabinet(z2);
    buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    e = make_pill_dispenser(z2);
    buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    e = make_trigger_area(zero, 0, 0, "test");
    buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    e = make_customer_spawner(zero);
    buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    e = make_sophie(zero);
    buff = network::serialize_to_entity(e);
    results.push_back({e->get<DebugName>().name(), (int) buff.size()});
    delete e;

    std::sort(results.begin(), results.end());

    return results;
}

inline void size_test() {
    // TODO turn back on at some point
    return;
    auto r = size_all_components_sorted();
    for (auto si : r) {
        std::cout << si.name << " " << si.size << std::endl;
    }

    r = size_all_sorted();
    for (auto si : r) {
        std::cout << si.name << " " << si.size << std::endl;
    }
}

*/
inline void size_test() {}
