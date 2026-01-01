#include "afterhours_collections.h"

#include <memory>

namespace pharmasea_afterhours {
namespace {

afterhours::EntityCollection& collection_server() {
    static afterhours::EntityCollection c{};
    return c;
}

afterhours::EntityCollection& collection_client() {
    static afterhours::EntityCollection c{};
    return c;
}

void bind_collection_for_this_thread(afterhours::EntityCollection& c) {
    // Afterhours stores the active collection in a thread_local pointer.
    if (afterhours::g_collection == &c) return;

    // Keep the scope alive for the lifetime of this thread (or until replaced).
    thread_local std::unique_ptr<afterhours::ScopedEntityCollection> scope;
    scope = std::make_unique<afterhours::ScopedEntityCollection>(c);
}

}  // namespace

afterhours::EntityCollection& server_collection() { return collection_server(); }
afterhours::EntityCollection& client_collection() { return collection_client(); }

void bind_server_collection_for_this_thread() {
    bind_collection_for_this_thread(server_collection());
}

void bind_client_collection_for_this_thread() {
    bind_collection_for_this_thread(client_collection());
}

}  // namespace pharmasea_afterhours

