// Thread role markers used to avoid fragile cross-thread ID plumbing.
//
// We use this to decide whether the current thread is the "server thread"
// (authoritative simulation) vs the main/client thread. This prevents relying
// on `GLOBALS` storing pointers to thread IDs (which was both racy and could
// dangle on shutdown).
//
// NOTE: This is intentionally simple: host mode has a dedicated server thread,
// and the rest of the app runs as client/main thread.
//
// TODO(threading): Consider expanding to explicit roles if we add more worker
// threads that need distinct ECS contexts.

#pragma once

namespace thread_role {

enum class Role {
    ClientMain,
    Server,
};

inline thread_local Role current = Role::ClientMain;

inline void set(Role r) { current = r; }
inline bool is_server() { return current == Role::Server; }

}  // namespace thread_role
