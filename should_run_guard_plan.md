# should_run guard plan

## Current state
- 38 afterhours systems override `should_run`, and ~75 % of them boil down to the same “game-like + (day|night) + transition flag” checks spread across multiple files. This duplication makes it easy to forget edge cases (e.g., skipping transition work) and costs a `NamedEntity::Sophie` lookup per system.
- Systems can be grouped into a handful of modes: always-on (trigger areas, trash, render helpers), planning-only, in-round-only, transition-only, and bespoke multi-context cases (e.g., model-test support for container systems). The current layout doesn’t communicate those groupings anywhere central.
- Because the guard logic lives on individual systems we cannot toggle whole classes of systems (e.g., “pause all in-round updates”) or add instrumentation/metrics without touching every file.

## Goals
1. Share the common guard logic, ideally without changing every `for_each_with` implementation.
2. Capture the “run profile” for a system in one place so we can reason about coverage (and potentially enforce policies/tests).
3. Avoid repeated entity lookups/exception handling for Sophie/`HasDayNightTimer`.
4. Keep a clean escape hatch for bespoke logic (tutorial/Nux, debug-only systems, etc.).

## Building blocks
### Frame context snapshot
- Add `SystemRunContext` (singleton or thread_local) populated once per `SystemManager::update_all_entities` tick. Fields: current `game::State`, `is_game_like`, `is_daytime`, `is_nighttime`, `needs_transition_processing`, `debug_flags`, `is_server`, etc. This already exists piecemeal via `SystemManager::get()` helpers; this step just snapshots it.

### RunCondition DSL
- Define a `RunCondition` struct (or tagged enum) with knobs for game mode, day/night regime, transition policy, optional debug flag, and optional custom predicate.
- Provide helpers such as `RunProfiles::InRoundNight`, `RunProfiles::PlanningDay`, `RunProfiles::TransitionNight`, `RunProfiles::Always`, `RunProfiles::ModelTestOrNight`. Each profile is just a constexpr `RunCondition`.

### GuardedSystem wrapper
- Implement `template<RunCondition Guard, typename... Components> using GuardedSystem = afterhours::System<Components...>` that overrides `should_run` to call `SystemRunGuards::match(Guard, context)`. For multi-mode cases expose `GuardedSystem<any_of<GuardA, GuardB>>` or let systems override `should_run` but call into `SystemRunGuards::any()`.
- Special-case `ModelTest` and tutorial-only systems by composing guards (e.g., `any_of(RunProfiles::ModelTest, RunProfiles::InRoundNight)`).

### Escape hatches & instrumentation
- Keep the ability for systems to override `should_run` entirely when they need bespoke state (e.g., `ProcessNuxUpdatesSystem`).
- When `SystemRunGuards` rejects a system, optionally track stats (per-profile skip counts) so we can assert expectations in tests or show them in dev UI.

## Implementation phases
1. **Context plumbing**: add `SystemRunContext` that snapshots the information we already compute (day/night flags, transition flag, debug options). Write unit tests to ensure it mirrors `SystemManager::is_daytime()` etc.
2. **Guard helpers**: add `RunCondition`, `RunProfiles`, and `SystemRunGuards::match/any`. Unit-test the matching logic with synthetic contexts.
3. **Introduce `GuardedSystem`**: create aliases for `InRoundSystem`, `PlanningSystem`, `TransitionNightSystem`, `TransitionDaySystem`, etc. Convert 2–3 representative systems to validate ergonomics and prove we remain binary-compatible with afterhours.
4. **Batch migrations**: convert systems by category (e.g., all in-round conveyors/grabbers first, then planning/day, then transition). After each batch run smoke tests (night round, planning load, store transitions) to ensure guards still fire.
5. **Optional guard registry**: add a simple table (e.g., `std::vector<SystemRunProfileStats>`) to log which systems use which guards. This becomes documentation and ensures new systems pick an existing profile when possible.

## Options & trade-offs
- **Option A – Template-based guards (recommended):** Compile-time `GuardedSystem<Profile,...>` keeps `should_run` inline, no heap allocations, and makes the run profile explicit in the type. Costs: template noise but manageable.
- **Option B – Runtime delegates:** Keep deriving from `afterhours::System` but add a helper `bool ShouldRunNight(EntityRunMode mode)` returning a lambda. Lower code churn but still repeats the `should_run` override and keeps the coupling loose.
- **Option C – Status quo:** Do nothing and keep per-system guards. Zero upfront work but ongoing maintenance pain and no chance to instrument or toggle per-profile.

## Testing strategy
- Add focused unit tests for `SystemRunGuards` and `SystemRunContext` (mock day/night states, transition flags).
- Scenario tests already exist (day/night transitions, conveyors, grabbers). When migrating each bucket run the relevant scenario to ensure nothing silently stops.
- Consider adding a debug overlay listing how many systems picked each profile to catch accidental regressions quickly.

## vendor/afterhours review & possible asks
- `afterhours::SystemBase` only exposes `virtual bool should_run(const float)` (no user context).
```47:55:vendor/afterhours/src/core/system.h
virtual bool should_run(const float) { return true; }
virtual bool should_run(const float) const { return true; }
virtual void once(const float) {}
```
- `SystemManager::tick` iterates systems and only passes `dt`, so every caller must reach into global state themselves.
```358:373:vendor/afterhours/src/core/system.h
for (auto &system : update_systems_) {
    if (!system->should_run(dt))
        continue;
    system->once(dt);
    for (std::shared_ptr<Entity> entity : entities) {
        if (!entity)
            continue;
        if (system->include_derived_children)
            system->for_each_derived(*entity, dt);
        else
            system->for_each(*entity, dt);
    }
    system->after(dt);
    EntityHelper::merge_entity_arrays();
}
```
- **Potential maintainer requests:**
  1. *Optional frame context hook*: allow hosts to register a `void* user_context` or `std::any` on `SystemManager` that gets passed into `should_run`/`once`/`after`. That would let us (and others) avoid global lookups and express guard logic once without changing every derived class.
  2. *System tagging*: provide lightweight tags (e.g., `system->update_tags = Tag::Night | Tag::Transition`) that `SystemManager` can check before invoking `should_run`. We could then skip entire groups when the game is paused or in a menu, and tooling could introspect tag coverage.
  3. *Pre/post callbacks*: expose hooks around the `for (auto &system)` loop so hosts can inject logic (profiling, guard enforcement, instrumentation) without forking the vendor file. That hook could also hand us the frame context that we would otherwise compute ourselves.
  4. *Name/identifier support*: letting `SystemBase` carry an optional string id would simplify logging/debugging when we toggle guards.
- None of these are blockers—the plan above can land entirely within our repo—but having upstream support would make future guard work cleaner and could benefit any afterhours consumer.

## Next steps
- Build `SystemRunContext` + guard helpers behind a feature flag so we can start migrating immediately.
- Reach out to the afterhours maintainer with the optional asks once we have a concrete prototype to show the value (screenshots of guard stats, etc.).
- Prioritize migrating the most error-prone buckets first (transition systems and conveyors/grabbers) so we retire the bulk of the duplicated guard code quickly.
