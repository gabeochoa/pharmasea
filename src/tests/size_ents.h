

#include <stdio.h>

#include <iostream>
#include <string>

#include "../entity.h"
#include "../external_include.h"
#include "../network/shared.h"

namespace tests {

struct SizeInfo {
    std::string name;
    int size;

    bool operator<(const SizeInfo& si) const { return size < si.size; }
};

inline int compute_size(Entity* entity) {
    network::Buffer buff = network::serialize_to_entity(entity);
    return (int) buff.size();
}

template<typename A>
inline void size_component_a(std::vector<SizeInfo>& results) {
    Entity* entity = new Entity();
    entity->addComponent<A>();
    int buff_size = compute_size(entity);
    results.push_back({std::string(type_name<A>()), buff_size});
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
    std::vector<SizeInfo> results;
    size_components<  //
        Transform, HasName,
        // Is
        IsRotatable, IsItem, IsSpawner, IsTriggerArea, IsSolid, IsItemContainer,
        IsDrink, IsPnumaticPipe,
        //
        AddsIngredient, CanHoldItem, CanBeHighlighted, CanHighlightOthers,
        CanHoldFurniture, CanBeGhostPlayer, CanPerformJob, CanBePushed,
        CustomHeldItemPosition, CanBeHeld, CanGrabFromOtherFurniture,
        ConveysHeldItem, CanBeTakenFrom, UsesCharacterModel, Indexer,
        CanOrderDrink,
        //
        HasWaitingQueue, HasTimer, HasSubtype, HasSpeechBubble, HasWork,
        HasBaseSpeed, HasRopeToItem,
        // render
        ModelRenderer, HasDynamicModelName, SimpleColoredBoxRenderer
        //
        //
        >(results);

    {
        Entity* e = new Entity();
        e->addAll<  //
            Transform, HasName,
            // Is
            IsRotatable, IsItem, IsSpawner, IsTriggerArea, IsSolid,
            IsItemContainer, IsDrink, IsPnumaticPipe,
            //
            AddsIngredient, CanHoldItem, CanBeHighlighted, CanHighlightOthers,
            CanHoldFurniture, CanBeGhostPlayer, CanPerformJob, CanBePushed,
            CustomHeldItemPosition, CanBeHeld, CanGrabFromOtherFurniture,
            ConveysHeldItem, CanBeTakenFrom, UsesCharacterModel, Indexer,
            CanOrderDrink,
            //
            HasWaitingQueue, HasTimer, HasSubtype, HasSpeechBubble, HasWork,
            HasBaseSpeed, HasRopeToItem,
            // render
            ModelRenderer, HasDynamicModelName, SimpleColoredBoxRenderer
            //
            //
            >();
        int all_size = compute_size(e);
        results.push_back({"all components", all_size});
        delete e;
    }

    std::sort(results.begin(), results.end());

    return results;
}

inline void size_test() {
    // auto r = size_all_components_sorted();
    // for (auto si : r) {
    // std::cout << si.name << " " << si.size << std::endl;
    // }
    //
    // r = size_all_sorted();
    // for (auto si : r) {
    // std::cout << si.name << " " << si.size << std::endl;
    // }
}

}  // namespace tests
